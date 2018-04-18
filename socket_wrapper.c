#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>

int make_server_socket(const char *service)
{
    struct addrinfo hints, *res, *r;
    int sfd, rv, enable = 1;

    memset(&hints, 0, sizeof(hints)); // zero the structure
    hints.ai_family = AF_UNSPEC; // use IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use current IP

    if ((rv = getaddrinfo(NULL, service, &hints, &res)) != 0) {
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

    return sfd;
}

//char *get_str_address(const struct sockaddr *addr, socklen_t addr_size,
void get_str_address(const struct sockaddr *addr, socklen_t addr_size,
        char *str, int str_len)
{
    int r;
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

    if ((r = getnameinfo(addr, addr_size,
                    hbuf, sizeof(hbuf), sbuf, sizeof(sbuf),
                    NI_NUMERICHOST | NI_NUMERICSERV)) == 0) {
        snprintf(str, str_len, "%s %s", hbuf, sbuf);
    } else {
        fprintf(stderr, "getnameinfo: %s\n", gai_strerror(r));
        snprintf(str, str_len, "unknown address");
    }

    //return str;
}

