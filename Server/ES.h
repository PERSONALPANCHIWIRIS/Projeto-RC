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
#include <dirent.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define base_ESport 58038
#define MAX_BUFFER 10240
#define MAX_COMMAND 65536 //Quantos bytes vamos ler no maximo para o TCP
#define MAX_CMD 4 //Tamanho maximo do comando (3 letras + \0)

bool verbose = false; 

typedef struct event_list{
    char event_path[512];
    char eid[100];
    struct event_list *next;
} event_list;

void handle_udp(int udp_fd);
void handle_tcp(int tcp_fd);
ssize_t read_line(int fd, char *buffer, size_t maxlen);
void parse_tcp_command(const char *line, int connect_fd);
bool dir_exists(const char *path);
void login_user(const char *buffer, int udp_fd, struct sockaddr_in client_addr, socklen_t client_len);
void logout_user(const char *buffer, int udp_fd, struct sockaddr_in client_addr, socklen_t client_len);
void unregister_user(const char *buffer, int udp_fd, struct sockaddr_in client_addr, socklen_t client_len);
void list_events(const char *buffer, int udp_fd, struct sockaddr_in client_addr, socklen_t client_len);
void create_directories(const char *user_path, const char *uid, const char *password, int udp_fd,
     struct sockaddr_in client_addr, socklen_t client_len);
bool uid_is_valid(const char *uid);
void status_events(struct event_list *events, int udp_fd, struct sockaddr_in client_addr, socklen_t client_len);
void parse_start_content(const char *start_content,
                         int *event_attend,
                         int *start_day, int *start_month, int *start_year,
                         int *start_hour, int *start_minute);
bool check_event_date(int day, int month, int year, int hour, int minute);
void list_reservations(const char *buffer, int udp_fd, struct sockaddr_in client_addr, socklen_t client_len);