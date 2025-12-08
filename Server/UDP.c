#include "UDP.h"
#include "UDP_aux.c"

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
    ssize_t n = read_line(connect_fd, line, sizeof(line));

    if (n <= 0) {
        close(connect_fd);
        return;
    }

    if (verbose) printf("[TCP] Received: %s", line);

    //Parse da linah
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
}

