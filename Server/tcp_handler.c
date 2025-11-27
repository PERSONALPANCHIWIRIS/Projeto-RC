#include "tcp_handler.h"

void client_reply(int connect_fd, const char *cmd, const char *status, const char *args) {
    char reply[1024];
    if (args) {
        snprintf(reply, sizeof(reply), "R%s %s %s\n", cmd, status, args);
    } else {
        snprintf(reply, sizeof(reply), "R%s %s\n", cmd, status);
    }
    write(connect_fd, reply, strlen(reply));
}

int check_uid(const char *uid) {
    if (!uid) return 0;
    if (strnlen(uid) != UID_SIZE) return 0; // Acho que não é preciso
    for (size_t i = 0; i < UID_SIZE; i++) {
        if (!isdigit((unsigned char)uid[i])) return 0;
    }
    // ADD: check if in database
    // return -2;
    // ADD: check if logged in
    // return -1;

    return 1;
}

int check_pwd(const char *uid, const char *pwd) {
    if (!uid || !pwd) return 0;
    if (strnlen(pwd) != PWD_SIZE) return 0; // Acho que não é preciso
    for (size_t i = 0; i < PWD_SIZE; i++) {
        if (!isalnum((unsigned char)pwd[i])) return 0;
    }
    // ADD: if in database, check if password matches
    // return -1;
    return 1;
}

int check_name(const char *name) {
    if (!name) return 0;
    size_t len = strnlen(name, E_NAME_SIZE + 1);
    if (len == 0 || len > E_NAME_SIZE) return 0; // Acho que não é preciso
    for (size_t i = 0; i < len; i++) {
        if (!isalnum((unsigned char)name[i])) return 0;
    }
    return 1;
}

int check_date(const char *date) {
    if (!date) return 0;
    if (strnlen(date) != DATE_SIZE) return 0; // Acho que não é preciso
    if (date[2] != '-' || date[5] != '-' || date[10] != ' ' || date[13] != ':') return 0;
    for (size_t i = 0; i < DATE_SIZE; i++) {
        if (i == 2 || i == 5 || i == 10 || i == 13) continue;
        if (!isdigit((unsigned char)date[i])) return 0;
    }
    return 1;
}

int check_att(const char *att_str) {
    if (!att_str) return 0;
    size_t len = strnlen(att_str, ATT_SIZE_STR + 1);
    if (len == 0 || len > ATT_SIZE_STR) return 0; // Acho que não é preciso
    for (size_t i = 0; i < len; i++) {
        if (!isdigit((unsigned char)att_str[i])) return 0;
    }
    int att_size = atoi(att_str);
    if (att_size < MIN_ATT || att_size > MAX_ATT) return 0;
    return att_size;
}

int check_fname(const char *fname) {
    if (!fname) return 0;
    size_t len = strnlen(fname, F_NAME_SIZE + 1);
    if (len == 0 || len > F_NAME_SIZE) return 0; // Acho que não é preciso
    for (size_t i = 0; i < len; i++) {
        if (!isalnum((unsigned char)fname[i]) && fname[i] != '.' && fname[i] != '_' && fname[i] != '-') return 0;
    }
    return 1;
}

int check_fsize(const char *fsize_str) {
    if (!fsize_str) return 0;
    size_t len = strnlen(fsize_str, F_SIZE_STR + 1);
    if (len == 0 || len > F_SIZE_STR) return 0; // Acho que não é preciso
    for (size_t i = 0; i < len; i++) {
        if (!isdigit((unsigned char)fsize_str[i])) return 0;
    }
    int fsize = atoi(fsize_str);
    if (fsize <= 0 || fsize > F_SIZE) return 0; // Será que o ficheiro pode ter tamanho 0?
    return fsize;
}

int check_eid(const char *eid_str) {
    if (!eid_str) return 0;
    if (strnlen(eid_str) != EID_SIZE) return 0;
    for (size_t i = 0; i < EID_SIZE; i++) {
        if (!isdigit((unsigned char)eid_str[i])) return 0;
    }
    // ADD: check if EID exists in database
    // return -1;
    return 1;
}

int check_pp(const char *pp_str) {
    if (!pp_str) return 0;
    size_t len = strnlen(pp_str, PP_SIZE_STR + 1);
    if (len == 0 || len > PP_SIZE_STR) return 0; // Acho que não é preciso
    for (size_t i = 0; i < len; i++) {
        if (!isdigit((unsigned char)pp_str[i])) return 0;
    }
    int pp = atoi(pp_str);
    if (pp < MIN_PP || pp > MAX_PP) return 0; // Limite superior pode ser ajustado conforme necessário
    return pp;
}

int handle_cre(const char *args, int connect_fd) {
    // Implement the logic for creating an event
    char buf[1024];
    strncpy(buf, args, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *saveptr = NULL;
    char *token;

    int err = 0;

    char uid[UID_SIZE + 1] = {0};
    char pwd[PWD_SIZE + 1] = {0};
    char name[E_NAME_SIZE + 1] = {0};
    char date[DATE_SIZE + 1] = {0};
    char att_str[ATT_SIZE_STR + 1] = {0};
    char fname[F_NAME_SIZE + 1] = {0};
    char fsize_str[F_SIZE_STR + 1] = {0};
    
    int att_size = 0;
    int fsize = 0;
    int eid = 0;
    char eid_str[EID_SIZE + 1] = {0};
    /* parse tokens in order: UID password name event_date ... */
    // Ler UID
    token = strtok_r(buf, " ", &saveptr);
    if (!token || strlen(token) != UID_SIZE) {
        client_reply(connect_fd, "RCE", "NOK", NULL);
        return 0;
    }
    strncpy(uid, token, UID_SIZE);
    err = check_uid(uid);
    if (err == 0 || err == -2) {
        client_reply(connect_fd, "RCE", "NOK", NULL);
        return 0;
    }
    if (err == -1) {
        client_reply(connect_fd, "RCE", "NLG", NULL); // Not logged in
        return 0;
    }

    // Ler password
    token = strtok_r(NULL, " ", &saveptr);
    if (!token || strlen(token) != PWD_SIZE) {
        client_reply(connect_fd, "RCE", "NOK", NULL);
        return 0;
    }
    strncpy(pwd, token, PWD_SIZE);
    err = check_pwd(uid, pwd);
    if (err == 0) {
        client_reply(connect_fd, "RCE", "NOK", NULL);
        return 0;
    }
    if (err == -1) {
        client_reply(connect_fd, "RCE", "WRP", NULL); // Wrong password
        return 0;
    }
    
    // Ler event_name
    token = strtok_r(NULL, " ", &saveptr);
    if (!token || strlen(token) > E_NAME_SIZE) {
        client_reply(connect_fd, "RCE", "NOK", NULL);
        return 0;
    }
    strncpy(name, token, E_NAME_SIZE);
    if (!check_name(name)) {
        client_reply(connect_fd, "RCE", "NOK", NULL);
        return 0;
    }

    // Ler event_date
    token = strtok_r(NULL, " ", &saveptr);
    if (!token || strlen(token) != DATE_SIZE) {
        client_reply(connect_fd, "RCE", "NOK", NULL);
        return 0;
    }
    strncpy(date, token, DATE_SIZE);
    if (!check_date(date)) {
        client_reply(connect_fd, "RCE", "NOK", NULL);
        return 0;
    }

    // Ler attendance_size
    token = strtok_r(NULL, " ", &saveptr);
    if (!token || strlen(token) > ATT_SIZE_STR) {
        client_reply(connect_fd, "RCE", "NOK", NULL);
        return 0;
    }
    strncpy(att_str, token, ATT_SIZE_STR);
    att_size = check_att(att_str);
    if (att_size == 0) {
        client_reply(connect_fd, "RCE", "NOK", NULL);
        return 0;
    }

    // Ler file_name
    token = strtok_r(NULL, " ", &saveptr);
    if (!token || strlen(token) > F_NAME_SIZE) {
        client_reply(connect_fd, "RCE", "NOK", NULL);
        return 0;
    }
    strncpy(fname, token, F_NAME_SIZE);
    if (!check_fname(fname)) {
        client_reply(connect_fd, "RCE", "NOK", NULL);
        return 0;
    }

    // Ler file_size
    token = strtok_r(NULL, " ", &saveptr);
    if (!token || strlen(token) > F_SIZE_STR) {
        client_reply(connect_fd, "RCE", "NOK", NULL);
        return 0;
    }
    strncpy(fsize_str, token, F_SIZE_STR);
    fsize = check_fsize(fsize_str);
    if (fsize == 0) {
        client_reply(connect_fd, "RCE", "NOK", NULL);
        return 0;
    }

    // Ler file_data
    // ADD: ler ficheiro de tamanho fsize

    // ADD: criar evento na base de dados

    client_reply(connect_fd, "RCE", "OK", eid_str);

    return 1;
}

int handle_cls(const char *args, int connect_fd) {
    // Implement the logic for closing an event
    char buf[1024];
    strncpy(buf, args, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *saveptr = NULL;
    char *token;

    int err = 0;

    char uid[UID_SIZE + 1] = {0};
    char pwd[PWD_SIZE + 1] = {0};
    char eid_str[EID_SIZE + 1] = {0};

    // Ler UID
    token = strtok_r(buf, " ", &saveptr);
    if (!token || strlen(token) != UID_SIZE) {
        client_reply(connect_fd, "RCL", "NOK", NULL);
        return 0;
    }
    strncpy(uid, token, UID_SIZE);
    err = check_uid(uid);
    if (err == 0 || err == -2) {
        client_reply(connect_fd, "RCL", "NOK", NULL);
        return 0;
    }
    if (err == -1) {
        client_reply(connect_fd, "RCL", "NLG", NULL); // Not logged in
        return 0;
    }

    // Ler password
    token = strtok_r(NULL, " ", &saveptr);
    if (!token || strlen(token) != PWD_SIZE) {
        client_reply(connect_fd, "RCE", "NOK", NULL);
        return 0;
    }
    strncpy(pwd, token, PWD_SIZE);
    err = check_pwd(uid, pwd);
    if (err != 1) {
        client_reply(connect_fd, "RCL", "NOK", NULL);
        return 0;
    }

    // Ler EID
    token = strtok_r(NULL, " ", &saveptr);
    if (!token || strlen(token) != EID_SIZE) {
        client_reply(connect_fd, "RCL", "NOK", NULL);
        return 0;
    }
    strncpy(eid_str, token, EID_SIZE);
    err = check_eid(eid_str);
    if (err == 0) {
        client_reply(connect_fd, "RCL", "NOK", NULL);
        return 0;
    }
    if (err == -1) {
        client_reply(connect_fd, "RCL", "NOE", NULL); // Does not exist
        return 0;
    }

    // ADD: check if user is owner of event
    // client_reply(connect_fd, "RCL", "EOW", NULL); // Not owner
    // return 0;

    // ADD: check if event is sold out
    // client_reply(connect_fd, "RCL", "SLD", NULL); // Sold out
    // return 0;

    // ADD: check if event is past
    // client_reply(connect_fd, "RCL", "PST", NULL); // Event past
    // return 0;

    // ADD: check if already closed
    // client_reply(connect_fd, "RCL", "CLO", NULL); // Already closed
    // return 0;

    // ADD: close event in database

    client_reply(connect_fd, "RCL", "OK", NULL);

    return 1;
}

int handle_lst(const char *args, int connect_fd) {
    // Implement the logic for listing events

    // ADD: check if there are any events to list
    // client_reply(connect_fd, "RLS", "NOK", NULL);
    // return 0;

    char *event_list = NULL;
    // ADD: populate event_list with events from database
    // EID name state event_date (...) \n

    client_reply(connect_fd, "RLS", "OK", event_list);

    return 1;
}

int handle_sed(const char *args, int connect_fd) {
    // Implement the logic for sending event data
    char buf[1024];
    strncpy(buf, args, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *saveptr = NULL;
    char *token;

    int err = 0;

    char eid_str[EID_SIZE + 1] = {0};

    char *file_data = NULL;

    // Ler EID
    token = strtok_r(buf, " ", &saveptr);
    if (!token || strlen(token) != EID_SIZE) {
        client_reply(connect_fd, "RSE", "NOK", NULL);
        return 0;
    }
    strncpy(eid_str, token, EID_SIZE);
    err = check_eid(eid_str);
    if (err != 1) {
        client_reply(connect_fd, "RSE", "NOK", NULL);
        return 0;
    }

    // ADD: check if there is a file
    // client_reply(connect_fd, "RSE", "NOK", NULL);
    // return 0;

    // ADD: populate file_data
    // UID name event_date attendance_size Seats_reserved Fname Fsize Fdata

    client_reply(connect_fd, "RSE", "OK", file_data);

    return 1;
}

int handle_rid(const char *args, int connect_fd) {
    // Implement the logic for making a reservation
    char buf[1024];
    strncpy(buf, args, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *saveptr = NULL;
    char *token;

    int err = 0;

    char uid[UID_SIZE + 1] = {0};
    char pwd[PWD_SIZE + 1] = {0};
    char eid_str[EID_SIZE + 1] = {0};
    char pp_str[PP_SIZE_STR + 1] = {0};

    int pp = 0;

    // Ler UID
    token = strtok_r(buf, " ", &saveptr);
    if (!token || strlen(token) != UID_SIZE) {
        client_reply(connect_fd, "RRI", "NOK", NULL);
        return 0;
    }
    strncpy(uid, token, UID_SIZE);
    err = check_uid(uid);
    if (err == 0 || err == -2) {
        client_reply(connect_fd, "RRI", "NOK", NULL);
        return 0;
    }
    if (err == -1) {
        client_reply(connect_fd, "RRI", "NLG", NULL); // Not logged in
        return 0;
    }

    // Ler password
    token = strtok_r(NULL, " ", &saveptr);
    if (!token || strlen(token) != PWD_SIZE) {
        client_reply(connect_fd, "RRI", "NOK", NULL);
        return 0;
    }
    strncpy(pwd, token, PWD_SIZE);
    err = check_pwd(uid, pwd);
    if (err == 0) {
        client_reply(connect_fd, "RRI", "NOK", NULL);
        return 0;
    }
    if (err == -1) {
        client_reply(connect_fd, "RRI", "WRP", NULL); // Wrong password
        return 0;
    }

    // Ler EID
    token = strtok_r(NULL, " ", &saveptr);
    if (!token || strlen(token) != EID_SIZE) {
        client_reply(connect_fd, "RRI", "NOK", NULL);
        return 0;
    }
    strncpy(eid_str, token, EID_SIZE);
    err = check_eid(eid_str);
    if (err == 0) {
        client_reply(connect_fd, "RRI", "NOK", NULL);
        return 0;
    }
    if (err == -1) {
        client_reply(connect_fd, "RRI", "NOE", NULL); // Does not exist
        return 0;
    }

    // ADD: check if event is closed
    // client_reply(connect_fd, "RRI", "CLS", NULL); // Closed
    // return 0;

    // ADD: check if event is sold out
    // client_reply(connect_fd, "RRI", "SLD", NULL); // Sold out
    // return 0;

    // ADD: check if event is past
    // client_reply(connect_fd, "RRI", "PST", NULL); // Event past
    // return 0;

    // Ler people to reserve
    token = strtok_r(NULL, " ", &saveptr);
    if (!token || strlen(token) > PP_SIZE_STR) {
        client_reply(connect_fd, "RRI", "NOK", NULL);
        return 0;
    }
    strncpy(pp_str, token, PP_SIZE_STR);
    pp = check_pp(pp_str);
    if (pp == 0) {
        client_reply(connect_fd, "RRI", "NOK", NULL);
        return 0;
    }

    // ADD: check if enough seats are available
    // n_seats
    // client_reply(connect_fd, "RRI", "REJ", n_seats);

    // ADD: make reservation in database

    client_reply(connect_fd, "RRI", "ACC", NULL);
    
    return 1;
}

int handle_cps(const char *args, int connect_fd) {
    // Implement the logic for changing password
    char buf[1024];
    strncpy(buf, args, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *saveptr = NULL;
    char *token;

    int err = 0;

    char uid[UID_SIZE + 1] = {0};
    char old_pwd[PWD_SIZE + 1] = {0};
    char new_pwd[PWD_SIZE + 1] = {0};

    int pp = 0;

    // Ler UID
    token = strtok_r(buf, " ", &saveptr);
    if (!token || strlen(token) != UID_SIZE) {
        client_reply(connect_fd, "RRI", "NOK", NULL);
        return 0;
    }
    strncpy(uid, token, UID_SIZE);
    err = check_uid(uid);
    if (err == 0) {
        client_reply(connect_fd, "RRI", "NOK", NULL);
        return 0;
    }
    if (err == -1) {
        client_reply(connect_fd, "RRI", "NLG", NULL); // Not logged in
        return 0;
    }
    if (err == -2) {
        client_reply(connect_fd, "RRI", "NID", NULL); // Not in database
        return 0;
    }

    // Ler old password
    token = strtok_r(NULL, " ", &saveptr);
    if (!token || strlen(token) != PWD_SIZE) {
        client_reply(connect_fd, "RRI", "NOK", NULL);
        return 0;
    }
    strncpy(old_pwd, token, PWD_SIZE);
    err = check_pwd(uid, old_pwd);
    if (err != 1) {
        client_reply(connect_fd, "RRI", "NOK", NULL);
        return 0;
    }

    // Ler new password
    token = strtok_r(NULL, " ", &saveptr);
    if (!token || strlen(token) != PWD_SIZE) {
        client_reply(connect_fd, "RRI", "NOK", NULL);
        return 0;
    }
    strncpy(new_pwd, token, PWD_SIZE);
    err = check_pwd(uid, new_pwd);
    if (err == 0) {
        client_reply(connect_fd, "RRI", "NOK", NULL);
        return 0;
    }

    // ADD: change password in database

    client_reply(connect_fd, "RRI", "OK", NULL);
    return 1;
}