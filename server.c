#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
//#include <sys/types.h>
//#include <sys/epoll.h>
#include <errno.h>

#include "fcn.h"
#include "epoll_wrapper.h"
#include "socket_wrapper.h"

#define MAX_BUF 512
#define MAX_EV 10

int main(int argc, char *argv[])
{
    struct sockaddr_storage c_addr;
    struct epoll_event events[MAX_EV];
    socklen_t addr_size;
    ssize_t nob, tnob = 0;
    int rv, sfd, cfd, efd, en;
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV], rbuf[MAX_BUF + 1];

    sfd = make_server_socket(PORT);
    setnonblock(sfd);
    efd = e_create();
    e_add(efd, sfd, EPOLLIN);

    while(1) {
        // wait for register events
        en = e_wait(efd, events);

        for (int i = 0; i < en; i++) {
            // error condition or hung up happened
            if ((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP)) {
                fprintf(stderr, "error epoll event\n");
                close(events[i].data.fd);
                continue;

            // got event from server socket
            } else if (events[i].data.fd == sfd) {
                addr_size = sizeof(c_addr);
                if ((cfd = accept(sfd,
                        (struct sockaddr *)&c_addr, &addr_size)) < 0) {
                    perror("Cant acept connection");
                    continue;
                }

                // print info about connected client
                if ((rv = getnameinfo(
                        (struct sockaddr *)&c_addr, addr_size,
                        hbuf, sizeof(hbuf), sbuf, sizeof(sbuf),
                        NI_NUMERICHOST | NI_NUMERICSERV)) == 0) {
                    printf("Client with address %s %s connected (fd %d)\n",
                            hbuf, sbuf, cfd);
                } else {
                    fprintf(stderr, "getnameinfo: %s\n", gai_strerror(rv));
                }

                // register client socket on epoll instance with endge trigger
                setnonblock(cfd);
                e_add(efd, cfd, EPOLLIN | EPOLLET);

            // got data from client
            } else if (events[i].events & EPOLLIN) {
                printf("EPOLLIN\n");
                while(1) {
                    if ((nob = read(events[i].data.fd, rbuf,
                            MAX_BUF)) < 0) {
                        if (errno != EAGAIN) {
                            perror("Cant read data from client socket");
                        }
                        // all data have been received
                        break;

                    // client closed connection
                    } else if (nob == 0) {
                        printf("Client %d disconnected\n", events[i].data.fd);
                        close(events[i].data.fd);
                        break;
                    }
                    // save total number of bytes received
                    tnob += nob;
                }
                if (tnob > 0) {
                    printf("got from %d descriptor\n", events[i].data.fd);
                    // register for epollout
                    e_mod(efd, events[i].data.fd, EPOLLOUT);
                }

            // client socket ready for write
            } else if (events[i].events & EPOLLOUT) {
                printf("descriptor %d ready to write\n", events[i].data.fd);
                printf("rbuf %s\n", rbuf);
                rbuf[tnob] = *"\0";
                send(events[i].data.fd, rbuf, tnob, 0);
                // reset total number of bytes recived from socket
                tnob = 0;
                
                e_mod(efd, events[i].data.fd, EPOLLIN | EPOLLET);

            }

        }
    }

    return 0;
}

