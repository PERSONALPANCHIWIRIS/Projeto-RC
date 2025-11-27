#include "ES.h"

int main(int argc, char *argv[]) {
    int ESport = base_ESport; //Valor por defeito

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            ESport = atoi(argv[i + 1]);
        }
    }

    if (strcmp(argv[argc - 1], "-v") == 0) {
        verbose = true;
        printf("Verbose mode enabled\n");
    }

    printf("ES booted with port: %d\n", ESport);

    //Criamos os sockets
    //UDP - Sock_dgram
    int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_fd < 0) { 
        perror("UDP socket"); exit(1);
    }

    //TCP - Sock_stream
    int tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_fd < 0) {
        perror("TCP socket"); exit(1);
    }

    //Address para bind
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_port = htons(ESport); //A qual socket vão as conexoes
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    
    //Bind UDP
    if (bind(udp_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) { //Casting para sockaddr
        perror("bind UDP");
        exit(1);
    }

    //Bind TCP
    if (bind(tcp_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind TCP");
        exit(1);
    }

    //TCP Listen
    if (listen(tcp_fd, 10) < 0) { //Backlog - fila de espera - de 10
        perror("listen");
        exit(1);
    }

    fd_set all_sets; //Set de file descriptors
    //Encontramos o maximo de forma ao select abranger todos os fds possiveis
    int max_fd = (udp_fd > tcp_fd ? udp_fd : tcp_fd);

    while (1) {
        FD_ZERO(&all_sets); //Limpar set
        FD_SET(udp_fd, &all_sets); //Adicionar UDP
        FD_SET(tcp_fd, &all_sets); //Adicionar TCP

        //Vamos, ao inicio, só ler dos sockets
        int ready = select(max_fd + 1, &all_sets, NULL, NULL, NULL);
        if (ready < 0) {
            perror("select"); continue;
        }

        if (FD_ISSET(udp_fd, &all_sets)) { //Se UDP está pronto
            handle_udp(udp_fd);
        }

        if (FD_ISSET(tcp_fd, &all_sets)) { //Se TCP está pronto
            handle_tcp(tcp_fd);
        }
    }

    close(udp_fd);
    close(tcp_fd);
    return 0;
}

//Tratar da mensagem recebida UDP
void handle_udp(int udp_fd) {
    char buffer[MAX_BUFFER];
    memset(buffer, 0, MAX_BUFFER);
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int n = recvfrom(udp_fd, buffer, MAX_BUFFER - 1, 0,
                     (struct sockaddr*)&client_addr, &client_len);

    if (n < 0) {
        perror("recvfrom - UDP");
        return;
    }
    buffer[n] = '\0'; //recvfrom lê tudo de uma vez
    
    //IP e port do client.
    if (verbose) printf("[UDP] Received: %s, IP: %s\n, Port: %d\n", buffer, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    char command[4];
    sscanf(buffer, "%3s", command);

        if (strcmp(command, "LIN") == 0) {
            if (verbose) printf("[UDP] Received: %s", buffer);
            //chamar função login
            login_user(buffer, udp_fd, client_addr, client_len);
        }

        else if (strcmp(command, "LOU") == 0) {
            //chamar função logout
            //No lado do user já verifica se está logged in
            logout_user(buffer, udp_fd, client_addr, client_len);
        }

        else if (strcmp(command, "UNR") == 0) {
            //chamar função unregister
            unregister_user(buffer, udp_fd, client_addr, client_len);
        }

        else if (strcmp(command, "LME") == 0) {
            //chamar função myevents
            list_events(buffer, udp_fd, client_addr, client_len);
        }

        else if (strcmp(command, "LMR") == 0) {
            //chamar função myreservations
            list_reservations(buffer, udp_fd, client_addr, client_len);
        }

        else{
            char* reply = "[UDP] Unknown command: %s\n", command;
            sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        }
}

//Tratar do TCP
void handle_tcp(int tcp_fd) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int connect_fd = accept(tcp_fd, (struct sockaddr*)&client_addr, &client_len);
    if (connect_fd < 0) return;

    char line[MAX_COMMAND];
    ssize_t n = read_line(connect_fd, line, sizeof(line));

    if (n <= 0) {
        close(connect_fd);
        return;
    }

    if (verbose) printf("[TCP] Received: %s", line);

    //Parse da linha
    parse_tcp_command(line, connect_fd);

    close(connect_fd);

    // CRE -> create event
    // CLS -> close
    // LST -> list
    // SED -> show event
    // RID -> reserve
    // CPS -> changePassword

    //sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
}

ssize_t read_line(int fd, char *buffer, size_t maxlen) {
    size_t total = 0;

    while (total < maxlen - 1) {
        char c;
        ssize_t n = read(fd, &c, 1);

        if (n <= 0) {  //erro ou desligaram
            return -1;
        }

        buffer[total] = c;
        total++;

        if (c == '\n'){
            break;
        }
    }

    buffer[total] = '\0';
    return total;
}

void parse_tcp_command(const char *line, int connect_fd) {
    //Le o comando, e executa a ação correspondente
    //Deve utilizar read, write
    //Switch case -> strcmp nos primeiros 3 caracteres e chamamos funções especificas para cada caso
    //Se está no modo verbose, pritar sempre o que está a ser executado
    
    // Extrair command da mensagem
    char cmd[MAX_CMD] = {0};
    strncpy(cmd, line, 3);
    cmd[3] = '\0';

    if (strcmp(cmd, "CRE") == 0) {
        // create (criar evento)
        handle_cre(line + MAX_CMD, connect_fd); // Passar os argumentos sem o comando

    } else if (strcmp(cmd, "CLS") == 0) {
        // close (fechar evento)
        handle_cls(line + MAX_CMD, connect_fd);

    } else if (strcmp(cmd, "LST") == 0) {
        // list (mostrar eventos)
        handle_lst(line + MAX_CMD, connect_fd);

    } else if (strcmp(cmd, "SED") == 0) {
        // show (dar ficheiro de evento)
        handle_sed(line + MAX_CMD, connect_fd);

    } else if (strcmp(cmd, "RID") == 0) {
        // reserve (fazer reserva)
        handle_rid(line + MAX_CMD, connect_fd);

    } else if (strcmp(cmd, "CPS") == 0) {
        // changePass (mudar passe)
        handle_cps(line + MAX_CMD, connect_fd);

    } else {
        // Comando desconhecido
        const char *reply = "ERR\n";
        write(connect_fd, reply, strlen(reply));
    }
}

bool dir_exists(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return false;   
    return S_ISDIR(st.st_mode);                
}

//Utilizei asprintf, que não tenho a certeza se funciona nos PC's do Lab
void login_user(const char *buffer, int udp_fd, struct sockaddr_in client_addr, socklen_t client_len) {
    //Deve verificar se o uid existe, password correspondente e ver se o ficheiro login.txt existe
    char uid[100], password[100];
    sscanf(buffer + 4, "%s %s", uid, password);
    
    //Tira o path para a diretoria do ususario
    //char user_path[1024];
    char* user_path = NULL;
    //snprintf(user_path, sizeof(user_path), "USERS/%s", uid);
    asprintf(&user_path, "USERS/%s", uid);

    if (dir_exists(user_path)){
        //Abre o ficheiro da password
        //char pass_path[1024];
        //snprintf(pass_path, sizeof(pass_path), "%s/%s_pass.txt", user_path , uid);
        char* pass_path = NULL;
        asprintf(&pass_path, "%s/%s_pass.txt", user_path , uid);
        FILE *f = fopen(pass_path, "r");
        if (f == NULL) {
            char* reply = "RLI NOK\n"; //Erro ao abrir ficheiro
            sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
            free(user_path);
            return; 
        }
        char stored_password[100];
        fscanf(f, "%s", stored_password);
        fclose(f);
        free(pass_path);

        if (strcmp(stored_password, password) == 0) {
            //char login_path[1024];
            //snprintf(login_path, sizeof(login_path), "%s/%s_login.txt", user_path, uid);
            char* login_path = NULL;
            asprintf(&login_path, "%s/%s_login.txt", user_path, uid);
            if (dir_exists(login_path)) {
                char* reply = "RLI OK\n"; //Já está logged in
                sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
                free(login_path);
                free(user_path);
                return;
            } 
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
            char* reply = "RLI WRP\n"; //Password incorreta
            sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
            free(user_path);
            return;
        }
    }

    else {
        //Diretorias necessárias
        mkdir(user_path, 0700);
        //CREATED
        //char created_dir[1024];
        //snprintf(created_dir, sizeof(created_dir), "%s/CREATED", user_path);
        char* created_dir = NULL;
        asprintf(&created_dir, "%s/CREATED", user_path);
        mkdir(created_dir, 0700);
        //RESERVED
        //char reserved_dir[1024];
        //snprintf(reserved_dir, sizeof(reserved_dir), "%s/RESERVED", user_path);
        char* reserved_dir = NULL;
        asprintf(&reserved_dir, "%s/RESERVED", user_path);
        mkdir(reserved_dir, 0700);
        free(reserved_dir);
        free(created_dir);

        //Criamos ficheiro para a password
        //char pass_path[1024];
        //snprintf(pass_path, sizeof(pass_path), "%s/%s_pass.txt", user_path, uid);
        char* pass_path = NULL;
        asprintf(&pass_path, "%s/%s_pass.txt", user_path, uid);
        //Registamos a password
        FILE *f = fopen(pass_path, "w");
        fprintf(f, "%s\n", password);
        fclose(f);
        free(pass_path);

        //char login_path[1024];
        //snprintf(login_path, sizeof(login_path), "%s/%s_login.txt", user_path, uid);
        char* login_path = NULL;
        asprintf(&login_path, "%s/%s_login.txt", user_path, uid);

        FILE *lf = fopen(login_path, "w");
        fprintf(lf, "logged\n"); //Placeholder?
        fclose(lf);
        free(login_path);

        char* reply = "RLI REG\n"; //Foi registado
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len); 
        free(user_path);
        return;
    }     
}

void logout_user(const char *buffer, int udp_fd, struct sockaddr_in client_addr, socklen_t client_len){
    char uid[100], password[100];
    sscanf(buffer + 4, "%s %s", uid, password);
    
    char* user_path = NULL;
    asprintf(&user_path, "USERS/%s", uid);

    if (dir_exists(user_path)){
        //Verificar se esta logged in
        char* login_path = NULL;
        asprintf(&login_path, "%s/%s_login.txt", user_path, uid);

        if (dir_exists(login_path)) {
            char* pass_path = NULL;
            asprintf(&pass_path, "%s/%s_pass.txt", user_path , uid);

            FILE *f = fopen(pass_path, "r");
            if (f == NULL) {
                char* reply = "RLO NOK\n"; //Erro ao abrir ficheiro
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

            if (strcmp(stored_password, password) == 0) {
                //Remover o ficheiro de login
                remove(login_path);
                char* reply = "RLO OK\n"; //Logout com sucesso
                sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len); 
                free(login_path);
                free(user_path);
                return;
            } 
            else {
                char* reply = "RLO WRP\n"; //Password incorreta
                sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
                free(login_path);
                free(user_path);
                return;
            }
        } 
        else {
            char* reply = "RLO NOK\n"; //Não está logged in
            sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
            free(login_path);
            free(user_path);
            return;
        }
    }

    else{//Usuario nunca foi registado 
        char* reply = "RLI UNR\n"; 
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        free(user_path);
        return;
    }
}

bool dir_exists(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return false;   
    return S_ISDIR(st.st_mode);                
}

//Utilizei asprintf, que não tenho a certeza se funciona nos PC's do Lab
void login_user(const char *buffer, int udp_fd, struct sockaddr_in client_addr, socklen_t client_len) {
    char uid[100], password[100];
    sscanf(buffer + 4, "%s %s", uid, password);

    if (!uid_is_valid(uid)){
        char* reply = "RLI NOK\n"; //UID inválido
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        return;
    }
    
    //Tira o path para a diretoria do ususario
    char* user_path = NULL;
    asprintf(&user_path, "USERS/%s", uid);
    char* pass_path = NULL;
    asprintf(&pass_path, "%s/%s_pass.txt", user_path , uid);
    char* login_path = NULL;
    asprintf(&login_path, "%s/%s_login.txt", user_path, uid);

    if (dir_exists(login_path)) {
        char* reply = "RLI OK\n"; //Já está logged in
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        free(login_path);
        free(user_path);
        free(pass_path);
        return;
    } 

    if (dir_exists(user_path)) {//Ja está registado
        if (!dir_exists(pass_path)) {
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
            char* reply = "RLI WRP\n"; //Password incorreta
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
        char* reply = "RLO NOK\n"; //UID inválido
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        return;
    }
    
    char* user_path = NULL;
    asprintf(&user_path, "USERS/%s", uid);

    if (dir_exists(user_path)){
        //Verificar se esta logged in
        char* login_path = NULL;
        asprintf(&login_path, "%s/%s_login.txt", user_path, uid);

        if (dir_exists(login_path)) {
            char* pass_path = NULL;
            asprintf(&pass_path, "%s/%s_pass.txt", user_path , uid);

            FILE *f = fopen(pass_path, "r");
            if (f == NULL) {
                char* reply = "RLO NOK\n"; //Erro ao abrir ficheiro
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

            if (strcmp(stored_password, password) == 0) {
                //Remover o ficheiro de login
                //remove(login_path);
                unlink(login_path);
                char* reply = "RLO OK\n"; //Logout com sucesso
                sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len); 
                free(login_path);
                free(user_path);
                return;
            } 
            else {
                char* reply = "RLO WRP\n"; //Password incorreta
                sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
                free(login_path);
                free(user_path);
                return;
            }
        } 
        else {
            char* reply = "RLO NOK\n"; //Não está logged in
            sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
            free(login_path);
            free(user_path);
            return;
        }
    }

    else{//Usuario nunca foi registado 
        char* reply = "RLO UNR\n"; 
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        free(user_path);
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
    
    char* user_path = NULL;
    asprintf(&user_path, "USERS/%s", uid);

    if (dir_exists(user_path)){//Registado
        char* login_path = NULL;
        asprintf(&login_path, "%s/%s_login.txt", user_path, uid);

        if (dir_exists(login_path)){//Logged in
            char* pass_path = NULL;
            asprintf(&pass_path, "%s/%s_pass.txt", user_path , uid);
            FILE *f = fopen(pass_path, "r");

            if (f == NULL) {
                char* reply = "RUR NOK\n"; //Erro ao abrir ficheiro
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
            //free(pass_path);

            if (strcmp(stored_password, password) == 0) {
                //Remover SÓ login.txt e pass.txt
                char command[1500];
                unlink(login_path);
                unlink(pass_path);

                char* reply = "RUR OK\n"; //Unregister com sucesso
                sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len); 
                free(login_path);
                free(user_path);
                free(pass_path);
                return;
            }

            else { //Password incorreta
                char* reply = "RUR WRP\n"; 
                sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
                free(login_path);
                free(user_path);
                free(pass_path);
                return;
            }
        }

        else { //Não estava logged in
            char* reply = "RUR NOK\n"; 
            sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
            free(user_path);
            free(login_path);
            return;
        }
    }

    else{//Usuario nunca foi registado 
        char* reply = "RUR UNR\n"; 
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        free(user_path);
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
    
    char* user_path = NULL;
    asprintf(&user_path, "USERS/%s", uid);
    if (!dir_exists(user_path)){
        char* reply = "RME NOK\n"; //Usuario não registado
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        free(user_path);
        return;
    }

    char* login_path = NULL;
    asprintf(&login_path, "%s/%s_login.txt", user_path, uid);
    if (!dir_exists(login_path)){
        char* reply = "RME NLG\n"; //Usuario não está logged in
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        free(user_path);
        free(login_path);
        return;
    }

    char* pass_path = NULL;
    asprintf(&pass_path, "%s/%s_pass.txt", user_path , uid);
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

    char* created_dir = NULL;
    asprintf(&created_dir, "%s/CREATED", user_path);
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

        char* event_path = NULL;
        asprintf(&event_path, "EVENTS/%s", eid);


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
    char* created_dir = NULL;
    asprintf(&created_dir, "%s/CREATED", user_path);
    mkdir(created_dir, 0700);
    //RESERVED
    char* reserved_dir = NULL;
    asprintf(&reserved_dir, "%s/RESERVED", user_path);
    mkdir(reserved_dir, 0700);
    free(reserved_dir);
    free(created_dir);

    //Criamos ficheiro para a password
    char* pass_path = NULL;
    asprintf(&pass_path, "%s/%s_pass.txt", user_path, uid);
    //Registamos a password
    FILE *f = fopen(pass_path, "w");
    fprintf(f, "%s\n", password);
    fclose(f);
    free(pass_path);

    char* login_path = NULL;
    asprintf(&login_path, "%s/%s_login.txt", user_path, uid);

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

void status_events(struct event_list *events, int udp_fd, struct sockaddr_in client_addr, socklen_t client_len){
    //Resposta a ser enviada
    char *reply = NULL;
    asprintf(&reply, "RME OK ");

    struct event_list *current = events;
    while (current != NULL) {
        //Abre a diretoria do evento
        DIR *dir = opendir(current->event_path);
        if (dir == NULL) {//Caso ocorra qualquer erro
            current = current->next;
            continue;
        }
        char *end_path = NULL;
        asprintf(&end_path, "END_%s.txt", current->eid);
        if (dir_exists(end_path)) {
            //Evento fechado
            strcat(reply, "3 ");
            free(end_path);
            continue; 
        } 
        free(end_path);

        char *start_path = NULL;
        asprintf(&start_path, "START_%s.txt", current->eid);
        FILE *fstart = fopen(start_path, "r");
        char *start_content = NULL;
        fprintf(fstart, "%s", start_content);
        fclose(fstart);
        free(start_path);

        //Tirar os dados do start_content
        int event_attend;
        int start_day, start_month, start_year;
        int star_hour, start_minute;
        parse_start_content(start_content, &event_attend, &start_day, &start_month, &start_year, &star_hour, &start_minute);
        free(start_content);
        if (check_event_date(start_day, start_month, start_year, star_hour, start_minute)) {
            //Evento futuro
            char *reservations_path = NULL;
            asprintf(&reservations_path, "%s/RESERVATIONS", current->event_path);
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
                strcat(reply, "2 ");
            }
            else {
                //Evento com vagas
                strcat(reply, "1 ");
            }
        }
        else {
            //Evento passado
            strcat(reply, "0 ");
            continue;
        }
    }
    sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);

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

    char *user_path = NULL;
    asprintf(&user_path, "USERS/%s", uid);
    if (!dir_exists(user_path)){
        char* reply = "RMR NOK\n"; //Usuario não registado
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        free(user_path);
        return;
    }

    char *login_path = NULL;
    asprintf(&login_path, "%s/%s_login.txt", user_path, uid);
    if (!dir_exists(login_path)){
        char* reply = "RMR NLG\n"; //Usuario não está logged in
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        free(user_path);
        free(login_path);
        return;
    }

    char *pass_path = NULL;
    asprintf(&pass_path, "%s/%s_pass.txt", user_path , uid);
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
    char *reservation_path = NULL;
    asprintf(&reservation_path, "USERS/%s/RESERVED", uid);
    DIR *dir = opendir(reservation_path);
    struct dirent *entry;
    int n_entries = 0;

    char *reply = NULL;
    asprintf(&reply, "RMR OK ");
    while ((entry = readdir(dir)) != NULL) {
        //Ignorar . e ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char *file_path = NULL;
        asprintf(&file_path, "%s/%s", reservation_path, entry->d_name);

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

        int eid_value, seats;
        int day, month, year;
        int hour, minute;

        // EID num_reserved_seats DD-MM-YYYY HH:MM
        ///////////////A ideia aqui é que, na altura da reserva, guardamos no r-uid-date.txt esta informação toda
        if (sscanf(line, "%d %d %d-%d-%d %d:%d",
                &eid_value, &seats,
                &day, &month, &year,
                &hour, &minute) == 7)
        {
            n_entries++;//Quantas reservations existem
            char *temp = NULL;
            asprintf(&temp, "%d %02d-%02d-%04d %02d:%02d %d ", eid_value, day, month, year, hour, minute, seats);
            strcat(reply, temp);  
            free(temp);
        }
    }

    if (n_entries <= 0){
        char* reply = "RMR NOK\n"; //Não existem reservas
        sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
        free(reservation_path);
        closedir(dir);
        return;
    }

    closedir(dir);
    free(reservation_path);
}