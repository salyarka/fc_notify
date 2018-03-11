#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT "59599"

// get text presentation of client address
void get_client_addr(const struct sockaddr *addr, char *ip, size_t ip_size,
		uint16_t *port) {
	if (addr->sa_family == AF_INET) {
		//inet_ntop(AF_INET, &(((struct sockaddr_in *)addr)->sin_addr),
		//		ip, ip_size);
		//port = ntohs(&((struct sockaddr_in *)addr)->sin_port);
	} else if (addr->sa_family == AF_INET6) {
		//inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)addr)->sin6_addr),
		//		ip, ip_size);
		//port = ntohs(&((struct sockaddr_in6 *)addr)->sin6_port);
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
	struct sockaddr_in peer;
	int peer_len;

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


		getpeername(sfd, (struct sockaddr *)&c_addr, &addr_size);
		if (c_addr.ss_family == AF_INET) {
			struct sockaddr_in *sfd = (struct sockaddr_in *)&c_addr;
			c_port = ntohs(sfd->sin_port);
			inet_ntop(AF_INET, &sfd->sin_addr, c_ip, sizeof(c_ip));
		} else {
			struct sockaddr_in6 *sfd = (struct sockaddr_in6 *)&c_addr;
			c_port = ntohs(sfd->sin6_port);
			inet_ntop(AF_INET6, &sfd->sin6_addr, c_ip, sizeof(c_ip));
		}



		//get_client_addr(&c_addr, c_ip,
		//		sizeof(c_ip), c_port);

		//peer_len = sizeof(peer);
		//getpeername(cfd, &peer, &peer_len);
		printf("Got connection from %s %d\n", c_ip, c_port);
		//printf("Got connection from %s %d\n", inet_ntoa(peer.sin_addr), (int)ntohs(peer.sin_port));

	}

	return 0;
}

