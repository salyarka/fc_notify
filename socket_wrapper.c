#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>


static void get_addr(const char *service, const char *host,
                        struct addrinfo **addr)
{
    struct addrinfo hints;
    int result_code;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((result_code = getaddrinfo(host, service, &hints, addr)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result_code));
        exit(1);
    }
}


int make_server_socket(const char *service)
{
    struct addrinfo *res_addrinfo, *addr;
    int sfd, enable = 1;

    get_addr(service, NULL, &res_addrinfo);

    for (addr = res_addrinfo; addr != NULL; addr = addr->ai_next) {
        // creating server socket
        if ((sfd = socket(addr->ai_family, addr->ai_socktype,
                        addr->ai_protocol)) < 0) {
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
        if (bind(sfd, addr->ai_addr, addr->ai_addrlen) < 0) {
            close(sfd);
            perror("Cant bind server socket");
            continue;
        }

        break;
    }

    freeaddrinfo(res_addrinfo);

    if (addr == NULL) {
        fprintf(stderr, "Failed to bind server socket\n");
        exit(1);
    }

    if (listen(sfd, 128) < 0) {
        perror("Cant listen connections");
        exit(1);
    }

    return sfd;
}

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
}

int make_connection(const char *host, const char *port)
{
    int sfd;
    struct addrinfo *res_addrinfo, *addr;

    get_addr(port, host, &res_addrinfo);

    for (addr = res_addrinfo; addr != NULL; addr = addr->ai_next) {
        if ((sfd = socket(addr->ai_family, addr->ai_socktype,
                        addr->ai_protocol)) < 0) {
            perror("Cant create client socket");
            continue;
        }

        if (connect(sfd, addr->ai_addr, addr->ai_addrlen) < 0) {
            perror("Cant connect");
            close(sfd);
            continue;
        }

        break;
    }

    if (addr == NULL) {
        fprintf(stderr, "Cant connect with server\n");
        exit(1);
    }

    freeaddrinfo(res_addrinfo);

    printf("Connected to server\n");

    return sfd;
}
