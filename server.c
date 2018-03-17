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

/*
 *TODO:
 * - add description
 * - think, ???setnonblock function instead of exit
 *   use return -1 and make abort ???
 * - redesign variables in main function
 * - register for EPOLLIN event for client socket
 * */


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
    int rv, sfd, cfd, c_port, efd, en, i, enable = 1;
    char c_ip[INET6_ADDRSTRLEN];
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
    void *src;

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
            // got event from server socket
            if (events[i].data.fd == sfd) {
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
                if ((getnameinfo(
                        (struct sockaddr *)&c_addr, addr_size,
                        hbuf, sizeof(hbuf), sbuf, sizeof(sbuf),
                        NI_NUMERICHOST | NI_NUMERICSERV)) == 0) {
                    printf("Got connection from %s %s\n", hbuf, sbuf);
                }

                setnonblock(cfd);
            }
        }
    }

    return 0;
}

