#include "sut.h"
#include "queue/queue.h"
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <ucontext.h>
#include <unistd.h>

typedef struct thread_context {
    pid_t thread_id;
    ucontext_t *context;
    bool run;
} thread_context;

pid_t gettid();

void append_to_ready_queue(struct queue_entry *new_entry);

void append_to_wait_queue(struct queue_entry *new_entry);

struct queue_entry *pop_ready_queue();

struct queue_entry *pop_wait_queue();

struct thread_context *get_parent_thread_context(pid_t thread_id);

void append_parent_thread_context(struct thread_context *new_context);

void increment_num_of_tasks();

void decrement_num_of_tasks();

ucontext_t *initialize_context();

void free_context(ucontext_t *context);

void append_to_wasted_tcb_queue(struct queue_entry *wasted_entry);

// -------------------------------------------------------------------------
// ---------------------------- GLOBAL VARIABLES ---------------------------
// -------------------------------------------------------------------------
const int NUM_OF_C_EXEC = 2, NUM_OF_I_EXEC = 1, THREAD_STACK_SIZE = 1024 * 16;
struct queue g_ready_queue, g_wait_queue, g_threads_queue, g_wasted_tcb_queue;
struct thread_context *g_parent_context_array[3] = {NULL, NULL, NULL};
int g_number_of_kernel_threads, g_num_of_tasks;
pthread_mutex_t g_ready_queue_lock, g_wait_queue_lock, g_parent_context_array_lock,
    g_num_tasks_lock, g_wasted_tcb_queue_lock;

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
    ucontext_t *current_context = initialize_context();
    thread_context *context_container = (thread_context *)malloc(sizeof(thread_context));
    context_container->thread_id = current_thread_id;
    context_container->context = current_context;
    context_container->run = true;
    append_parent_thread_context(context_container);

    while (true) {
        if (context_container->run == false && g_num_of_tasks == 0)
            pthread_exit(NULL);

        struct queue_entry *next_thread = pop_ready_queue();
        if (next_thread != NULL) {
            swapcontext(current_context, next_thread->data);
            // Register wasted memory.
            append_to_wasted_tcb_queue(next_thread);
        } else
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
    ucontext_t *current_context = initialize_context();
    thread_context *context_container = (thread_context *)malloc(sizeof(thread_context));
    context_container->thread_id = current_thread_id;
    context_container->context = current_context;
    context_container->run = true;
    append_parent_thread_context(context_container);

    while (true) {
        if (context_container->run == false && g_num_of_tasks == 0)
            pthread_exit(NULL);

        struct queue_entry *next_thread = pop_wait_queue();
        if (next_thread != NULL) {
            swapcontext(current_context, next_thread->data);
            // Register wasted memory.
            append_to_wasted_tcb_queue(next_thread);
        } else
            usleep(100);
    }
}

// -------------------------------------------------------------------------
// ----------------------------- UTIL FUNCTIONS ----------------------------
// -------------------------------------------------------------------------
/**
 * @brief Obtain the current thread ID.
 *
 * @return pid_t
 */
pid_t gettid(void) { return syscall(SYS_gettid); }

/**
 * @brief Append a TCB to the ready queue.
 *
 * @param new_entry
 */
void append_to_ready_queue(struct queue_entry *new_entry) {
    pthread_mutex_lock(&g_ready_queue_lock);
    queue_insert_tail(&g_ready_queue, new_entry);
    pthread_mutex_unlock(&g_ready_queue_lock);
}

/**
 * @brief Append a TCB to the wait queue.
 *
 * @param new_entry
 */
void append_to_wait_queue(struct queue_entry *new_entry) {
    pthread_mutex_lock(&g_wait_queue_lock);
    queue_insert_tail(&g_wait_queue, new_entry);
    pthread_mutex_unlock(&g_wait_queue_lock);
}

/**
 * @brief Pop a TCB from the ready queue.
 *
 * @return struct queue_entry*
 */
struct queue_entry *pop_ready_queue() {
    struct queue_entry *result;
    pthread_mutex_lock(&g_ready_queue_lock);
    result = queue_pop_head(&g_ready_queue);
    pthread_mutex_unlock(&g_ready_queue_lock);
    return result;
}

/**
 * @brief Pop a TCB from the wait queue.
 *
 * @return struct queue_entry*
 */
struct queue_entry *pop_wait_queue() {
    struct queue_entry *result;
    pthread_mutex_lock(&g_wait_queue_lock);
    result = queue_pop_head(&g_wait_queue);
    pthread_mutex_unlock(&g_wait_queue_lock);
    return result;
}

/**
 * @brief Get the parent executor's context.
 *
 * @param thread_id The current thread ID.
 * @return struct thread_context* The executor's context.
 */
struct thread_context *get_parent_thread_context(pid_t thread_id) {
    struct thread_context *result = NULL;
    pthread_mutex_lock(&g_parent_context_array_lock);
    for (int i = 0; i < NUM_OF_C_EXEC + NUM_OF_I_EXEC; i++)
        if (g_parent_context_array[i] && g_parent_context_array[i]->thread_id == thread_id) {
            result = g_parent_context_array[i];
            break;
        }
    pthread_mutex_unlock(&g_parent_context_array_lock);
    return result;
}

/**
 * @brief Append a new executor's context.
 *
 * @param new_context
 */
void append_parent_thread_context(struct thread_context *new_context) {
    pthread_mutex_lock(&g_parent_context_array_lock);
    g_parent_context_array[g_number_of_kernel_threads++] = new_context;
    pthread_mutex_unlock(&g_parent_context_array_lock);
}

void increment_num_of_tasks() {
    pthread_mutex_lock(&g_num_tasks_lock);
    g_num_of_tasks++;
    pthread_mutex_unlock(&g_num_tasks_lock);
}

void decrement_num_of_tasks() {
    pthread_mutex_lock(&g_num_tasks_lock);
    g_num_of_tasks--;
    pthread_mutex_unlock(&g_num_tasks_lock);
}

ucontext_t *initialize_context() {
    ucontext_t *new_context = (ucontext_t *)malloc(sizeof(ucontext_t));
    new_context->uc_stack.ss_sp = (char *)malloc(THREAD_STACK_SIZE);
    new_context->uc_stack.ss_size = THREAD_STACK_SIZE;
    new_context->uc_stack.ss_flags = 0;
    new_context->uc_link = 0;
    getcontext(new_context);
    return new_context;
}

void free_context(ucontext_t *context) {
    free(context->uc_stack.ss_sp);
    free(context);
}

void append_to_wasted_tcb_queue(struct queue_entry *wasted_entry) {
    pthread_mutex_lock(&g_wasted_tcb_queue_lock);
    queue_insert_tail(&g_wasted_tcb_queue, queue_new_node(wasted_entry));
    pthread_mutex_unlock(&g_wasted_tcb_queue_lock);
}

// -------------------------------------------------------------------------
// ------------------------------ PUBLIC APIs ------------------------------
// -------------------------------------------------------------------------
/**
 * Init the User Scheduling Library.
 */
void sut_init() {
    // Initialize global variables.
    g_number_of_kernel_threads = 0;
    g_num_of_tasks = 0;
    g_ready_queue = queue_create();
    g_wait_queue = queue_create();
    g_threads_queue = queue_create();
    g_wasted_tcb_queue = queue_create();
    queue_init(&g_ready_queue);
    queue_init(&g_wait_queue);
    queue_init(&g_threads_queue);
    queue_init(&g_wasted_tcb_queue);
    pthread_mutex_init(&g_ready_queue_lock, NULL);
    pthread_mutex_init(&g_wait_queue_lock, NULL);
    pthread_mutex_init(&g_parent_context_array_lock, NULL);
    pthread_mutex_init(&g_num_tasks_lock, NULL);
    pthread_mutex_init(&g_wasted_tcb_queue_lock, NULL);

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
    ucontext_t *new_context = initialize_context();
    makecontext(new_context, fn, 0);
    append_to_ready_queue(queue_new_node(new_context));
    increment_num_of_tasks();
    return 1;
}

/**
 * Yield the current task.
 */
void sut_yield() {
    ucontext_t *my_context = initialize_context();
    ucontext_t *parent_context = get_parent_thread_context(gettid())->context;
    append_to_ready_queue(queue_new_node(my_context));
    swapcontext(my_context, parent_context);
}

/**
 * Terminate execution.
 */
void sut_exit() {
    decrement_num_of_tasks();
    setcontext(get_parent_thread_context(gettid())->context);
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
    ucontext_t *my_context = initialize_context();
    ucontext_t *parent_context = get_parent_thread_context(gettid())->context;
    append_to_wait_queue(queue_new_node(my_context));
    swapcontext(my_context, parent_context);

    // When I_EXE executes us, we shall execute the I/O.
    // After the I/O completes, we shall wait for C_EXE.
    fd = open(dest, O_RDWR, 0777);
    my_context = initialize_context();
    parent_context = get_parent_thread_context(gettid())->context;
    append_to_ready_queue(queue_new_node(my_context));
    swapcontext(my_context, parent_context);

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
    ucontext_t *my_context = initialize_context();
    ucontext_t *parent_context = get_parent_thread_context(gettid())->context;
    append_to_wait_queue(queue_new_node(my_context));
    swapcontext(my_context, parent_context);

    // When I_EXE executes us, we shall execute the I/O.
    // After the I/O completes, we shall wait for C_EXE.
    write(fd, buf, size);
    my_context = initialize_context();
    parent_context = get_parent_thread_context(gettid())->context;
    append_to_ready_queue(queue_new_node(my_context));
    swapcontext(my_context, parent_context);
}

/**
 * Close a file.
 * @param fd The file descriptor.
 */
void sut_close(int fd) {
    // First of all, we append ourselves to
    // the wait queue and transfer our control
    // back to C_EXE.
    ucontext_t *my_context = initialize_context();
    ucontext_t *parent_context = get_parent_thread_context(gettid())->context;
    append_to_wait_queue(queue_new_node(my_context));
    swapcontext(my_context, parent_context);

    // When I_EXE executes us, we shall execute the I/O.
    // After the I/O completes, we shall wait for C_EXE.
    close(fd);
    my_context = initialize_context();
    parent_context = get_parent_thread_context(gettid())->context;
    append_to_ready_queue(queue_new_node(my_context));
    swapcontext(my_context, parent_context);
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
    ucontext_t *my_context = initialize_context();
    ucontext_t *parent_context = get_parent_thread_context(gettid())->context;
    append_to_wait_queue(queue_new_node(my_context));
    swapcontext(my_context, parent_context);

    // When I_EXE executes us, we shall execute the I/O.
    // After the I/O completes, we shall wait for C_EXE.
    if (read(fd, buf, size) <= 0)
        result = NULL;
    my_context = initialize_context();
    parent_context = get_parent_thread_context(gettid())->context;
    append_to_ready_queue(queue_new_node(my_context));
    swapcontext(my_context, parent_context);

    return result;
}

/**
 * Shut down the Thread Scheduling Library.
 */
void sut_shutdown() {
    while (g_number_of_kernel_threads < NUM_OF_C_EXEC + NUM_OF_I_EXEC)
        usleep(100);

    // Command all kernel threads to stop.
    pthread_mutex_lock(&g_parent_context_array_lock);
    for (int i = 0; i < NUM_OF_C_EXEC + NUM_OF_I_EXEC; i++) {
        g_parent_context_array[i]->run = false;
    }
    pthread_mutex_unlock(&g_parent_context_array_lock);

    // Wait for all kernel threads to complete.
    while (queue_peek_front(&g_threads_queue)) {
        struct queue_entry *entry = queue_pop_head(&g_threads_queue);
        pthread_t *thread = (pthread_t *)entry->data;
        pthread_join(*thread, NULL);
        free(entry);
        free(thread);
    }

    // Clear memory.
    for (int i = 0; i < NUM_OF_C_EXEC + NUM_OF_I_EXEC; i++) {
        free_context(g_parent_context_array[i]->context);
        free(g_parent_context_array[i]);
    }

    struct queue_entry *wasted_tcb = queue_pop_head(&g_wasted_tcb_queue);
    while (wasted_tcb) {
        struct queue_entry *inner_entry = (struct queue_entry *)wasted_tcb->data;
        ucontext_t *tcb = (ucontext_t *)inner_entry->data;
        free_context(tcb);
        free(inner_entry);
        free(wasted_tcb);
        wasted_tcb = queue_pop_head(&g_wasted_tcb_queue);
    }
}