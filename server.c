#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>

#include "fcn.h"

#define MAX_BUF 512
#define MAX_EV 10

int main(int argc, char *argv[])
{
    struct addrinfo hints, *res, *r;
    struct sockaddr_storage c_addr;
    struct epoll_event ev, events[MAX_EV];
    socklen_t addr_size;
    ssize_t nob, tnob = 0;
    int rv, sfd, cfd, efd, en, enable = 1;
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV], rbuf[MAX_BUF + 1];

    memset(&hints, 0, sizeof(hints)); // zero the structure
    hints.ai_family = AF_UNSPEC; // use IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use current IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
    	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	    exit(1);
    }

    for (r = res; r != NULL; r = r->ai_next) {
	    // creating server socket
	    if ((sfd = socket(r->ai_family, r->ai_socktype, r->ai_protocol)) < 0) {
            perror("Cant create server socket");
            continue;
        }

        // make the socket reusable
        if (setsockopt(sfd, SOL_SOCKET,
                    SO_REUSEADDR, &enable, sizeof(int)) < 0) {
            perror("Cant setsockopt");
            exit(1);
        }

        // bind socket to address
        if (bind(sfd, r->ai_addr, r->ai_addrlen) < 0) {
            close(sfd);
            perror("Cant bind server socket");
            continue;
        }

        break;
    }

    freeaddrinfo(res);

    if (r == NULL) {
        fprintf(stderr, "Failed to bind server socket\n");
        exit(1);
    }

    if (listen(sfd, 128) < 0) {
        perror("Cant listen connections");
        exit(1);
    }

    setnonblock(sfd);

    if ((efd = epoll_create1(0)) < 0) {
        perror("Cant create epoll instance");
        exit(1);
    }

    // register server socket on the epoll instance
    ev.data.fd = sfd;
    ev.events = EPOLLIN;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &ev) < 0) {
        perror("Cant register server socket on the epoll instance");
        exit(1);
    }

    while(1) {
        // wait for register events
        if ((en = epoll_wait(efd, events, MAX_EV, -1)) < 0) {
            perror("epoll_wait");
            exit(1);
        }

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
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = cfd;
                if (epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &ev) < 0) {
                    perror("Cant register client socket on epoll instance");
                    exit(1);
                }

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
                    // register for epollout for one shot
                    ev.events = EPOLLOUT | EPOLLONESHOT;

                    if(epoll_ctl(efd, EPOLL_CTL_MOD,
                                events[i].data.fd, &ev) < 0) {
                        perror("Cant change event on client "
                                "socket to EPOLLOUT");
                        exit(1);
                    }
                }

            // client socket ready for write
            } else if (events[i].events & EPOLLOUT) {
                printf("descriptor %d ready to write\n", events[i].data.fd);
                printf("rbuf %s\n", rbuf);
                rbuf[tnob] = *"\0";
                send(events[i].data.fd, rbuf, tnob, 0);
                // reset total number of bytes recived from socket
                tnob = 0;
                // register client socket on epoll instance with endge trigger
                ev.events = EPOLLIN | EPOLLET;
                if (epoll_ctl(efd, EPOLL_CTL_MOD, events[i].data.fd, &ev) < 0) {
                    perror("Cant register client socket on epoll instance");
                    exit(1);
                }

            }

        }
    }

    return 0;
}

