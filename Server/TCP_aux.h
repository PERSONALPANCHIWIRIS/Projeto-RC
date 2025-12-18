#ifndef TCP_HANDLER_H
#define TCP_HANDLER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>

// Definições conforme enunciado
#define UID_SIZE 6
#define PWD_SIZE 8
#define EID_SIZE 3
#define E_NAME_SIZE 10
#define DATE_SIZE 16        // "DD-MM-YYYY HH:MM"
#define DATE_SIZE_FULL 19   // "DD-MM-YYYY HH:MM:SS"
#define F_NAME_SIZE 24
#define MAX_F_SIZE 10000000 // 10MB
#define BUFFER_SIZE 2048

// Estrutura auxiliar para metadados de eventos
typedef struct {
    char uid[UID_SIZE + 1];
    char name[E_NAME_SIZE + 1];
    char fname[F_NAME_SIZE + 1];
    char date[DATE_SIZE + 1]; 
    int att_size;
} EventMeta;

// --- Funções de Ajuda e Validação ---
int check_uid(const char *uid);
int check_pwd(const char *uid, const char *pwd);
int get_event_meta(const char *eid, EventMeta *meta);
int get_reservations_count(const char *eid);
int is_event_owner(const char *uid, const char *eid);

/**
 * Calcula o estado do evento com base na hierarquia:
 * 3 (Closed) > 0 (Past) > 2 (Sold Out) > 1 (Open)
 */
int get_event_state(const char *eid, const char *date_str, int max_seats, int reserved);
void get_current_datetime_str(char *buffer);

// --- Handlers TCP ---

/**
 * Criação de evento.
 * Recebe o socket e o buffer inicial já lido pelo servidor principal.
 * Processa o upload do ficheiro binário.
 */
int handle_cre(int conn_fd, char *header_buffer, int bytes_read);

/**
 * Fecha um evento (apenas pelo dono).
 */
int handle_cls(int conn_fd, const char *args);

/**
 * Lista todos os eventos.
 */
int handle_lst(int conn_fd);

/**
 * Envia os detalhes e o ficheiro de um evento (Download).
 */
int handle_sed(int conn_fd, const char *args);

/**
 * Realiza uma reserva.
 * Cria registos redundantes em EVENTS e USERS.
 */
int handle_rid(int conn_fd, const char *args);

/**
 * Altera a password do utilizador.
 */
int handle_cps(int conn_fd, const char *args);

#endif