#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#define PORT "59599"

int main(int argc, char *argv[])
{
	struct addrinfo hints, *res, *r;
	int rv, sfd;
	int enable = 1;

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
		if ((sfd = socket(r->ai_family, r->ai_socktype,
						r->ai_protocol)) < 0) {
			perror("Cant create server socket");
			continue;
		}

		// make the socket reusable
		if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &enable,
					sizeof(int)) < 0) {
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
	}

	if (listen(sfd, 128) < 0) {
		perror("Cant listen connections");
		exit(1);
	}

	return 0;
}
