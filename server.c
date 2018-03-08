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
	struct sockaddr_storage c_addr;
	socklen_t addr_size;
	int rv, sfd, cfd;
	int enable = 1;
	char c_addr_string[INET6_ADDRSTRLEN];
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

	while(1) {
		addr_size = sizeof(c_addr);
		cfd = accept(sfd, (struct sockaddr *)&c_addr, &addr_size);

		if (cfd < 0) {
			perror("Cant accept connection");
			continue;
		}
		// TODO: client addr dont prints
		get_client_addr(c_addr, c_addr_string,
				sizeof(c_addr_string));
		printf("Got connection from %s\n", c_addr_string);
	}

	return 0;
}

// get text presentation of client address
void get_client_addr(const struct sockaddr *addr, char *s, size_t s_size) {
	if (addr->sa_family == AF_INET) {
		inet_ntop(AF_INET, &(((struct sockaddr_in *)addr)->sin_addr),
				s, s_size);
	} else if (addr->sa_family == AF_INET6) {
		inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)addr)->sin6_addr),
				s, s_size);
	}
}
