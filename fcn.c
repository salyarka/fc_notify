#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include "fcn.h"

void setnonblock(int fd) {
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

