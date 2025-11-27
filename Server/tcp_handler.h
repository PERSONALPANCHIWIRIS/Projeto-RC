#include "ES.h"
#include <ctype.h>

#define UID_SIZE 6
#define PWD_SIZE 8
#define E_NAME_SIZE 10
#define DATE_SIZE 16
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

/**
 * Sends appropriate reply to client
 * @connect_fd: TCP socket to send reply
 * @cmd: command string (3 letters)
 * @status: status string ("OK", "NOK", etc.)
 * @args: additional arguments (optional)
 */
void client_reply(int connect_fd, const char *cmd, const char *status, const char *args) {
    char reply[1024];
    if (args) {
        snprintf(reply, sizeof(reply), "R%s %s %s\n", cmd, status, args);
    } else {
        snprintf(reply, sizeof(reply), "R%s %s\n", cmd, status);
    }
    write(connect_fd, reply, strlen(reply));
}

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
 * Return: 1 if correct, 0 if incorrect, -1 if not correct pwd
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
 * Handle the CRE command to create an event
 * @args: UID password name event_date attendance_size Fname Fsize Fdata
 * @connect_fd: TCP socket to send replies
 * Return: 1 on success, 0 on failure
 */
int handle_cre(const char *args, int connect_fd);

/**
 * Handle the CLS command to close an event
 * @args: UID password EID
 * @connect_fd: TCP socket to send replies
 * Return: 1 on success, 0 on failure
 */
int handle_cls(const char *args, int connect_fd);

/**
 * Handle the LST command to list events
 * @args: no arguments
 * @connect_fd: TCP socket to send replies
 * Return: 1 on success, 0 on failure
 */
int handle_lst(const char *args, int connect_fd);

/**
 * Handle the SED command to send event data
 * @args: EID
 * @connect_fd: TCP socket to send replies
 * Return: 1 on success, 0 on failure
 */
int handle_sed(const char *args, int connect_fd);

/**
 * Handle the RID command to make a reservation
 * @args: UID password EID people
 * @connect_fd: TCP socket to send replies
 * Return: 1 on success, 0 on failure
 */
int handle_rid(const char *args, int connect_fd);

/**
 * Handle the CPS command to change password
 * @args: UID oldPassword newPassword
 * @connect_fd: TCP socket to send replies
 * Return: 1 on success, 0 on failure
 */
int handle_cps(const char *args, int connect_fd);
