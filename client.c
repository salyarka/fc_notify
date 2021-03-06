#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <limits.h>  // for NAME_MAX
#include <errno.h>

#include "fcn.h"
#include "epoll_wrapper.h"
#include "socket_wrapper.h"

// size of buffer is length of inotify_event struct +
// maximum file length + zero symbol
#define IBUF_SIZE sizeof(struct inotify_event) + NAME_MAX + 1
#define MAX_EV 10

int is_dir(char *path)
{
    struct stat path_stat;
    if (stat(path, &path_stat) < 0) {
        perror("stat");
        exit(1);
    }
    return S_ISDIR(path_stat.st_mode);
}

int main(int argc, char *argv[])
{
    int sfd, inotify_fd, wd, br, en, efd;
    char buf[IBUF_SIZE], *p;
    struct inotify_event *iev;
    struct epoll_event events[MAX_EV];

    if (argc != 3) {
        fprintf(stderr, "Usage: %s [hostname] [directory]\n", argv[0]);
        exit(1);
    }

    if (is_dir(argv[2]) != 1) {
        fprintf(stderr, "%s is not a directory\n", argv[2]);
        exit(1);
    }

    if ((inotify_fd = inotify_init()) < 0) {
        perror("Cant initialize inotify");
        exit(1);
    }

    if ((wd = inotify_add_watch(inotify_fd, argv[2], IN_CREATE)) < 0) {
        perror("Cant add dir to inotify watch list");
        exit(1);
    }

    sfd = make_connection(argv[1], PORT);

    setnonblock(sfd);
    setnonblock(inotify_fd);

    efd = e_create();
    e_add(efd, sfd, EPOLLIN);
    e_add(efd, inotify_fd, EPOLLIN);

    while (1) {
        en = e_wait(efd, events);

        for (int i = 0; i < en; i++) {
            if ((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP)) {
                fprintf(stderr, "error epoll event\n");
                close(events[i].data.fd);
                continue;
            } else if (events[i].data.fd == inotify_fd) {
                while (1) { 
                    if ((br = read(inotify_fd, buf, IBUF_SIZE)) < 1) {
                        if (errno != EAGAIN) {
                            perror("Cant read from inotify file descriptor");
                            exit(1);
                        }
                        // all data have been received
                        break;
                    }
            
                    for (p = buf; p < buf + br; ) {
                        iev = (struct inotify_event *) p;
                        printf("new file %s\n", iev->name);
                        p += sizeof(struct inotify_event) + iev->len;
                    }
                }

            } else if (events[i].data.fd == sfd) {
            
            }
        }
    }

    return 0;
}

