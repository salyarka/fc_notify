#ifndef SOCKET_WRAPPER_H
#define SOCKET_WRAPPER_H

#define ADDR_STR_LEN 4096

int make_server_socket(const char *service);
int make_connection();
//char *get_str_address(const struct sockaddr *addr, socklen_t addr_size,
void get_str_address(const struct sockaddr *addr, socklen_t addr_size,
        char *str, int str_len);

#endif
