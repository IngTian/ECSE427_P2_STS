#ifndef __SUT_H__
#define __SUT_H__
#include "fcntl.h"
#include "pthread.h"
#include "queue/queue.h"
#include "signal.h"
#include "sys/syscall.h"
#include "sys/types.h"
#include "unistd.h"
#include <stdbool.h>
#include <ucontext.h>

typedef struct thread_context {
  pid_t thread_id;
  ucontext_t *context;
} thread_context;

typedef void (*sut_task_f)();

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
