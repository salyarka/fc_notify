all: server client
server: server.c
	gcc -o server -Wall server.c fcn.c epoll_wrapper.c socket_wrapper.c
client: client.c
	gcc -o client -Wall client.c fcn.c epoll_wrapper.c socket_wrapper.c

