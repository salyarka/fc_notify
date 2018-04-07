#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#include "fcn.h"

int main(int argc, char *argv[])
{
    struct addrinfo hints, *res, *r;
    int rv, sfd;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s [hostname] [directory]\n", argv[0]);
        exit(1);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], PORT, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    for (r = res; r != NULL; r = r->ai_next) {
        if ((sfd = socket(r->ai_family, r->ai_socktype, r->ai_protocol)) < 0) {
            perror("Cant create client socket");
            continue;
        }

        if (connect(sfd, r->ai_addr, r->ai_addrlen) < 0) {
            perror("Cant connect");
            close(sfd);
            continue;
        }

        break;
    }

    if (r == NULL) {
        fprintf(stderr, "Cant connect with server\n");
        exit(1);
    }

    printf("Connected to server\n");
    /*
     * TODO:
     *  - check that argv[2] is a dir
     *  - add inotify watch for new files in dir
     *  - send message to server
     * */
   return 0;
}

