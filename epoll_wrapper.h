#include <sys/epoll.h>

#ifndef EPOLL_WRAPPER_H
#define EPOLL_WRAPPER_H

#define MAX_EVS 10

int e_create();
void e_add(int efd, int fd, int evs);
void e_mod(int efd, int fd, int evs);
int e_wait(int efd, struct epoll_event *evs);

#endif
