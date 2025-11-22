#include "tcp_handler.h"

int check_uid(const char *uid) {
    if (!uid) return 0;
    if (strnlen(uid) != UID_SIZE) return 0;
    for (size_t i = 0; i < UID_SIZE; i++) {
        if (!isdigit((unsigned char)uid[i])) return 0;
    }
    // ADD: check if in database
    return 1;
}

int handle_cre(const char *args, int connect_fd) {
    // Implement the logic for creating an event
    char buf[1024];
    strncpy(buf, args, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *saveptr = NULL;
    char *token;

    char uid[UID_SIZE + 1] = {0};
    char pwd[PWD_SIZE + 1] = {0};
    char name[E_NAME_SIZE + 1] = {0};
    char date[DATE_SIZE + 1] = {0};
    char att_str[ATT_SIZE_STR + 1] = {0};
    char fname[F_NAME_SIZE + 1] = {0};
    char fsize[F_SIZE_STR + 1] = {0};
    /* parse tokens in order: UID password name event_date ... */
    // Ler UID
    token = strtok_r(buf, " ", &saveptr);
    if (token) strncpy(uid, token, UID_SIZE);
    if (!check_uid(uid)) return 0;

    // Ler password
    token = strtok_r(NULL, " ", &saveptr);
    if (token) strncpy(pwd, token, PWD_SIZE);
    token = strtok_r(NULL, " ", &saveptr);

    
    if (token) strncpy(name, token, E_NAME_SIZE);
    token = strtok_r(NULL, " ", &saveptr);
    if (token) strncpy(date, token, DATE_SIZE);
    token = strtok_r(NULL, " ", &saveptr);
    if (token) strncpy(att_str, token, ATT_SIZE_STR);
    token = strtok_r(NULL, " ", &saveptr);
    if (token) strncpy(fname, token, F_NAME_SIZE);
    token = strtok_r(NULL, " ", &saveptr);
    if (token) strncpy(fsize, token, F_SIZE_STR);

    return 1;
}

int handle_cls(const char *args, int connect_fd) {
    // Implement the logic for closing an event
    // Return 1 on success, 0 on failure
    return 1;
}

int handle_lst(const char *args, int connect_fd) {
    // Implement the logic for listing events
    // Return 1 on success, 0 on failure
    return 1;
}

int handle_sed(const char *args, int connect_fd) {
    // Implement the logic for sending event data
    // Return 1 on success, 0 on failure
    return 1;
}

int handle_rid(const char *args, int connect_fd) {
    // Implement the logic for making a reservation
    // Return 1 on success, 0 on failure
    return 1;
}

int handle_cps(const char *args, int connect_fd) {
    // Implement the logic for changing password
    // Return 1 on success, 0 on failure
    return 1;
}