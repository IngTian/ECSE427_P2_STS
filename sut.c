#include "sut.h"

// -------------------------------------------------------------------------
// ---------------------------- GLOBAL VARIABLES ---------------------------
// -------------------------------------------------------------------------
const int NUM_OF_C_EXEC = 1, NUM_OF_I_EXEC = 1, THREAD_STACK_SIZE = 1024 * 64;
struct queue g_ready_queue, g_wait_queue, g_threads_queue;
struct thread_context *g_parent_context_array[3] = {NULL, NULL, NULL};
int g_number_of_threads;
pthread_mutex_t g_ready_queue_lock, g_wait_queue_lock, g_num_threads_lock;

// -------------------------------------------------------------------------
// -------------------------------- EXECUTORS ------------------------------
// -------------------------------------------------------------------------
/**
 * This defines the C_EXEC, which is
 * responsible for executing non-I/O tasks.
 * @return Nothing.
 */
void *C_EXEC() {
    pid_t current_thread_id = gettid();
    ucontext_t *current_context = (ucontext_t *)malloc(sizeof(ucontext_t));
    thread_context *context_container = (thread_context *)malloc(sizeof(thread_context));
    context_container->thread_id = current_thread_id;
    context_container->context = current_context;
    append_parent_thread_context(context_container);

    while (true) {
        ucontext_t *next_thread = pop_ready_queue()->data;
        if (next_thread != NULL)
            swapcontext(current_context, next_thread);
        else
            usleep(100);
    }
}

/**
 * This defines the I_EXEC, which is
 * responsible for executing I/O tasks.
 * @return Nothing.
 */
void *I_EXEC() {
    pid_t current_thread_id = gettid();
    ucontext_t *current_context = (ucontext_t *)malloc(sizeof(ucontext_t *));
    thread_context *context_container = (thread_context *)malloc(sizeof(thread_context));
    context_container->thread_id = current_thread_id;
    context_container->context = current_context;
    append_parent_thread_context(context_container);

    while (true) {
        ucontext_t *next_thread = pop_wait_queue()->data;
        if (next_thread != NULL)
            swapcontext(current_context, next_thread);
        else
            usleep(100);
    }
}

// -------------------------------------------------------------------------
// ----------------------------- UTIL FUNCTIONS ----------------------------
// -------------------------------------------------------------------------
pid_t gettid(void) { return syscall(SYS_gettid); }

void append_to_ready_queue(struct queue_entry *new_entry) {
    pthread_mutex_lock(&g_ready_queue_lock);
    queue_insert_tail(&g_ready_queue, new_entry);
    pthread_mutex_unlock(&g_ready_queue_lock);
}

void append_to_wait_queue(struct queue_entry *new_entry) {
    pthread_mutex_lock(&g_wait_queue_lock);
    queue_insert_tail(&g_wait_queue, new_entry);
    pthread_mutex_unlock(&g_wait_queue_lock);
}

struct queue_entry *pop_ready_queue() {
    struct queue_entry *result;
    pthread_mutex_lock(&g_ready_queue_lock);
    result = queue_pop_head(&g_ready_queue);
    pthread_mutex_unlock(&g_ready_queue_lock);
    return result;
}

struct queue_entry *pop_wait_queue() {
    struct queue_entry *result;
    pthread_mutex_lock(&g_wait_queue_lock);
    result = queue_pop_head(&g_wait_queue);
    pthread_mutex_unlock(&g_wait_queue_lock);
    return result;
}

struct thread_context *get_parent_thread_context(pid_t thread_id) {
    for (int i = 0; i < g_number_of_threads; ++i)
        if (!g_parent_context_array[i] && g_parent_context_array[i]->thread_id == thread_id)
            return g_parent_context_array[i];
    return NULL;
}

void append_parent_thread_context(struct thread_context *new_context) {
    pthread_mutex_lock(&g_num_threads_lock);
    g_parent_context_array[g_number_of_threads++] = new_context;
    pthread_mutex_unlock(&g_num_threads_lock);
}

// -------------------------------------------------------------------------
// ------------------------------ PUBLIC APIs ------------------------------
// -------------------------------------------------------------------------
/**
 * Init the User Scheduling Library.
 */
void sut_init() {
    // Initialize global variables.
    g_number_of_threads = 0;
    g_ready_queue = queue_create();
    g_wait_queue = queue_create();
    g_threads_queue = queue_create();
    pthread_mutex_init(&g_ready_queue_lock, NULL);
    pthread_mutex_init(&g_wait_queue_lock, NULL);
    pthread_mutex_init(&g_num_threads_lock, NULL);

    // Create kernel threads.
    pthread_t *i_exe = (pthread_t *)malloc(sizeof(pthread_t));
    pthread_create(i_exe, NULL, I_EXEC, NULL);
    queue_insert_tail(&g_threads_queue, queue_new_node(i_exe));

    for (int i = 0; i < NUM_OF_C_EXEC; i++) {
        pthread_t *c_exe = (pthread_t *)malloc(sizeof(pthread_t));
        pthread_create(c_exe, NULL, C_EXEC, NULL);
        queue_insert_tail(&g_threads_queue, queue_new_node(c_exe));
    }
}

/**
 * Create a thread with the specified task.
 * @param fn A task.
 * @return True for success, and false otherwise.
 */
bool sut_create(sut_task_f fn) {
    ucontext_t *new_context = (ucontext_t *)malloc(sizeof(ucontext_t));
    getcontext(new_context);
    new_context->uc_stack.ss_sp = (char *)malloc(THREAD_STACK_SIZE);
    new_context->uc_stack.ss_size = THREAD_STACK_SIZE;
    makecontext(new_context, fn, 0);
    append_to_ready_queue(queue_new_node(new_context));
    return 1;
}

/**
 * Yield the current task.
 */
void sut_yield() {
    ucontext_t *my_context = (ucontext_t *)malloc(sizeof(ucontext_t));
    getcontext(my_context);
    append_to_ready_queue(queue_new_node(my_context));
    swapcontext(my_context, get_parent_thread_context(gettid())->context);
}

/**
 * Terminate execution.
 */
void sut_exit() {
    ucontext_t *parent_context = get_parent_thread_context(gettid())->context;
    setcontext(parent_context);
}

/**
 * Open a file
 * @param dest The destination directory.
 * @return The file descriptor.
 */
int sut_open(char *dest) {
    int fd = -1;

    // First of all, we append ourselves to
    // the wait queue and transfer our control
    // back to C_EXE.
    ucontext_t *my_context = (ucontext_t *)malloc(sizeof(ucontext_t));
    getcontext(my_context);
    append_to_wait_queue(queue_new_node(my_context));
    swapcontext(my_context, get_parent_thread_context(gettid())->context);

    // When I_EXE executes us, we shall execute the I/O.
    // After the I/O completes, we shall wait for C_EXE.
    fd = open(dest, 00700);
    append_to_ready_queue(queue_new_node(my_context));
    swapcontext(my_context, get_parent_thread_context(gettid())->context);

    // When the operation resumes, simply return.
    return fd;
}

/**
 * Write to a file.
 * @param fd The file descriptor.
 * @param buf The buffer, in which the contents are stored.
 * @param size The size of the content.
 */
void sut_write(int fd, char *buf, int size) {
    // First of all, we append ourselves to
    // the wait queue and transfer our control
    // back to C_EXE.
    ucontext_t *my_context = (ucontext_t *)malloc(sizeof(ucontext_t));
    getcontext(my_context);
    append_to_wait_queue(queue_new_node(my_context));
    swapcontext(my_context, get_parent_thread_context(gettid())->context);

    // When I_EXE executes us, we shall execute the I/O.
    // After the I/O completes, we shall wait for C_EXE.
    write(fd, buf, size);
    append_to_ready_queue(queue_new_node(my_context));
    swapcontext(my_context, get_parent_thread_context(gettid())->context);
}

/**
 * Close a file.
 * @param fd The file descriptor.
 */
void sut_close(int fd) {
    // First of all, we append ourselves to
    // the wait queue and transfer our control
    // back to C_EXE.
    ucontext_t *my_context = (ucontext_t *)malloc(sizeof(ucontext_t));
    getcontext(my_context);
    append_to_wait_queue(queue_new_node(my_context));
    swapcontext(my_context, get_parent_thread_context(gettid())->context);

    // When I_EXE executes us, we shall execute the I/O.
    // After the I/O completes, we shall wait for C_EXE.
    close(fd);
    append_to_ready_queue(queue_new_node(my_context));
    swapcontext(my_context, get_parent_thread_context(gettid())->context);
}

/**
 * Read from a file.
 * @param fd The file descriptor.
 * @param buf The buffer to which the results are stored.
 * @param size The maximum size.
 * @return A non-NULL value if success, and a NULL value otherwise.
 */
char *sut_read(int fd, char *buf, int size) {
    char *result = "success";

    // First of all, we append ourselves to
    // the wait queue and transfer our control
    // back to C_EXE.
    ucontext_t *my_context = (ucontext_t *)malloc(sizeof(ucontext_t));
    getcontext(my_context);
    append_to_wait_queue(queue_new_node(my_context));
    swapcontext(my_context, get_parent_thread_context(gettid())->context);

    // When I_EXE executes us, we shall execute the I/O.
    // After the I/O completes, we shall wait for C_EXE.
    read(fd, buf, size);
    append_to_ready_queue(queue_new_node(my_context));
    swapcontext(my_context, get_parent_thread_context(gettid())->context);

    return result;
}

/**
 * Shut down the Thread Scheduling Library.
 */
void sut_shutdown() {
    while (queue_peek_front(&g_threads_queue))
        pthread_kill(queue_pop_head(&g_threads_queue)->data, SIGUSR1);
    exit(0);
}
