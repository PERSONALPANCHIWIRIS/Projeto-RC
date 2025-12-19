#include "ES.h"
#include "UDP_aux.c"
#include "TCP_aux.c"

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

    int opt = 1;
    if (setsockopt(tcp_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_REUSEADDR");
        exit(1);
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
    if (verbose) printf("[UDP] Received: %s IP: %s\n Port: %d\n", buffer, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    char command[4];
    sscanf(buffer, "%3s", command);

        if (strcmp(command, "LIN") == 0) {
            //if (verbose) printf("[UDP] Received: %s", buffer);
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
            //char* reply = NULL; 
            //asprintf(&reply, "[UDP] Unknown command: %s\n", command);
            int needed = snprintf(NULL, 0, "[UDP] Unknown command: %s\n", command);
            char *reply = malloc(needed + 1);
            snprintf(reply, needed + 1, "[UDP] Unknown command: %s\n", command);
            sendto(udp_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_len);
            free(reply);
        }
}

//Tratar do TCP
void handle_tcp(int tcp_fd) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int connect_fd = accept(tcp_fd, (struct sockaddr*)&client_addr, &client_len);
    if (connect_fd < 0) return;

    char line[MAX_COMMAND];
    char cmd[MAX_CMD] = {0};
    

    ssize_t n = read(connect_fd, cmd, 3); //para identificar se é CRE ou outro
    if (n <= 0 || n != 3) { //Erro qualquer
        close(connect_fd);
        return;
    }

    cmd[3] = '\0';
    if (strncmp(cmd, "CRE", 3) == 0) {
        char uid[7], pass[9], name[11], date[11], time[6], fname[25];
        int attendance;
        ssize_t fsize;
        char header_part[512];
        int i = 0, spaces = 0;

        //Byte por byte
        while (spaces < 9 && i < (int)sizeof(header_part) - 1) {
            if (read(connect_fd, &header_part[i], 1) <= 0) break;
            if (header_part[i] == ' ') spaces++;
            i++;
        }
        header_part[i] = '\0';

        if (sscanf(header_part, "%s %s %s %s %s %d %s %zd", 
            uid, pass, name, date, time, &attendance, fname, &fsize) != 8) {
            const char* reply = "RCE ERR\n";
            write(connect_fd, reply, strlen(reply));
            close(connect_fd);
            return;
        }

        int u_status = check_uid(uid);
        if (u_status == -1) { send_reply(connect_fd, "RCE", "NLG", NULL); return; }
        if (u_status == -2) { send_reply(connect_fd, "RCE", "ERR", NULL); return; } 
        if (check_pwd(uid, pass) != 1) { send_reply(connect_fd, "RCE", "WRP", NULL); return; }
        if (fsize > MAX_F_SIZE || fsize < 0) { send_reply(connect_fd, "RCE", "NOK", NULL); return; }

        if (verbose) printf("[TCP] Received CRE: %s uid: %s, size: %zd\n", name, uid, fsize);

        // Criar Estrutura de Diretórios
        int eid = 1;
        char eid_str[4];
        char path[200];
        
        // Encontrar EID livre (001 a 999)
        while (eid <= 999) {
            snprintf(eid_str, sizeof(eid_str), "%03d", eid);
            snprintf(path, sizeof(path), "EVENTS/%s", eid_str);
            if (mkdir(path, 0700) == 0) break; 
            eid++;
        }
        if (eid > 999) { send_reply(connect_fd, "RCE", "NOK", NULL); return; }

        // Criar subdiretorias obrigatórias
        char path_desc[128], path_resv[128];
        snprintf(path_desc, sizeof(path_desc), "EVENTS/%s/DESCRIPTION", eid_str);
        snprintf(path_resv, sizeof(path_resv), "EVENTS/%s/RESERVATIONS", eid_str);
        mkdir(path_desc, 0700);
        mkdir(path_resv, 0700);

        char *file_buffer = malloc(fsize);
        if (!file_buffer) {
            send_reply(connect_fd, "RCE", "NOK", NULL);
            close(connect_fd);
            return;
        }

        read_tcp_fdata(connect_fd, file_buffer, fsize);

        snprintf(path, sizeof(path), "%s/%s", path_desc, fname);
        int fd_file = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
        
        if (fd_file < 0) { send_reply(connect_fd, "RCE", "NOK", NULL); return; }

        write(fd_file, file_buffer, fsize);
        close(fd_file);
        // Criar ficheiro START e atualizar diretoria do User
        snprintf(path, sizeof(path), "EVENTS/%s/START_%s.txt", eid_str, eid_str);
        FILE *fp = fopen(path, "w");
        if (fp) {
            fprintf(fp, "%s %s %s %d %s %s\n", uid, name, fname, attendance, date, time);
            fclose(fp);
        }

        // Criar ficheiro RES (Contador) a 0
        snprintf(path, sizeof(path), "EVENTS/%s/RES_%s.txt", eid_str, eid_str);
        fp = fopen(path, "w");
        if (fp) { fprintf(fp, "0\n"); fclose(fp); }

        // Adicionar ficheiro vazio na pasta CREATED do User para indexação rápida
        snprintf(path, sizeof(path), "USERS/%s/CREATED/%s.txt", uid, eid_str);
        fp = fopen(path, "w");
        //if (fp) fclose(fp); // Apenas criar o ficheiro
        if (fp) { //Podemos até guardar alguma coisa
            fprintf(fp, "%s %s %s %d %s %s\n", uid, name, fname, attendance, date, time);
            fclose(fp);
        }

        send_reply(connect_fd, "RCE", "OK", eid_str);
        close(connect_fd);
        return;
    }
        
    else{
        strncpy(line, cmd, 3);
        ssize_t n = read_line(connect_fd, line + 3, sizeof(line) - 3);

        if (n <= 0) {
            close(connect_fd);
            return;
        }

        if (verbose) printf("[TCP] Received: %s", line);

        //Parse da linha
        parse_tcp_command(line, connect_fd);

        close(connect_fd);
    }

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
        size_t space_left = maxlen - 1 - total;
        //char c;
        //ssize_t n = read(fd, &c, 1);
        ssize_t n = read(fd, buffer + total, space_left);
        

        if (n <= 0) {  //erro ou desligaram
            if (errno == EINTR) continue;   //retry
            return -1;
        }
        if (n == 0) {                       //EOF
            break;
        }

        // look for newline in the newly read block
        char *newline = memchr(buffer + total, '\n', n);
        if (newline) {
            size_t used = (newline - (buffer + total)) + 1; // include '\n'
            total += used;
            break;
        }

        total += n;
    }

    buffer[total] = '\0';
    return total;
}

ssize_t read_tcp_fdata(int fd, char* buffer, ssize_t fsize) {
    ssize_t total = 0;
    while (total < fsize) {
        ssize_t res = read(fd, buffer + total, fsize - total);
        //printf("Read %zd bytes, total so far %zd/%zd\n", res, total, fsize); // Debug
        if (res > 0) {
            total += res;
        } 

        else if (res == 0) {//Fechou cedo, ou antes já foi alguma metadata
            return total;  
        } 
        else {
            return -1; //erro
        }
    }
    return total;
}

void parse_tcp_command(char *line, int connect_fd) {
    //Le o comando, e executa a ação correspondente
    
    // Extrair command da mensagem
    char cmd[MAX_CMD] = {0};
    strncpy(cmd, line, 3);
    cmd[3] = '\0';

    /* if (strcmp(cmd, "CRE") == 0) {
        // create (criar evento)
        handle_cre(connect_fd, line, strlen(line)); 

    } */ 
    if (strcmp(cmd, "CLS") == 0) {
        // close (fechar evento)
        handle_cls( connect_fd, line + MAX_CMD);

    } 
    else if (strcmp(cmd, "LST") == 0) {
        // list (mostrar eventos)
        handle_lst( connect_fd);

    } 
    else if (strcmp(cmd, "SED") == 0) {
        // show (dar ficheiro de evento)
        //printf("[TCP] Handling SED command\n");
        handle_sed(connect_fd, line + MAX_CMD);

    }
    else if (strcmp(cmd, "RID") == 0) {
        // reserve (fazer reserva)
        handle_rid(connect_fd, line + MAX_CMD);

    }
    else if (strcmp(cmd, "CPS") == 0) {
        // changePass (mudar passe)
        handle_cps( connect_fd, line + MAX_CMD);

    }
    else {
        // Comando desconhecido
        const char *reply = "ERR\n";
        write(connect_fd, reply, strlen(reply));
    }
}
