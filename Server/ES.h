#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <stdbool.h>
#include <sys/stat.h>

#define base_ESport 58038
#define MAX_BUFFER 10240
#define MAX_COMMAND 65536 //Quantos bytes vamos ler no maximo para o TCP

bool verbose = false; 

void handle_udp(int udp_fd);
void handle_tcp(int tcp_fd);
ssize_t read_line(int fd, char *buffer, size_t maxlen);
void parse_tcp_command(const char *line, int connect_fd);
bool dir_exists(const char *path);
void login_user(const char *buffer, int udp_fd, struct sockaddr_in client_addr, socklen_t client_len);
void logout_user(const char *buffer, int udp_fd, struct sockaddr_in client_addr, socklen_t client_len);