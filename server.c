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

#define PORT "59599"

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
	socklen_t addr_size;
	int rv, sfd, cfd;
	int enable = 1;
	char c_ip[INET6_ADDRSTRLEN];
	int c_port;
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
		exit(1);
	}

	if (listen(sfd, 128) < 0) {
		perror("Cant listen connections");
		exit(1);
	}

	//setnonblock(sfd);

	while(1) {
		addr_size = sizeof(c_addr);
		cfd = accept(sfd, (struct sockaddr *)&c_addr, &addr_size);

		if (cfd < 0) {
			perror("Cant accept connection");
			continue;
		}

		// get info about client address
		if (c_addr.ss_family == AF_INET) {
			struct sockaddr_in *tmp = (struct sockaddr_in *)&c_addr;
			c_port = ntohs(tmp->sin_port);
			inet_ntop(AF_INET, &(tmp->sin_addr), c_ip, sizeof(c_ip));
		} else {
			struct sockaddr_in6 *tmp = (struct sockaddr_in6 *)&c_addr;
			c_port = ntohs(tmp->sin6_port);
			inet_ntop(AF_INET6, &(tmp->sin6_addr), c_ip, sizeof(c_ip));
		}

		printf("Got connection from %s %d\n", c_ip, c_port);
	}

	return 0;
}

