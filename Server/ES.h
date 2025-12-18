#define _POSIX_C_SOURCE 200809L
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
#include <time.h>



#define base_ESport 58038
#define MAX_BUFFER 10240
#define MAX_COMMAND 65536 //Quantos bytes vamos ler no maximo para o TCP
#define MAX_CMD 4 //Tamanho maximo do comando (3 letras + \0)

#define UID_SIZE 6
#define PWD_SIZE 8
#define E_NAME_SIZE 10
#define DATE_SIZE 16 // DD-MM-YYYY HH:MM
#define DATE_SIZE_SEC 19 // DD-MM-YYYY HH:MM:SS
#define P_DATE_SIZE 10 // DD-MM-YYYY
#define TIME_SIZE 5 // HH:MM
#define ATT_SIZE_STR 3 // max attendance size as string (max 3 digits)
#define MIN_ATT 10
#define MAX_ATT 999
#define F_NAME_SIZE 24
#define F_SIZE_STR 8 // max file size as string (max 7 digits)
#define F_SIZE 10000000 // 10MB
#define EID_SIZE 3
#define PP_SIZE_STR 3 // max people to reserve as string (max 3 digits)
#define MIN_PP 1
#define MAX_PP 999

bool verbose = false; 

typedef struct event_list{
    char event_path[512];
    char eid[100];
    struct event_list *next;
} event_list;

void handle_udp(int udp_fd);
void handle_tcp(int tcp_fd);
ssize_t read_line(int fd, char *buffer, size_t maxlen);
void parse_tcp_command(char *line, int connect_fd);
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
bool path_exists(const char *path);
bool pass_is_valid(const char *password);
bool standard_checks(int udp_fd, struct sockaddr_in client_addr, socklen_t client_len, const char *uid,
     const char *password, const char *user_path, char *reply);

/**
 * Aux function, create event directory
 * Return: EID on success, 0 on failure
 */
int create_e_dir(void);

/**
 * Aux function, create event files (START and RES)
 * @eid: EID
 * @uid: UID of event creator
 * @name: event name
 * @fname: file name
 * @att_size: attendance size
 * @date: event date
 * Return: 1 on success, 0 on failure
 */
int create_e_files(int eid, const char *uid, const char *name,
                      const char *fname, int att_size, const char *date);

/**
 * Aux function, create event closure file (END)
 * @eid: EID
 * Return: 1 on success, 0 on failure
 */                      
int create_cls_file(const char *eid_str);

/**
 * Aux function, create reservation files and updates RES file
 * @eid_str: EID string
 * @uid: UID of reserver
 * @pp: people to reserve
 * @date: reservation date
 * Return: 1 on success, 0 on failure
 */
int create_res_files(const char *eid_str, const char *uid, int pp, const char *date);

/**
 * Aux function, get event status
 * @date: event date
 * @eid_str: EID string
 * Return: 1 if future and not sold_out, 0 if past, 2 if future and sold_out, 3 if closed by UID, -1 on failure
 */
int get_status(const char *date, const char *eid_str);

/**
 * Aux function, get current date and time in format DD-MM-YYYY HH:MM
 * @date_buf: buffer to store date string
 * @buf_size: size of the buffer
 */
void get_current_date(char *date_buf, size_t buf_size);

/**
 * Aux function, change user password
 * @uid: UID
 * @new_pwd: new password
 * Return: 1 on success, 0 on failure
 */
int change_pwd(const char *uid, const char *new_pwd);
                      
/**
 * Sends appropriate reply to client
 * @connect_fd: TCP socket to send reply
 * @cmd: command string (3 letters)
 * @status: status string ("OK", "NOK", etc.)
 * @args: additional arguments (optional)
 */
void client_reply(int connect_fd, const char *cmd, const char *status, const char *args);

/**
 * Aux function, check if UID is valid
 * @uid: UID
 * Return: 1 if valid, 0 if invalid, -1 if not logged in, -2 if not in database
 */
int check_uid(const char *uid);

/**
 * Aux function, check if password is correct for given UID
 * @uid: UID
 * @pwd: password
 * Return: 1 if correct, 0 if failure, -1 if not correct pwd
 */
int check_pwd(const char *uid, const char *pwd);
// Dependendo da implementação, data_base pode ser um ponteiro para a estrutura do utilizador

/**
 * Aux function, check if event name is valid
 * @name: event name
 * Return: 1 if valid, 0 if invalid
 */
int check_name(const char *name);

/**
 * Aux function, check if date is valid
 * @date: date string
 * Return: 1 if valid, 0 if invalid
 */
int check_date(const char *date);

/**
 * Aux function, check if attendance size is valid
 * @att_str: attendance size string
 * Return: att_size if valid, 0 if invalid
 */
int check_att(const char *att_str);

/**
 * Aux function, check if file name is valid
 * @fname: file name
 * Return: 1 if valid, 0 if invalid
 */
int check_fname(const char *fname);

/**
 * Aux function, check if file size is valid
 * @fsize_str: file size string
 * Return: fsize if valid, 0 if invalid
 */
int check_fsize(const char *fsize_str);

/**
 * Aux function, check if EID is valid
 * @eid_str: EID string
 * Return: 1 if valid, 0 if invalid, -1 if does not exist
 */
int check_eid(const char *eid_str);

/**
 * Aux function, check if people to reserve is valid
 * @pp_str: people to reserve string
 * Return: pp if valid, 0 if invalid
 */
int check_pp(const char *pp_str);

/**
 * Aux function, check if user is owner of event
 * @uid: UID
 * @eid_str: EID string
 * Return: 1 if owner, 0 if not owner
 */
int check_owner(const char *uid, const char *eid_str);

/**
 * Aux function, check if event is sold out
 * @eid_str: EID string
 * Return: number of available spots if not sold out, 0 if sold out, -1 on failure
 */
int check_sold_out(const char *eid_str);

/**
 * Aux function, check if event is closed
 * @eid_str: EID string
 * Return: 1 if not closed, 0 if closed
 */
int check_cls(const char *eid_str);

/**
 * Aux function, check if date is in the future
 * @date: date DD-MM-YYYY HH:MM
 * Return: 1 if future, 0 if past
 */
int check_future_date(const char *date);

/**
 * Aux function, check if event is in the future
 * @eid_str: EID string
 * Return: 1 if future, 0 if past, -1 on failure
 */
int check_future_event(const char *eid_str);

/**
 * Handle the CRE command to create an event
 * @args: UID password name event_date attendance_size Fname Fsize Fdata
 * @connect_fd: TCP socket to send replies
 * Return: 1 on success, 0 on failure
 */
int handle_cre(int conn_fd, char *header_buffer, int bytes_read);

/**
 * Handle the CLS command to close an event
 * @args: UID password EID
 * @connect_fd: TCP socket to send replies
 * Return: 1 on success, 0 on failure
 */
//int handle_cls(const char *args, int connect_fd);

/**
 * Handle the LST command to list events
 * @args: no arguments
 * @connect_fd: TCP socket to send replies
 * Return: 1 on success, 0 on failure
 */
//int handle_lst(const char *args, int connect_fd);

/**
 * Handle the SED command to send event data
 * @args: EID
 * @connect_fd: TCP socket to send replies
 * Return: 1 on success, 0 on failure
 */
//int handle_sed(const char *args, int connect_fd);

/**
 * Handle the RID command to make a reservation
 * @args: UID password EID people
 * @connect_fd: TCP socket to send replies
 * Return: 1 on success, 0 on failure
 */
//int handle_rid(const char *args, int connect_fd);

/**
 * Handle the CPS command to change password
 * @args: UID oldPassword newPassword
 * @connect_fd: TCP socket to send replies
 * Return: 1 on success, 0 on failure
 */
//int handle_cps(const char *args, int connect_fd);