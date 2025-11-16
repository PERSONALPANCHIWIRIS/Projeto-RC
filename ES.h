#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>

#define base_ESport 58000 // Ainda não temos numero de grupo definido
#define MAX_BUFFER 10240
#define MAX_COMMAND 65536 //Quantos bytes vamos ler no maximo para o TCP

bool verbose = false; 

void handle_udp(int udp_fd);
void handle_tcp(int tcp_fd);
ssize_t read_line(int fd, char *buffer, size_t maxlen);
void parse_tcp_command(const char *line, int connect_fd);