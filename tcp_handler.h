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

/**
 * Aux function, check if UID is valid
 * @uid: UID
 * Return: 1 if valid, 0 if invalid
 */
int check_uid(const char *uid);

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
