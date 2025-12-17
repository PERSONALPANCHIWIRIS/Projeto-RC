//Placeholder function
int create_reservation(const char *uid, const char *eid_str, int pp, const char *date) {
    if (!create_res_files(eid_str, uid, pp, date)) {
        return 0;
    }
    return 0;
}

int create_e_dir(void) {
    int eid = 0;
    char eid_dirname[15];
    char reserv_dirname[25];
    char desc_dirname[25];
    int ret;
    
    for (eid = 1; eid < 999; eid++) {
        snprintf(eid_dirname, sizeof(eid_dirname), "EVENTS/%03d", eid);
        if (access(eid_dirname, F_OK) == -1) {
            break; // Diretoria não existe, podemos usar este EID
        }
    }
    ret = mkdir(eid_dirname, 0700);
    if (ret == -1)
        return 0;
    snprintf(reserv_dirname, sizeof(reserv_dirname), "EVENTS/%03d/RESERVATIONS", eid);
    ret = mkdir(reserv_dirname, 0700);
    if (ret == -1) {
        rmdir(eid_dirname);
        return 0;
    }
    snprintf(desc_dirname, sizeof(desc_dirname), "EVENTS/%03d/DESCRIPTION", eid);
    ret = mkdir(desc_dirname, 0700);
    if (ret == -1) {
        rmdir(reserv_dirname);
        rmdir(eid_dirname);
        return 0;
    }
    return eid;
}

int create_e_files(int eid, const char *uid, const char *name,
                      const char *fname, int att_size, const char *date) {
    char start_dirname[30];
    char res_dirname[30];
    FILE *fstart;
    FILE *fres;
    int res = 0;

    snprintf(start_dirname, sizeof(start_dirname), "EVENTS/%03d/START_%03d.txt", eid, eid);
    fstart = fopen(start_dirname, "w");
    if (fstart == NULL)
        return 0;
    fprintf(fstart, "%s %s %s %d %s\n", uid, name, fname, att_size, date);
    fclose(fstart);

    snprintf(res_dirname, sizeof(res_dirname), "EVENTS/%03d/RES_%03d.txt", eid, eid);
    fres = fopen(res_dirname, "w");
    if (fres == NULL)
        return 0;
    fprintf(fres, "%d\n", res); // Inicialmente, 0 reservas
    fclose(fres);

    return 1;
}

int create_cls_file(const char *eid_str) {
    char date[DATE_SIZE_SEC + 1];
    char cls_dirname[30];
    FILE *fcls;

    get_current_date(date, sizeof(date));

    snprintf(cls_dirname, sizeof(cls_dirname), "EVENTS/%s/END_%s.txt", eid_str, eid_str);
    fcls = fopen(cls_dirname, "w");
    if (fcls == NULL)
        return 0;
    fprintf(fcls, "%s\n", date);
    fclose(fcls);

    return 1;
}

int create_res_files(const char *eid_str, const char *uid, int pp, const char *date) {
    char file_date[DATE_SIZE + 1];
    char res_dirname[35];
    char res_u_dirname[60];
    char res_e_dirname[60];
    FILE *fres;
    FILE *fres_u;
    FILE *fres_e;
    int n_res = 0;
    int day, month, year;
    int hour, min, sec;

    // Convert date to file-friendly format YYYYMMDD_HHMMSS
    sscanf(date, "%d-%d-%d %d:%d:%d", &day, &month, &year, &hour, &min, &sec);
    snprintf(file_date, sizeof(file_date), "%04d%02d%02d_%02d%02d%02d", year, month, day, hour, min, sec);

    // Update RES file
    snprintf(res_dirname, sizeof(res_dirname), "EVENTS/%s/RES_%s.txt", eid_str, eid_str);
    fres = fopen(res_dirname, "r+");
    if (fres == NULL)
        return 0;
    fscanf(fres, "%d", &n_res);
    n_res += pp;
    rewind(fres);
    fprintf(fres, "%d\n", n_res);
    fclose(fres);

    // Create reservation files
    snprintf(res_u_dirname, sizeof(res_u_dirname), "EVENTS/%s/RESERVATIONS/R-%s-%s.txt", eid_str, uid, file_date);
    snprintf(res_e_dirname, sizeof(res_e_dirname), "USERS/%s/RESERVED/R-%s-%s.txt", uid, uid, file_date);
    fres_e = fopen(res_e_dirname, "w");
    if (fres_e == NULL)
        return 0;
    fprintf(fres_e, "%s %d %s\n", uid, pp, date);
    fclose(fres_e);
    fres_u = fopen(res_u_dirname, "w");
    if (fres_u == NULL)
        return 0;
    fprintf(fres_u, "%s %d %s\n", uid, pp, date);
    fclose(fres_u);

    return 1;
}

int get_status(const char *date, const char *eid_str) {
    // Check if closed by UID
    char end_dirname[30];

    snprintf(end_dirname, sizeof(end_dirname), "EVENTS/%s/END_%s.txt", eid_str, eid_str);
    if (dir_exists(end_dirname)) {
        return 3; // Closed
    }

    // Check date
    if (check_future_date(date)) {
        // Future event
        // Check if sold out
        int avail = check_sold_out(eid_str);
        if (avail == 1) {
            return 1; // Future and not sold out
        }
        if (avail == 0) {
            return 2; // Future and sold out
        } 
        if (avail == -1) {
            return -1; // Error checking sold out status
        }
    } else {
        return 0; // Evento passado
    }
    return 1;
}

void get_current_date(char *date_buf, size_t buf_size) {
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    snprintf(date_buf, buf_size, "%02d-%02d-%04d %02d:%02d:%02d",
             tm_now->tm_mday,
             tm_now->tm_mon + 1,
             tm_now->tm_year + 1900,
             tm_now->tm_hour,
             tm_now->tm_min,
             tm_now->tm_sec);
}

int change_pwd(const char *uid, const char *new_pwd) {
    char pass_dirname[35];
    FILE *f;

    snprintf(pass_dirname, sizeof(pass_dirname), "USERS/%s/%s_pass.txt", uid, uid);
    f = fopen(pass_dirname, "w");
    if (f == NULL) {
        return 0; // Could not open password file
    }
    fprintf(f, "%s\n", new_pwd);
    fclose(f);
    return 1;
}

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
    char uid_dirname[20];
    char login_dirname[35];

    if (!uid) return 0;
    if (strnlen(uid, UID_SIZE + 1) != UID_SIZE) return 0; // Acho que não é preciso
    for (size_t i = 0; i < UID_SIZE; i++) {
        if (!isdigit((unsigned char)uid[i])) return 0;
    }

    snprintf(uid_dirname, sizeof(uid_dirname), "USERS/%s", uid);
    if (!dir_exists(uid_dirname)) {
        return -2; // Not in database
    }
    snprintf(login_dirname, sizeof(login_dirname), "USERS/%s/%s_login.txt", uid, uid);
    if (!dir_exists(login_dirname)) {
        return -1; // Not logged in
    }

    return 1;
}

int check_pwd(const char *uid, const char *pwd) {
    char pass_dirname[35];
    char u_pwd[PWD_SIZE + 1];
    FILE *f;

    if (!uid || !pwd) return 0;
    if (strnlen(pwd, PWD_SIZE + 1) != PWD_SIZE) return 0; // Acho que não é preciso
    for (size_t i = 0; i < PWD_SIZE; i++) {
        if (!isalnum((unsigned char)pwd[i])) return 0;
    }

    snprintf(pass_dirname, sizeof(pass_dirname), "USERS/%s/%s_pass.txt", uid, uid);
    if (!dir_exists(pass_dirname)) {
        return 0; // Password file does not exist
    }
    f = fopen(pass_dirname, "r");
    if (f == NULL) {
        return 0; // Could not open password file
    }
    if (fgets(u_pwd, sizeof(u_pwd), f) == NULL) {
        fclose(f);
        return 0; // Could not read password
    }
    fclose(f);
    // Remove newline character if present
    size_t len = strnlen(u_pwd, PWD_SIZE + 1);
    if (len > 0 && u_pwd[len - 1] == '\n') {
        u_pwd[len - 1] = '\0';
    }
    if (strncmp(u_pwd, pwd, PWD_SIZE) != 0) {
        return -1; // Password does not match
    }
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
    if (strnlen(date, DATE_SIZE + 1) != DATE_SIZE) return 0;
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
    char eid_dirname[15];

    if (!eid_str) return 0;
    if (strnlen(eid_str, EID_SIZE + 1) != EID_SIZE) return 0;
    for (size_t i = 0; i < EID_SIZE; i++) {
        if (!isdigit((unsigned char)eid_str[i])) return 0;
    }

    snprintf(eid_dirname, sizeof(eid_dirname), "EVENTS/%s", eid_str);
    if (!dir_exists(eid_dirname)) {
        return -1; // EID does not exist
    }
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

int check_owner(const char *uid, const char *eid_str) {
    char start_dirname[30];
    FILE *fstart;
    char owner_uid[UID_SIZE + 1];

    snprintf(start_dirname, sizeof(start_dirname), "EVENTS/%s/START_%s.txt", eid_str, eid_str);
    fstart = fopen(start_dirname, "r");
    if (fstart == NULL) {
        return 0; // Could not open start file
    }
    if (fscanf(fstart, "%s", owner_uid) != 1) {
        fclose(fstart);
        return 0; // Could not read owner UID
    }
    fclose(fstart);

    if (strncmp(owner_uid, uid, UID_SIZE) == 0) {
        return 1; // Is owner
    } else {
        return 0; // Not owner
    }
}

int check_sold_out(const char *eid_str) {
    char start_dirname[30];
    char res_dirname[30];
    FILE *fstart;
    FILE *fres;
    int att_size = 0;
    int current_res = 0;
    int avail = 0;

    snprintf(start_dirname, sizeof(start_dirname), "EVENTS/%s/START_%s.txt", eid_str, eid_str);
    fstart = fopen(start_dirname, "r");
    if (fstart == NULL) {
        return -1; // Could not open start file
    }
    // Skip owner UID, event name and file name
    fscanf(fstart, "%*s %*s %*s %d", &att_size);
    fclose(fstart);

    snprintf(res_dirname, sizeof(res_dirname), "EVENTS/%s/RES_%s.txt", eid_str, eid_str);
    fres = fopen(res_dirname, "r");
    if (fres == NULL) {
        return -1; // Could not open reservations file
    }
    fscanf(fres, "%d", &current_res);
    fclose(fres);

    if (current_res >= att_size) {
        return 0; // Sold out
    } else {
        avail = att_size - current_res;
        return avail; // Not sold out
    }
}

int check_cls(const char *eid_str) {
    char end_dirname[30];

    snprintf(end_dirname, sizeof(end_dirname), "EVENTS/%s/END_%s.txt", eid_str, eid_str);
    if (dir_exists(end_dirname)) {
        return 0; // Event is closed
    } else {
        return 1; // Event is not closed
    }
}

int check_future_date(const char *date) {
    int day, month, year;
    int hour, minute;
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);

    sscanf(date, "%d-%d-%d %d:%d", &day, &month, &year, &hour, &minute);
    if (year < (tm_now->tm_year + 1900)) return 0;
    if (year > (tm_now->tm_year + 1900)) return 1;
    if (month < (tm_now->tm_mon + 1)) return 0;
    if (month > (tm_now->tm_mon + 1)) return 1;
    if (day < tm_now->tm_mday) return 0;
    if (day > tm_now->tm_mday) return 1;
    if (hour < tm_now->tm_hour) return 0;
    if (hour > tm_now->tm_hour) return 1;
    if (minute < tm_now->tm_min) return 0;
    return 1;
}

int check_future_event(const char *eid_str) {
    char start_dirname[30];
    FILE *fstart;
    char full_date[DATE_SIZE + 1];
    char date[P_DATE_SIZE + 1];
    char time[TIME_SIZE + 1];

    snprintf(start_dirname, sizeof(start_dirname), "EVENTS/%s/START_%s.txt", eid_str, eid_str);
    fstart = fopen(start_dirname, "r");
    if (fstart == NULL) {
        return -1; // Could not open start file
    }
    // Skip owner UID, event name, file name and attendance
    fscanf(fstart, "%*s %*s %*s %*d %s %s", date, time);
    fclose(fstart);

    sscanf(full_date, "%s %s", date, time);

    if (check_future_date(full_date)) {
        return 1; // Future event
    } else {
        return 0; // Past event
    }
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
    char full_date[DATE_SIZE + 1] = {0};
    char date[P_DATE_SIZE + 1] = {0};
    char time[TIME_SIZE + 1] = {0};
    char att_str[ATT_SIZE_STR + 1] = {0};
    char fname[F_NAME_SIZE + 1] = {0};
    char fsize_str[F_SIZE_STR + 1] = {0};
    
    int att_size = 0;
    int fsize = 0;
    int eid = 0;
    char eid_str[EID_SIZE + 1] = {0};
    char desc_dirname[60];
    FILE *fdesc;
    /* parse tokens in order: UID password name event_date ... */
    // Read UID
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

    // Read password
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
    
    // Read event_name
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

    // Read event_date
    token = strtok_r(NULL, " ", &saveptr);
    if (!token || strlen(token) != P_DATE_SIZE) {
        client_reply(connect_fd, "RCE", "NOK", NULL);
        return 0;
    }
    strncpy(date, token, P_DATE_SIZE);

    token = strtok_r(NULL, " ", &saveptr);
    if (!token || strlen(token) != TIME_SIZE) {
        client_reply(connect_fd, "RCE", "NOK", NULL);
        return 0;
    }
    strncpy(time, token, TIME_SIZE);

    snprintf(full_date, sizeof(full_date), "%s %s", date, time);
    if (!check_date(full_date)) {
        client_reply(connect_fd, "RCE", "NOK", NULL);
        return 0;
    }

    // Read attendance_size
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

    // Read file_name
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

    // Read file_size
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

    // Create event
    eid = create_e_dir();
    if (eid == 0) {
        client_reply(connect_fd, "RCE", "NOK", NULL);
        return 0;
    }

    err = create_e_files(eid, uid, name, fname, att_size, date);
    if (err == 0) {
        client_reply(connect_fd, "RCE", "NOK", NULL);
        return 0;
    }

    // Read file_data to DESC directory
    snprintf(desc_dirname, sizeof(desc_dirname), "EVENTS/%03d/DESCRIPTION/%s", eid, fname);
    fdesc = fopen(desc_dirname, "wb");
    if (fdesc == NULL) {
        client_reply(connect_fd, "RCE", "NOK", NULL);
        return 0;
    }
    char file_buffer[1024];
    size_t bytes_remaining = fsize;
    while (bytes_remaining > 0) {
        size_t bytes_to_read = (bytes_remaining < sizeof(file_buffer)) ? bytes_remaining : sizeof(file_buffer);
        ssize_t bytes_read = read(connect_fd, file_buffer, bytes_to_read);
        if (bytes_read <= 0) {
            fclose(fdesc);
            client_reply(connect_fd, "RCE", "NOK", NULL);
            return 0;
        }
        fwrite(file_buffer, 1, bytes_read, fdesc);
        bytes_remaining -= bytes_read;
    }
    fclose(fdesc);

    snprintf(eid_str, sizeof(eid_str), "%03d", eid);
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

    // Read UID
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

    // Read password
    token = strtok_r(NULL, " ", &saveptr);
    if (!token || strlen(token) != PWD_SIZE) {
        client_reply(connect_fd, "RCL", "NOK", NULL);
        return 0;
    }
    strncpy(pwd, token, PWD_SIZE);
    err = check_pwd(uid, pwd);
    if (err != 1) {
        client_reply(connect_fd, "RCL", "NOK", NULL);
        return 0;
    }

    // Read EID
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

    err = check_owner(uid, eid_str);
    if (err == 0) {
        client_reply(connect_fd, "RCL", "EOW", NULL); // Not owner
        return 0;
    }

    err = check_sold_out(eid_str);
    if (err == -1) {
        client_reply(connect_fd, "RCL", "NOK", NULL); // Failure
        return 0;
    }
    if (err == 0) {
        client_reply(connect_fd, "RCL", "SLD", NULL); // Sold out
        return 0;
    }

    err = check_future_event(eid_str);
    if (err == -1) {
        client_reply(connect_fd, "RCL", "NOK", NULL); // Failure
        return 0;
    }
    if (err == 0) {
        client_reply(connect_fd, "RCL", "PST", NULL); // Event past
        return 0;
    }

    err = check_cls(eid_str);
    if (err == 0) {
        client_reply(connect_fd, "RCL", "CLO", NULL); // Already closed
        return 0;
    }

    // Close event
    err = create_cls_file(eid_str);
    if (err == 0) {
        client_reply(connect_fd, "RCL", "NOK", NULL); // Failure
        return 0;
    }

    client_reply(connect_fd, "RCL", "OK", NULL);

    return 1;
}

int handle_lst(const char *args, int connect_fd) {
    // Implement the logic for listing events
    int status = 0;
    char state_str[2];
    char name[E_NAME_SIZE + 1];
    char full_date[DATE_SIZE + 1];
    char date[P_DATE_SIZE + 1];
    char time[TIME_SIZE + 1];
    char eid_str[EID_SIZE + 1];
    char *event_list = NULL;
    char start_dirname[35];
    char res_dirname[35];
    FILE *fstart;
    DIR *dir;
    struct dirent *entry;
    dir = opendir("EVENTS");

    if (dir == NULL) {
        client_reply(connect_fd, "RLS", "NOK", NULL);
        return 0; // Could not open EVENTS directory
    }

    while((entry = readdir(dir)) != NULL) {
        // Ignore . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        // Make file paths
        strncpy(eid_str, entry->d_name, EID_SIZE);
        eid_str[EID_SIZE] = '\0';
        snprintf(start_dirname, sizeof(start_dirname), "EVENTS/%s/START_%s.txt", eid_str, eid_str);
        snprintf(res_dirname, sizeof(res_dirname), "EVENTS/%s/RES_%s.txt", eid_str, eid_str);

        // Open START file
        fstart = fopen(start_dirname, "r");
        if (fstart == NULL) {
            continue; // Could not open start file, skip this event
        }
        // Read event details, skip owner UID, file name and attendance
        fscanf(fstart, "%*s %s %*s %*d %s %s", name, date, time);
        fclose(fstart);
        snprintf(full_date, sizeof(full_date), "%s %s", date, time);
        
        status = get_status(full_date, eid_str);
        if (status == -1) {
            continue; // Error getting status, skip this event
        }

        // Append event details to event_list
        strncat(event_list, eid_str, sizeof(event_list) - strlen(event_list) - 1);
        strncat(event_list, " ", sizeof(event_list) - strlen(event_list) - 1);
        strncat(event_list, name, sizeof(event_list) - strlen(event_list) - 1);
        strncat(event_list, " ", sizeof(event_list) - strlen(event_list) - 1);
        snprintf(state_str, sizeof(state_str), "%d", status);
        strncat(event_list, state_str, sizeof(event_list) - strlen(event_list) - 1);
        strncat(event_list, " ", sizeof(event_list) - strlen(event_list) - 1);
        strncat(event_list, full_date, sizeof(event_list) - strlen(event_list) - 1);
        strncat(event_list, " ", sizeof(event_list) - strlen(event_list) - 1);
    }

    closedir(dir);
    
    if (event_list == NULL) {
        client_reply(connect_fd, "RLS", "NOK", NULL);
        return 0; // No events found
    }

    strncat(event_list, "\n", sizeof(event_list) - strlen(event_list) - 1);

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
    int future = 0;

    int att = 0;
    int res = 0;
    int fsize = 0;

    char eid_str[EID_SIZE + 1] = {0};
    char uid[UID_SIZE + 1] = {0};
    char name[E_NAME_SIZE + 1] = {0};
    char full_date[DATE_SIZE + 1] = {0};
    char date[P_DATE_SIZE + 1] = {0};
    char time[TIME_SIZE + 1] = {0};
    char att_str[ATT_SIZE_STR + 1] = {0};
    char res_str[ATT_SIZE_STR + 1] = {0};
    char fname[F_NAME_SIZE + 1];
    char fsize_str[F_SIZE_STR + 1];
    char fdata[F_SIZE + 1];
    
    char start_dirname[35];
    char res_dirname[35];
    char file_dirname[FILENAME_MAX + 30];
    FILE *fstart;
    FILE *fres;
    FILE *ffile;

    char *file_data = NULL;

    // Read EID
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

    // Read event details from START file
    snprintf(start_dirname, sizeof(start_dirname), "EVENTS/%s/START_%s.txt", eid_str, eid_str);
    if (!dir_exists(start_dirname)) {
        client_reply(connect_fd, "RSE", "NOK", NULL);
        return 0;
    }
    fstart = fopen(start_dirname, "r");
    if (fstart == NULL) {
        client_reply(connect_fd, "RSE", "NOK", NULL);
        return 0;
    }
    fscanf(fstart, "%s %s %s %d %s %s", uid, name, fname, &att, date, time);
    fclose(fstart);
    snprintf(full_date, sizeof(full_date), "%s %s", date, time);

    // Read current reservations from RES file
    snprintf(res_dirname, sizeof(res_dirname), "EVENTS/%s/RES_%s.txt", eid_str, eid_str);
    if (!dir_exists(res_dirname)) {
        client_reply(connect_fd, "RSE", "NOK", NULL);
        return 0;
    }
    fres = fopen(res_dirname, "r");
    if (fres == NULL) {
        client_reply(connect_fd, "RSE", "NOK", NULL);
        return 0;
    }
    fscanf(fres, "%d", &res);
    fclose(fres);
    snprintf(att_str, sizeof(att_str), "%d", att);
    snprintf(res_str, sizeof(res_str), "%d", res);

    // Read event description file
    snprintf(file_dirname, sizeof(file_dirname), "EVENTS/%s/DESCRIPTIONS/%s", eid_str, fname);
    if (!dir_exists(file_dirname)) {
        client_reply(connect_fd, "RSE", "NOK", NULL);
        return 0;
    }
    ffile = fopen(file_dirname, "r");
    if (ffile == NULL) {
        client_reply(connect_fd, "RSE", "NOK", NULL);
        return 0;
    }
    fseek(ffile, 0, SEEK_END);
    fsize = ftell(ffile);
    fseek(ffile, 0, SEEK_SET);
    fread(fdata, 1, fsize, ffile);
    fclose(ffile);
    fdata[fsize] = '\0';
    snprintf(fsize_str, sizeof(fsize_str), "%d", fsize);

    strcat(file_data, uid);
    strcat(file_data, " ");
    strcat(file_data, name);
    strcat(file_data, " ");
    strcat(file_data, full_date);
    strcat(file_data, " ");
    strcat(file_data, att_str);
    strcat(file_data, " ");
    strcat(file_data, res_str);
    strcat(file_data, " ");
    strcat(file_data, fname);
    strcat(file_data, " ");
    strcat(file_data, fsize_str);
    strcat(file_data, " ");
    strcat(file_data, fdata);

    // If event date is past, create CLS file
    future = check_future_date(full_date);
    if (future == 0) {
        err = create_cls_file(eid_str);
    }

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
    char date[DATE_SIZE_SEC + 1] = {0};

    //int pp = 0;
    int pp;
    pp = 0;
    int n_seats = 0;

    // Read UID
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

    // Read password
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

    // Read EID
    token = strtok_r(NULL, " ", &saveptr);
    if (!token || strlen(token) != EID_SIZE) {
        client_reply(connect_fd, "RRI", "NOK", NULL);
        return 0;
    }
    strncpy(eid_str, token, EID_SIZE);
    err = check_eid(eid_str);
    if (err != 1) {
        client_reply(connect_fd, "RRI", "NOK", NULL);
        return 0;
    }

    err = check_cls(eid_str);
    if (err == 0) {
        client_reply(connect_fd, "RRI", "CLS", NULL); // Closed
        return 0;
    }

    n_seats = check_sold_out(eid_str);
    if (n_seats == -1) {
        client_reply(connect_fd, "RRI", "NOK", NULL); // Failure
        return 0;
    }
    if (n_seats == 0) {
        client_reply(connect_fd, "RRI", "SLD", NULL); // Sold out
        return 0;
    }

    err = check_future_event(eid_str);
    if (err == -1) {
        client_reply(connect_fd, "RRI", "NOK", NULL); // Failure
        return 0;
    }
    if (err == 0) {
        client_reply(connect_fd, "RRI", "PST", NULL); // Event past
        create_cls_file(eid_str);
        return 0;
    }

    // Read people to reserve
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

    if (pp > n_seats) {
        char seats_str[16];
        snprintf(seats_str, sizeof(seats_str), "%d", n_seats);
        client_reply(connect_fd, "RRI", "REJ", seats_str); // Not enough seats
        return 0;
    }

    get_current_date(date, sizeof(date));
    
    err = create_reservation(uid, eid_str, pp, date);
    if (err == 0) {
        client_reply(connect_fd, "RRI", "NOK", NULL); // Failure
        return 0;
    }

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

    //int pp = 0;

    // Read UID
    token = strtok_r(buf, " ", &saveptr);
    if (!token || strlen(token) != UID_SIZE) {
        client_reply(connect_fd, "RCP", "NOK", NULL);
        return 0;
    }
    strncpy(uid, token, UID_SIZE);
    err = check_uid(uid);
    if (err == 0) {
        client_reply(connect_fd, "RCP", "NOK", NULL);
        return 0;
    }
    if (err == -1) {
        client_reply(connect_fd, "RCP", "NLG", NULL); // Not logged in
        return 0;
    }
    if (err == -2) {
        client_reply(connect_fd, "RCP", "NID", NULL); // Not in database
        return 0;
    }

    // Read old password
    token = strtok_r(NULL, " ", &saveptr);
    if (!token || strlen(token) != PWD_SIZE) {
        client_reply(connect_fd, "RCP", "NOK", NULL);
        return 0;
    }
    strncpy(old_pwd, token, PWD_SIZE);
    err = check_pwd(uid, old_pwd);
    if (err != 1) {
        client_reply(connect_fd, "RCP", "NOK", NULL);
        return 0;
    }

    // Read new password
    token = strtok_r(NULL, " ", &saveptr);
    if (!token || strlen(token) != PWD_SIZE) {
        client_reply(connect_fd, "RCP", "NOK", NULL);
        return 0;
    }
    strncpy(new_pwd, token, PWD_SIZE);
    err = check_pwd(uid, new_pwd);
    if (err == 0) {
        client_reply(connect_fd, "RCP", "NOK", NULL);
        return 0;
    }

    err = change_pwd(uid, new_pwd);
    if (err == 0) {
        client_reply(connect_fd, "RCP", "NOK", NULL);
        return 0;
    }

    client_reply(connect_fd, "RCP", "OK", NULL);
    return 1;
}
