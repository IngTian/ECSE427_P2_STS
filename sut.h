#ifndef __SUT_H__
#define __SUT_H__
#include "queue/queue.h"
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <ucontext.h>
#include <unistd.h>

typedef struct thread_context {
  pid_t thread_id;
  ucontext_t *context;
} thread_context;

typedef void (*sut_task_f)();

// -------------------------------------------------------------------------
// ----------------------------- UTIL FUNCTIONS ----------------------------
// -------------------------------------------------------------------------
pid_t gettid();

void append_to_ready_queue(struct queue_entry *new_entry);

void append_to_wait_queue(struct queue_entry *new_entry);

struct queue_entry *pop_ready_queue();

struct queue_entry *pop_wait_queue();

struct thread_context *get_parent_thread_context(pid_t thread_id);

void append_parent_thread_context(struct thread_context *new_context);

// -------------------------------------------------------------------------
// ------------------------------ PUBLIC APIs ------------------------------
// -------------------------------------------------------------------------
void sut_init();
bool sut_create(sut_task_f fn);
void sut_yield();
void sut_exit();
int sut_open(char *dest);
void sut_write(int fd, char *buf, int size);
void sut_close(int fd);
char *sut_read(int fd, char *buf, int size);
void sut_shutdown();

#endif
