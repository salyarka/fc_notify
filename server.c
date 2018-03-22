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

#define PORT "59599"
#define MAX_EV 10

// makes file non blocking
void setnonblock(int fd)
{
    int f;

    if ((f = fcntl(fd, F_GETFL)) < 0) {
        perror("Cant get flags");
        exit(1);
    }
    f |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, f) < 0) {
        perror("Cant set NONBLOCK");
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    struct addrinfo hints, *res, *r;
    struct sockaddr_storage c_addr;
    struct epoll_event ev, events[MAX_EV];
    socklen_t addr_size;
    ssize_t nob;
    int rv, sfd, cfd, efd, en, i, enable = 1;
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV], rbuf[512];
    

    memset(&hints, 0, sizeof(hints)); // zero the structure
    hints.ai_family = AF_UNSPEC; // use IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use current IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
    	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	    return 1;
    }

    for (r = res; r != NULL; r = r->ai_next) {
	    // creating server socket
	    if ((sfd = socket(r->ai_family, r->ai_socktype,	r->ai_protocol)) < 0) {
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

        for (i = 0; i < en; i++) {
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
                    // accepted all incoming connections
                    if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                        break;
                    } else {
                        perror("Cant acept connection");
                        continue;
                    }
                }

                // print info about connected client
                printf("addr_size is %d\n", addr_size);
                printf("c_addr storage %d\n", c_addr.ss_family);
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
                            sizeof(rbuf))) < 0) {
                        if (errno != EAGAIN) {
                            perror("Cant read data from client socket");
                        }
                        // all data have been received
                        break;

                    // client closed connection
                    } else if (nob == 0) {
                        close(events[i].data.fd);
                        break;
                    }
                    if (write(1, rbuf, nob) < 0) {
                        perror("Cant write to stdout");
                    }
                }
                printf("got from %d descriptor\n", events[i].data.fd);

            // client socket ready for write
            } else if (events[i].events & EPOLLOUT) {
                printf("descriptor %d ready to write\n", events[i].data.fd);

            }

        }
    }

    return 0;
}

