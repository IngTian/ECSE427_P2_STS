#include "sut.h"

void sut_init() {}

bool sut_create(sut_task_f fn) {}

void sut_yield() {}

void sut_exit() {}

int sut_open(char *dest) {}

void sut_write(int fd, char *buf, int size) {}

void sut_close(int fd) {}

char *sut_read(int fd, char *buf, int size) {}

void sut_shutdown() {}