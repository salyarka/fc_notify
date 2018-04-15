#include <stdlib.h>
#include <stdio.h>
#include <sys/epoll.h>

#include "epoll_wrapper.h"

/* 
 * TODO:
 * - ptr argument in function for ev.data.ptr
 * */


int e_create()
{
    int efd;

    if ((efd = epoll_create1(0)) < 0) {
        perror("Cant create epoll instance");
        exit(1);
    }

    return efd;
}

void e_add(int efd, int fd, int evs)
{
    struct epoll_event ev;

    ev.data.fd = fd;
    ev.events = evs;

    if (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        perror("Cant register fd on the epoll instance");
        exit(1);
    }
}

void e_mod(int efd, int fd, int evs)
{
    struct epoll_event ev;

    ev.data.fd = fd;
    ev.events = evs;

    if (epoll_ctl(efd, EPOLL_CTL_MOD, fd, &ev) < 0) {
        perror("Cant change event associated with fd");
        exit(1);
    }
}

int e_wait(int efd, struct epoll_event *evs)
{
    int en;

    if ((en = epoll_wait(efd, evs, MAX_EVS, -1)) < 0) {
        perror("epoll_wait");
        exit(1);
    }
    return en;
}

