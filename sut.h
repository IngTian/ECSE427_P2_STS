#ifndef __SUT_H__
#define __SUT_H__
#include <stdbool.h>
#include "unistd.h"
#include <ucontext.h>
#include "pthread.h"
#include "sys/types.h"
#include "sys/syscall.h"
#include "queue/queue.h"

struct thread_context {
    pthread_id_np_t thread_id;
    ucontext_t* context;
};

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
