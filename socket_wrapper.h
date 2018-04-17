#ifndef SOCKET_WRAPPER_H
#define SOCKET_WRAPPER_H

int make_server_socket(const char *service);
int make_connection();
char *get_address();

#endif
