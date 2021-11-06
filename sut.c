#include "sut.h"

// -------------------------------------------------------------------------
// ---------------------------- GLOBAL VARIABLES ---------------------------
// -------------------------------------------------------------------------
const int NUM_OF_C_EXEC = 1, NUM_OF_I_EXEC = 1, THREAD_STACK_SIZE = 1024 * 64;
struct queue g_ready_queue, g_wait_queue;
struct thread_context *g_parent_context_array[] = {NULL, NULL, NULL, NULL, NULL};
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

}

/**
 * This defines the I_EXEC, which is
 * responsible for executing I/O tasks.
 * @return Nothing.
 */
void *I_EXEC() {

}

// -------------------------------------------------------------------------
// ----------------------------- UTIL FUNCTIONS ----------------------------
// -------------------------------------------------------------------------
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

void append_to_parent_context_queue(struct queue_entry *new_entry) {
    pthread_mutex_lock(&g_parent_context_queue_lock);
    queue_insert_tail(&g_parent_context_queue, new_entry);
    pthread_mutex_unlock(&g_parent_context_queue_lock);
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

struct thread_context *get_parent_thread_context(pthread_id_np_t thread_id) {
    for (int i = 0; i < g_number_of_threads; ++i)
        if (!g_parent_context_array[i] && g_parent_context_array[i]->thread_id == thread_id)
            return g_parent_context_array[i];
    return NULL;
}

void append_parent_thread_context(struct thread_context *new_context) {
    pthread_mutex_lock(&g_num_threads_lock);
    g_parent_context_array[g_num_threads_lock++] = new_context;
    pthread_mutex_unlock(&g_num_threads_lock);
}

void swap_back_to_parent_context(ucontext_t* current_context){
    ucontext_t *parent_context = get_parent_thread_context(gettid());
    swapcontext();
}

// -------------------------------------------------------------------------
// ------------------------------ PUBLIC APIs ------------------------------
// -------------------------------------------------------------------------
/**
 * Init the User Scheduling Library.
 */
void sut_init() {}

/**
 * Create a thread with the specified task.
 * @param fn A task.
 * @return True for success, and false otherwise.
 */
bool sut_create(sut_task_f fn) {}

/**
 * Yield the current task.
 */
void sut_yield() {}

/**
 * Terminate execution.
 */
void sut_exit() {}

/**
 * Open a file
 * @param dest The destination directory.
 * @return The file descriptor.
 */
int sut_open(char *dest) {}

/**
 * Write to a file.
 * @param fd The file descriptor.
 * @param buf The buffer, in which the contents are stored.
* @param size The size of the content.
 */
void sut_write(int fd, char *buf, int size) {}

/**
 * Close a file.
 * @param fd The file descriptor.
 */
void sut_close(int fd) {}

/**
 * Read from a file.
 * @param fd The file descriptor.
 * @param buf The buffer to which the results are stored.
 * @param size The maximum size.
 * @return A non-NULL value if success, and a NULL value otherwise.
 */
char *sut_read(int fd, char *buf, int size) {}

/**
 * Shut down the Thread Scheduling Library.
 */
void sut_shutdown() {}