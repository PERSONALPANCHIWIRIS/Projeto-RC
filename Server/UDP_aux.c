

bool dir_exists(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return false;   
    return S_ISDIR(st.st_mode);                
}

bool path_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

void login_user(const char *buffer, int udp_fd, struct sockaddr_in client_addr, socklen_t client_len) {
    char uid[100], password[100];
    sscanf(buffer + 4, "%s %s", uid, password);

    if (!uid_is_valid(uid)){
        char* reply = "RLI NOK\n"; //UID inválido
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        return;
    }

    if (!pass_is_valid(password)){
        char* reply = "RLI NOK\n"; //Password inválida
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        return;
    }
    
    //Tira o path para a diretoria do ususario
    /* char* user_path = NULL;
    asprintf(&user_path, "USERS/%s", uid);
    char* pass_path = NULL;
    asprintf(&pass_path, "%s/%s_pass.txt", user_path , uid);
    char* login_path = NULL;
    asprintf(&login_path, "%s/%s_login.txt", user_path, uid); */

    int needed = snprintf(NULL, 0, "[USERS/%s", uid);
    char *user_path = malloc(needed + 1);
    snprintf(user_path, needed + 1, "USERS/%s", uid);

    int needed_pass = snprintf(NULL, 0, "%s/%s_pass.txt", user_path , uid);
    char *pass_path = malloc(needed_pass + 1);
    snprintf(pass_path, needed_pass + 1, "%s/%s_pass.txt", user_path , uid);

    int needed_login = snprintf(NULL, 0, "%s/%s_login.txt", user_path, uid);
    char *login_path = malloc(needed_login + 1);
    snprintf(login_path, needed_login + 1, "%s/%s_login.txt", user_path, uid);

    if (path_exists(login_path)) {
        char* reply = "RLI OK\n"; //Já está logged in
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        free(login_path);
        free(user_path);
        free(pass_path);
        return;
    } 

    if (dir_exists(user_path)) {//Ja está registado
        if (!path_exists(pass_path)) {
            //Usuario ja foi registado, usuario antigo
            create_directories(user_path, uid, password, udp_fd, client_addr, client_len);
            return;
        }

        //Usuario novo
        //Abre o ficheiro da password
        FILE *f = fopen(pass_path, "r");
        if (f == NULL) {
            char* reply = "RLI NOK\n"; //Erro ao abrir ficheiro
            sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
            free(user_path);
            free(pass_path);
            free(login_path);
            fclose(f);
            return; 
        }

        char stored_password[100];
        fscanf(f, "%s", stored_password);
        fclose(f);
        free(pass_path);

        if (strcmp(stored_password, password) == 0) {
            FILE *lf = fopen(login_path, "w");
            fprintf(lf, "logged\n"); //Placeholder?
            fclose(lf);
            char* reply = "RLI OK\n"; //Login com sucesso
            sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len); 
            free(login_path);
            free(user_path);
            return;
        } 
        else {
            char* reply = "RLI NOK\n"; //Password incorreta
            sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
            free(user_path);
            return;
        }
    }
    else {
        //Diretorias necessárias
        mkdir(user_path, 0700);
        create_directories(user_path, uid, password, udp_fd, client_addr, client_len);
        return;
    }     
}

void logout_user(const char *buffer, int udp_fd, struct sockaddr_in client_addr, socklen_t client_len){
    char uid[100], password[100];
    sscanf(buffer + 4, "%s %s", uid, password);

    if (!uid_is_valid(uid)){
        //printf("DEBUG 1\n");
        char* reply = "RLO NOK\n"; //UID inválido
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        return;
    }

    if (!pass_is_valid(password)){
        char* reply = "RLO WRP\n"; //Password inválida
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        return;
    }
    //char *reply;
    //asprintf(&reply, "RLO "); //Resposta base

    int needed = snprintf(NULL, 0, "RLO ");
    char *reply = malloc(needed + 1);
    snprintf(reply, needed + 1, "RLO ");
    
    /* char* user_path = NULL;
    asprintf(&user_path, "USERS/%s", uid); */

    int needed_user = snprintf(NULL, 0, "USERS/%s", uid);
    char *user_path = malloc(needed_user + 1);
    snprintf(user_path, needed_user + 1, "USERS/%s", uid);

    if (!standard_checks(udp_fd, client_addr, client_len, uid, password, user_path, reply)){
        free(user_path);
        free(reply);
        return;
    }
    else{
        //Tudo ok
        /* char* login_path = NULL;
        asprintf(&login_path, "%s/%s_login.txt", user_path, uid); */

        int needed_login = snprintf(NULL, 0, "%s/%s_login.txt", user_path, uid);
        char *login_path = malloc(needed_login + 1);
        snprintf(login_path, needed_login + 1, "%s/%s_login.txt", user_path, uid);
        unlink(login_path);

        //asprintf(&reply, "RLO OK\n"); //Logout com sucesso
        snprintf(reply, needed + 4, "RLO OK\n");
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len); 
        free(login_path);
        free(user_path);
        free(reply);
        return;
    }
}

void unregister_user(const char *buffer, int udp_fd, struct sockaddr_in client_addr, socklen_t client_len){
    char uid[100], password[100];
    sscanf(buffer + 4, "%s %s", uid, password);

    if (!uid_is_valid(uid)){
        char* reply = "RUR NOK\n"; //UID inválido
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        return;
    }

    if (!pass_is_valid(password)){
        char* reply = "RUR WRP\n"; //Password inválida
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        return;
    }
    /* char *reply;
    asprintf(&reply, "RUR "); //Resposta base */

    int needed = snprintf(NULL, 0, "RUR ");
    char *reply = malloc(needed + 1);
    snprintf(reply, needed + 1, "RUR ");
    
    /* char* user_path = NULL;
    asprintf(&user_path, "USERS/%s", uid); */

    int needed_user = snprintf(NULL, 0, "USERS/%s", uid);
    char *user_path = malloc(needed_user + 1);
    snprintf(user_path, needed_user + 1, "USERS/%s", uid);

    if (!standard_checks(udp_fd, client_addr, client_len, uid, password, user_path, reply)){
        free(user_path);
        free(reply);
        return;
    }
    else{
        //Tudo ok
        /* char* login_path = NULL;
        asprintf(&login_path, "%s/%s_login.txt", user_path, uid); */

        int needed_login = snprintf(NULL, 0, "%s/%s_login.txt", user_path, uid);
        char *login_path = malloc(needed_login + 1);
        snprintf(login_path, needed_login + 1, "%s/%s_login.txt", user_path, uid);

        /* char* pass_path = NULL;
        asprintf(&pass_path, "%s/%s_pass.txt", user_path , uid); */

        int needed_pass = snprintf(NULL, 0, "%s/%s_pass.txt", user_path , uid);
        char *pass_path = malloc(needed_pass + 1);
        snprintf(pass_path, needed_pass + 1, "%s/%s_pass.txt", user_path , uid);
        unlink(login_path);
        unlink(pass_path);

        //asprintf(&reply, "RUR OK\n"); //Unregister com sucesso
        snprintf(reply, needed + 4, "RUR OK\n");
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len); 
        free(login_path);
        free(user_path);
        free(pass_path);
        free(reply);
        return;
    }
}

void list_events(const char *buffer, int udp_fd, struct sockaddr_in client_addr, socklen_t client_len){
    char uid[100], password[100];
    sscanf(buffer + 4, "%s %s", uid, password);

    if (!uid_is_valid(uid)){
        char* reply = "RME NOK\n"; //UID inválido
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        return;
    }

    if (!pass_is_valid(password)){
        char* reply = "RME WRP\n"; //Password inválida
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        return;
    }
    
    /* char* user_path = NULL;
    asprintf(&user_path, "USERS/%s", uid); */

    int needed_user = snprintf(NULL, 0, "USERS/%s", uid);
    char *user_path = malloc(needed_user + 1);
    snprintf(user_path, needed_user + 1, "USERS/%s", uid);

    if (!dir_exists(user_path)){
        char* reply = "RME NOK\n"; //Usuario não registado
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        free(user_path);
        return;
    }

    /* char* login_path = NULL;
    asprintf(&login_path, "%s/%s_login.txt", user_path, uid); */

    int needed_login = snprintf(NULL, 0, "%s/%s_login.txt", user_path, uid);
    char *login_path = malloc(needed_login + 1);
    snprintf(login_path, needed_login + 1, "%s/%s_login.txt", user_path, uid);

    if (!path_exists(login_path)){
        char* reply = "RME NLG\n"; //Usuario não está logged in
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        free(user_path);
        free(login_path);
        return;
    }

    /* char* pass_path = NULL;
    asprintf(&pass_path, "%s/%s_pass.txt", user_path , uid); */

    int needed_pass = snprintf(NULL, 0, "%s/%s_pass.txt", user_path , uid);
    char *pass_path = malloc(needed_pass + 1);
    snprintf(pass_path, needed_pass + 1, "%s/%s_pass.txt", user_path , uid);

    FILE *f = fopen(pass_path, "r");
    if (f == NULL) {
        char* reply = "RME NOK\n"; //Erro ao abrir ficheiro
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        free(login_path);
        free(user_path);
        free(pass_path);
        fclose(f); 
        return;
    }

    char stored_password[100];
    fscanf(f, "%s", stored_password);
    fclose(f);
    free(pass_path);
    if (strcmp(stored_password, password) != 0) {
        char* reply = "RME WRP\n"; //Password incorreta
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        free(login_path);
        free(user_path);
        return;
    }
    //Esta tudo certo no input
    struct event_list *events = NULL;
    struct event_list *tail = NULL;

    /* char* created_dir = NULL;
    asprintf(&created_dir, "%s/CREATED", user_path); */

    int needed_created = snprintf(NULL, 0, "%s/CREATED", user_path);
    char *created_dir = malloc(needed_created + 1);
    snprintf(created_dir, needed_created + 1, "%s/CREATED", user_path);

    DIR *dir = opendir(created_dir);
    struct dirent *entry;
    int n_entries = 0;
    while ((entry = readdir(dir)) != NULL) {
        //Ignorar . e ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        //Espera ficheiros do tipo EID.txt
        char eid[64];
        memcpy(eid, entry->d_name, strlen(entry->d_name) - 4); //sem o .txt
        eid[strlen(entry->d_name) - 4] = '\0';

        /* char* event_path = NULL;
        asprintf(&event_path, "EVENTS/%s", eid); */

        int neede_event = snprintf(NULL, 0, "EVENTS/%s", eid);
        char *event_path = malloc(neede_event + 1);
        snprintf(event_path, neede_event + 1, "EVENTS/%s", eid);

        //Adicionar à lista
        struct event_list *new_event = malloc(sizeof(struct event_list));
        strncpy(new_event->event_path, event_path, sizeof(new_event->event_path)-1);
        new_event->event_path[sizeof(new_event->event_path)-1] = '\0';
        strncpy(new_event->eid, eid, sizeof(new_event->eid)-1);
        new_event->eid[sizeof(new_event->eid)-1] = '\0';
        new_event->next = NULL;

        //Anexar
        if (events == NULL) {
            events = new_event;
            tail = new_event;
        } else {
            tail->next = new_event;
            tail = new_event;
        }
        n_entries++;
        free(event_path);
        
    }
    closedir(dir);

    if (n_entries <= 0){
        char* reply = "RME NOK\n"; //Não existem eventos criados
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        free(created_dir);
        free(login_path);
        free(user_path);
        return;
    }
    status_events(events, udp_fd, client_addr, client_len);

    free(created_dir);
    free(login_path);
    free(user_path);
    return;
}

void create_directories(const char *user_path, const char *uid, const char *password, int udp_fd,
     struct sockaddr_in client_addr, socklen_t client_len){
    //CREATED
    /* char* created_dir = NULL;
    asprintf(&created_dir, "%s/CREATED", user_path); */

    int needed_created = snprintf(NULL, 0, "%s/CREATED", user_path);
    char *created_dir = malloc(needed_created + 1);
    snprintf(created_dir, needed_created + 1, "%s/CREATED", user_path);
    mkdir(created_dir, 0700);
    //RESERVED
    /* char* reserved_dir = NULL;
    asprintf(&reserved_dir, "%s/RESERVED", user_path); */

    int neede_reserved = snprintf(NULL, 0, "%s/RESERVED", user_path);
    char *reserved_dir = malloc(neede_reserved + 1);
    snprintf(reserved_dir, neede_reserved + 1, "%s/RESERVED", user_path);
    mkdir(reserved_dir, 0700);
    free(reserved_dir);
    free(created_dir);

    //Criamos ficheiro para a password
    /* char* pass_path = NULL;
    asprintf(&pass_path, "%s/%s_pass.txt", user_path, uid); */

    int needed_pass = snprintf(NULL, 0, "%s/%s_pass.txt", user_path, uid);
    char *pass_path = malloc(needed_pass + 1);
    snprintf(pass_path, needed_pass + 1, "%s/%s_pass.txt", user_path, uid);

    //Registamos a password
    FILE *f = fopen(pass_path, "w");
    fprintf(f, "%s\n", password);
    fclose(f);
    free(pass_path);

    /* char* login_path = NULL;
    asprintf(&login_path, "%s/%s_login.txt", user_path, uid); */

    int needed_login = snprintf(NULL, 0, "%s/%s_login.txt", user_path, uid);
    char *login_path = malloc(needed_login + 1);
    snprintf(login_path, needed_login + 1, "%s/%s_login.txt", user_path, uid);

    FILE *lf = fopen(login_path, "w");
    fprintf(lf, "logged in\n"); //Placeholder?
    fclose(lf);
    free(login_path);

    char* reply = "RLI REG\n"; //Foi registado
    sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len); 
    return;
}

bool uid_is_valid(const char *uid){
    //Verifica se o uid é valido
    for (size_t i = 0; i < strlen(uid); i++) {
        if (!(uid[i] >= '0' && uid[i] <= '9')) {
            return false;
        }
    }//É numerico

    return strlen(uid) == 6; //Deve ser de 6 digitos
}

bool pass_is_valid(const char *password){
    //Verifica se a password é valida
    return strlen(password) == 8;
}

void status_events(struct event_list *events, int udp_fd, struct sockaddr_in client_addr, socklen_t client_len){
    //Resposta a ser enviada
    /* char *reply = NULL;
    asprintf(&reply, "RME OK "); */

    int needed = snprintf(NULL, 0, "RME OK ");
    char *reply = malloc(needed + 1);
    snprintf(reply, needed + 1, "RME OK ");

    struct event_list *current = events;
    while (current != NULL) {
        //Abre a diretoria do evento
        DIR *dir = opendir(current->event_path);
        if (dir == NULL) {//Caso ocorra qualquer erro
            current = current->next;
            continue;
        }
        /* char *end_path = NULL;
        asprintf(&end_path, "%s/END_%s.txt", current->event_path, current->eid); */
        
        int needed_end = snprintf(NULL, 0, "%s/END_%s.txt", current->event_path, current->eid);
        char *end_path = malloc(needed_end + 1);
        snprintf(end_path, needed_end + 1, "%s/END_%s.txt", current->event_path, current->eid);
        if (path_exists(end_path)) {
            //Evento fechado
            /* asprintf(&reply, "%s%s 3 ", reply, current->eid); */

            int needed = snprintf(NULL, 0, "%s%s 3 ", reply, current->eid);
            char *new_reply = malloc(needed + 1);
            snprintf(new_reply, needed + 1, "%s%s 3 ", reply, current->eid);
            free(reply);
            reply = new_reply;
            free(end_path);
            current = current->next;
            continue; 
        } 
        free(end_path);

        /* char *start_path = NULL; */
        /* asprintf(&start_path, "%s/START_%s.txt", current->event_path, current->eid); */

        int neede_start = snprintf(NULL, 0, "%s/START_%s.txt", current->event_path, current->eid);
        char *start_path = malloc(neede_start + 1);
        snprintf(start_path, neede_start + 1, "%s/START_%s.txt", current->event_path, current->eid);
        FILE *fstart = fopen(start_path, "r");
        char *start_content = NULL;

        fseek(fstart, 0, SEEK_END);
        long size = ftell(fstart);
        rewind(fstart);

        start_content = malloc(size + 1);
        fread(start_content, 1, size, fstart);
        start_content[size] = '\0';
        fclose(fstart);
        free(start_path);

        //Tirar os dados do start_content
        int event_attend;
        int start_day, start_month, start_year;
        int start_hour, start_minute;
        parse_start_content(start_content, &event_attend, &start_day, &start_month, &start_year, &start_hour, &start_minute);
        free(start_content);
        if (check_event_date(start_day, start_month, start_year, start_hour, start_minute)) {
            //Evento futuro
            /* char *reservations_path = NULL;
            asprintf(&reservations_path, "%s/RESERVATIONS", current->event_path); */

            int needed_reservations = snprintf(NULL, 0, "%s/RESERVATIONS", current->event_path);
            char *reservations_path = malloc(needed_reservations + 1);
            snprintf(reservations_path, needed_reservations + 1, "%s/RESERVATIONS", current->event_path);

            DIR *res_dir = opendir(reservations_path);
            struct dirent *entry;
            int n_reservations = 0;
            while ((entry = readdir(res_dir)) != NULL) {
                //Ignorar . e ..
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                    continue;
                }
                n_reservations++;
            }
            closedir(res_dir);
            free(reservations_path);
            if (n_reservations >= event_attend) {
                //Evento cheio
                //asprintf(&reply, "%s%s 2 ", reply, current->eid);

                int needed = snprintf(NULL, 0, "%s%s 2 ", reply, current->eid);
                char *new_reply = malloc(needed + 1);
                snprintf(new_reply, needed + 1, "%s%s 2 ", reply, current->eid);
                free(reply);
                reply = new_reply; 
                current = current->next;
                continue;
            }
            else {
                //Evento com vagas
                //Evento com vagas
                //asprintf(&reply, "%s%s 1 ", reply, current->eid);

                int needed = snprintf(NULL, 0, "%s%s 1 ", reply, current->eid);
                char *new_reply = malloc(needed + 1);
                snprintf(new_reply, needed + 1, "%s%s 1 ", reply, current->eid);
                free(reply);
                reply = new_reply;
                current = current->next;
                continue;
            }
        }
        else {
            //Evento passado
            //asprintf(&reply, "%s%s 0 ", reply, current->eid);

            int needed = snprintf(NULL, 0, "%s%s 0 ", reply, current->eid);
            char *new_reply = malloc(needed + 1);
            snprintf(new_reply, needed + 1, "%s%s 0 ", reply, current->eid);
            free(reply);
            reply = new_reply;
            current = current->next;
            continue;
        }
    }
    //asprintf(&reply, "%s\n", reply);
    int needed_final = snprintf(NULL, 0, "%s\n", reply);
    char *final_reply = malloc(needed_final + 1);
    snprintf(final_reply, needed_final + 1, "%s\n", reply);
    free(reply);
    reply = final_reply;
    sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
    free(reply);

    //Libertar a lista
    current = events;
    while (current != NULL) {
        struct event_list *temp = current;
        current = current->next;
        free(temp);
    }
}

void parse_start_content(const char *start_content,
                         int *event_attend,
                         int *start_day, int *start_month, int *start_year,
                         int *start_hour, int *start_minute)
{
    char buffer[1024];
    strncpy(buffer, start_content ? start_content : "", sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    int found = 0;
    long vals[6] = {0,0,0,0,0,0}; //minuto, hora, ano, mes, dia, attendance

    int i = (int)strlen(buffer) - 1;
    while (i >= 0 && found < 6) {
        //Ignorar o que não é digito
        while (i >= 0 && !isdigit((unsigned char)buffer[i])) {
            i--;
        }
        if (i < 0) break;//Caso de erro

        //Encontramos o fim do numero, agora devemos encontrar o começo
        int end = i;
        while (i >= 0 && isdigit((unsigned char)buffer[i])) {
            i--;
        }

        //Extrair
        char temp[16];
        int len = end - i;
        if (len > 15) len = 15; //evitar overflow
        strncpy(temp, &buffer[i+1], len);
        temp[len] = '\0';

        vals[found++] = strtol(temp, NULL, 10);
    }

    *start_minute = (found >= 1) ? vals[0] : 0;
    *start_hour   = (found >= 2) ? vals[1] : 0;
    *start_year   = (found >= 3) ? vals[2] : 0;
    *start_month  = (found >= 4) ? vals[3] : 0;
    *start_day    = (found >= 5) ? vals[4] : 0;
    *event_attend = (found >= 6) ? vals[5] : 0;
}

bool check_event_date(int day, int month, int year, int hour, int minute) {
    //Obter data atual
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);

    if (year < (tm_now->tm_year + 1900)) return false;
    if (year > (tm_now->tm_year + 1900)) return true;

    if (month < (tm_now->tm_mon + 1)) return false;
    if (month > (tm_now->tm_mon + 1)) return true;

    if (day < tm_now->tm_mday) return false;
    if (day > tm_now->tm_mday) return true;

    if (hour < tm_now->tm_hour) return false;
    if (hour > tm_now->tm_hour) return true;

    if (minute < tm_now->tm_min) return false;
    return true;
}

void list_reservations(const char *buffer, int udp_fd, struct sockaddr_in client_addr, socklen_t client_len){
    //Implementar função para listar reservas
    char uid[100], password[100];
    sscanf(buffer + 4, "%s %s", uid, password);

    if (!uid_is_valid(uid)){
        char* reply = "RMR NOK\n"; //UID inválido
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        return;
    }

    if (!pass_is_valid(password)){
        char* reply = "RMR WRP\n"; //Password inválida
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        return;
    }

    /* char *user_path = NULL;
    asprintf(&user_path, "USERS/%s", uid); */

    int needed_user = snprintf(NULL, 0, "USERS/%s", uid);
    char *user_path = malloc(needed_user + 1);
    snprintf(user_path, needed_user + 1, "USERS/%s", uid);
    if (!dir_exists(user_path)){
        char* reply = "RMR NOK\n"; //Usuario não registado
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        free(user_path);
        return;
    }

    /* char *login_path = NULL;
    asprintf(&login_path, "%s/%s_login.txt", user_path, uid); */

    int needed_login = snprintf(NULL, 0, "%s/%s_login.txt", user_path, uid);
    char *login_path = malloc(needed_login + 1);
    snprintf(login_path, needed_login + 1, "%s/%s_login.txt", user_path, uid);
    if (!path_exists(login_path)){
        char* reply = "RMR NLG\n"; //Usuario não está logged in
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        free(user_path);
        free(login_path);
        return;
    }

    /* char *pass_path = NULL;
    asprintf(&pass_path, "%s/%s_pass.txt", user_path , uid); */

    int needed_pass = snprintf(NULL, 0, "%s/%s_pass.txt", user_path , uid);
    char *pass_path = malloc(needed_pass + 1);
    snprintf(pass_path, needed_pass + 1, "%s/%s_pass.txt", user_path , uid);
    FILE *f = fopen(pass_path, "r");
    if (f == NULL) {
        char* reply = "RMR NOK\n"; //Erro ao abrir ficheiro
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        free(login_path);
        free(user_path);
        free(pass_path);
        fclose(f); 
        return; 
    }   
    char stored_password[100];
    fscanf(f, "%s", stored_password);
    fclose(f);
    free(pass_path);
    if (strcmp(stored_password, password) != 0) {
        char* reply = "RMR WRP\n"; //Password incorreta
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        free(login_path);
        free(user_path);
        return;
    }   
    //Resto 
    /* char *reservation_path = NULL;
    asprintf(&reservation_path, "USERS/%s/RESERVED", uid); */

    int needed_reservation = snprintf(NULL, 0, "USERS/%s/RESERVED", uid);
    char *reservation_path = malloc(needed_reservation + 1);
    snprintf(reservation_path, needed_reservation + 1, "USERS/%s/RESERVED", uid);

    DIR *dir = opendir(reservation_path);
    struct dirent *entry;
    int n_entries = 0;

    /* char *reply = NULL;
    asprintf(&reply, "RMR OK "); */

    int needed = snprintf(NULL, 0, "RMR OK ");
    char *reply = malloc(needed + 1);
    snprintf(reply, needed + 1, "RMR OK ");
    //printf("PATH: %s\n", reservation_path);
    while ((entry = readdir(dir)) != NULL && (n_entries <= 50)) {
        //Ignorar . e ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        /* char *file_path = NULL;
        asprintf(&file_path, "%s/%s", reservation_path, entry->d_name);
 */
        int needed_file = snprintf(NULL, 0, "%s/%s", reservation_path, entry->d_name);
        char *file_path = malloc(needed_file + 1);
        snprintf(file_path, needed_file + 1, "%s/%s", reservation_path, entry->d_name);

        //printf("FILE: %s\n", file_path);

        FILE *fres = fopen(file_path, "r");
        if (!fres) {
            free(file_path);
            continue;
        }

        //le tudo
        char line[256];
        if (!fgets(line, sizeof(line), fres)) {
            fclose(fres);
            free(file_path);
            continue;
        }

        fclose(fres);
        free(file_path);

        char eid_value[100];
        int seats;
        int day, month, year;
        int hour, minute, second;

        // EID num_reserved_seats DD-MM-YYYY HH:MM:SS
        ///////////////A ideia aqui é que, na altura da reserva, guardamos no r-uid-date.txt esta informação toda
        ///////////////Em seats guardamos o numero de lugares que o user reservou
        if (sscanf(line, "%s %d %d-%d-%d %d:%d:%d",
                eid_value, &seats,
                &day, &month, &year,
                &hour, &minute, &second) == 8)
        {
            n_entries++;//Quantas reservations existem
            //printf("ADICIONAMOS\n");
            /* char *temp = NULL;
            asprintf(&temp, "%s %02d-%02d-%04d %02d:%02d:%02d %d ", eid_value, day, month, year, hour, minute, second, seats); */

            int needed_temp = snprintf(NULL, 0, "%s %02d-%02d-%04d %02d:%02d:%02d %d ", eid_value, day, month, year, hour, minute, second, seats);
            char *temp = malloc(needed_temp + 1);
            snprintf(temp, needed_temp + 1, "%s %02d-%02d-%04d %02d:%02d:%02d %d ", eid_value, day, month, year, hour, minute, second, seats);




            /* char *new_reply = NULL;
            asprintf(&new_reply, "%s%s", reply, temp); */

            int needed_reply = snprintf(NULL, 0, "%s%s", reply, temp);
            char *new_reply = malloc(needed_reply + 1);
            snprintf(new_reply, needed_reply + 1, "%s%s", reply, temp);

            free(reply);
            free(temp);
            //free(eid_value);
            reply = new_reply;
        }
    }

    if (n_entries <= 0){
        char* reply = "RMR NOK\n"; //Não existem reservas
        //printf("YQUEFUE\n");
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        free(reservation_path);
        closedir(dir);
        return;
    }
    /* asprintf(&reply, "%s\n", reply) */;

    int neede_final = snprintf(NULL, 0, "%s\n", reply);
    char *final_reply = malloc(neede_final + 1);
    snprintf(final_reply, neede_final + 1, "%s\n", reply);
    free(reply);
    reply = final_reply;
    sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
    free(reply);

    closedir(dir);
    free(reservation_path);
}

bool standard_checks(int udp_fd, struct sockaddr_in client_addr, socklen_t client_len, const char *uid,
     const char *password, const char *user_path, char *reply){

    if (dir_exists(user_path)){
        //Verificar se esta logged in
        /* char* login_path = NULL;
        asprintf(&login_path, "%s/%s_login.txt", user_path, uid); */

        int needed_login = snprintf(NULL, 0, "%s/%s_login.txt", user_path, uid);
        char *login_path = malloc(needed_login + 1);
        snprintf(login_path, needed_login + 1, "%s/%s_login.txt", user_path, uid);

        if (path_exists(login_path)) {
            /* char* pass_path = NULL;
            asprintf(&pass_path, "%s/%s_pass.txt", user_path , uid); */

            int needed_user = snprintf(NULL, 0, "%s/%s_pass.txt", user_path , uid);
            char *pass_path = malloc(needed_user + 1);
            snprintf(pass_path, needed_user + 1, "%s/%s_pass.txt", user_path , uid);

            FILE *f = fopen(pass_path, "r");
            if (f == NULL) {
                /* char *temp = NULL; 
                asprintf(&temp, "%sNOK\n", reply); //Erro ao abrir ficheiro */

                int needed_temp = snprintf(NULL, 0, "%sNOK\n", reply);
                char *temp = malloc(needed_temp + 1);
                snprintf(temp, needed_temp + 1, "%sNOK\n", reply);

                sendto(udp_fd, temp, strlen(temp), 0, (struct sockaddr*)&client_addr, client_len);
                free(temp);
                free(login_path);
                free(pass_path);
                fclose(f); 
                return false; 
            }
            char stored_password[100];
            fscanf(f, "%s", stored_password);
            fclose(f);
            free(pass_path);

            if (strcmp(stored_password, password) == 0) {
                free(login_path);
                return true;
            } 

            else {
                /* char *temp = NULL;
                asprintf(&temp, "%sWRP\n", reply); //Password errada */

                int needed_temp = snprintf(NULL, 0, "%sWRP\n", reply);
                char *temp = malloc(needed_temp + 1);
                snprintf(temp, needed_temp + 1, "%sWRP\n", reply);

                sendto(udp_fd, temp, strlen(temp), 0, (struct sockaddr*)&client_addr, client_len);
                free(temp);
                free(login_path);
                return false;
            }
        } 
        else {
            /* char *temp = NULL;
            asprintf(&temp, "%sNOK\n", reply); //Não está logged in */

            int needed_temp = snprintf(NULL, 0, "%sNOK\n", reply);
            char *temp = malloc(needed_temp + 1);
            snprintf(temp, needed_temp + 1, "%sNOK\n", reply);

            sendto(udp_fd, temp, strlen(temp), 0, (struct sockaddr*)&client_addr, client_len);
            free(temp);
            free(login_path);
            return false;
        }
    }

    else{//Usuario nunca foi registado 
        /* char *temp = NULL;
        asprintf(&temp, "%sUNR\n", reply); //Usuario não registado */

        int needed_temp = snprintf(NULL, 0, "%sUNR\n", reply);
        char *temp = malloc(needed_temp + 1);
        snprintf(temp, needed_temp + 1, "%sUNR\n", reply);

        sendto(udp_fd, temp, strlen(temp), 0, (struct sockaddr*)&client_addr, client_len);
        free(temp);
        return false;
    }
    
    return true;
}