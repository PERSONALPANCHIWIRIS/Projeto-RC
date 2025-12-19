#include "TCP_aux.h"

// ============================================================================
// FUNÇÕES AUXILIARES
// ============================================================================

//Envia resposta formatada para o cliente, garantindo escrita completa
void send_reply(int conn_fd, const char *head, const char *status, const char *body) {
    char buffer[BUFFER_SIZE];
    if (body) {
        snprintf(buffer, sizeof(buffer), "%s %s %s\n", head, status, body);
    } else {
        snprintf(buffer, sizeof(buffer), "%s %s\n", head, status);
    }
    
    size_t len = strlen(buffer);
    size_t written = 0;
    while (written < len) {
        ssize_t n = write(conn_fd, buffer + written, len - written);
        if (n <= 0) break;
        written += n;
    }
}

// Verifica se UID existe e se está logado (procura ficheiro _login.txt)
// Retorno: 1 (OK), -1 (Não logado), -2 (Não existe)
int check_uid(const char *uid) {
    char path[64];
    snprintf(path, sizeof(path), "Server/USERS/%s", uid);
    if (!dir_exists(path)) return -2; 

    snprintf(path, sizeof(path), "Server/USERS/%s/%s_login.txt", uid, uid);
    if (access(path, F_OK) != 0) return -1;

    return 1;
}

int check_pwd(const char *uid, const char *pwd) {
    char path[64];
    char file_pwd[32];
    FILE *fp;

    snprintf(path, sizeof(path), "Server/USERS/%s/%s_pass.txt", uid, uid);
    fp = fopen(path, "r");
    if (!fp) return 0;
    
    if (fgets(file_pwd, sizeof(file_pwd), fp)) {
        file_pwd[strcspn(file_pwd, "\n")] = 0;
        file_pwd[strcspn(file_pwd, "\r")] = 0;
    }
    fclose(fp);

    if (strcmp(file_pwd, pwd) == 0) return 1;
    return -1; 
}

// Lê metadados essenciais do ficheiro START
int get_event_meta(const char *eid, EventMeta *meta) {
    char path[64];
    char date_part[16], time_part[8];
    FILE *fp;

    snprintf(path, sizeof(path), "Server/EVENTS/%s/START_%s.txt", eid, eid);
    fp = fopen(path, "r");
    if (!fp) {
        printf("Failed to open event meta file %s\n", path);
        return 0;
    }

    // Formato: UID name fname att_size date time
    if (fscanf(fp, "%s %s %s %d %s %s", 
        meta->uid, meta->name, meta->fname, &meta->att_size, date_part, time_part) != 6) {
        fclose(fp);
        printf("Failed to read event meta from %s\n", path);
        return 0;
    }
    fclose(fp);
    snprintf(meta->date, sizeof(meta->date), "%.10s %.5s", date_part, time_part);
    return 1;
}

// Obtém total de reservas do ficheiro RES
int get_reservations_count(const char *eid) {
    char path[64];
    int count = 0;
    FILE *fp;

    snprintf(path, sizeof(path), "Server/EVENTS/%s/RES_%s.txt", eid, eid);
    fp = fopen(path, "r");
    if (!fp) return 0;
    fscanf(fp, "%d", &count);
    fclose(fp);
    return count;
}

int is_event_owner(const char *uid, const char *eid) {
    EventMeta meta;
    if (!get_event_meta(eid, &meta)) return 0;
    return (strcmp(meta.uid, uid) == 0);
}

// Lógica de Prioridade de Estados 
int get_event_state(const char *eid, const char *date_str, int max_seats, int reserved) {
    char path[64];
    
    // 1. Prioridade Máxima: Evento encerrado explicitamente (START existe, mas END também)
    snprintf(path, sizeof(path), "Server/EVENTS/%s/END_%s.txt", eid, eid);
    if (access(path, F_OK) == 0) return 3;

    // 2. Data no Passado
    struct tm tm_evt = {0};
    time_t t_now;
    int d, m, y, H, M;
    
    sscanf(date_str, "%d-%d-%d %d:%d", &d, &m, &y, &H, &M);
    tm_evt.tm_mday = d; tm_evt.tm_mon = m - 1; tm_evt.tm_year = y - 1900;
    tm_evt.tm_hour = H; tm_evt.tm_min = M;
    tm_evt.tm_isdst = -1; // Deixar o sistema determinar horário de verão
    
    time_t t_evt = mktime(&tm_evt);
    time(&t_now);
    
    if (difftime(t_evt, t_now) < 0) return 0;

    // 3. Esgotado (Futuro mas cheio)
    if (reserved >= max_seats) return 2;

    // 4. Aberto
    return 1;
}

void get_current_datetime_str(char *buffer) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    sprintf(buffer, "%04d%02d%02d_%02d%02d%02d", 
        tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
        tm->tm_hour, tm->tm_min, tm->tm_sec);
}

// ============================================================================
// HANDLERS TCP
// ============================================================================

// --- CLOSE EVENT (CLS) ---
int handle_cls(int conn_fd, const char *args) {
    char uid[UID_SIZE+1], pwd[PWD_SIZE+1], eid[EID_SIZE+1];
    
    if (sscanf(args, "%s %s %s", uid, pwd, eid) != 3) {
        send_reply(conn_fd, "RCL", "ERR", NULL); return 0;
    }

    int u_stat = check_uid(uid);
    if (u_stat == -1) { send_reply(conn_fd, "RCL", "NLG", NULL); return 0; }
    if (u_stat == -2) { send_reply(conn_fd, "RCL", "NOK", NULL); return 0; }
    if (check_pwd(uid, pwd) != 1) { send_reply(conn_fd, "RCL", "NOK", NULL); return 0; }

    char path[128];
    snprintf(path, sizeof(path), "Server/EVENTS/%s", eid);
    if (!dir_exists(path)) { send_reply(conn_fd, "RCL", "NOE", NULL); return 0; }

    if (!is_event_owner(uid, eid)) { send_reply(conn_fd, "RCL", "EOW", NULL); return 0; }

    // Verificar Estado Atual
    EventMeta meta;
    get_event_meta(eid, &meta);
    int reserved = get_reservations_count(eid);
    int state = get_event_state(eid, meta.date, meta.att_size, reserved);

    if (state == 3) { send_reply(conn_fd, "RCL", "CLO", NULL); return 0; }
    if (state == 0) { send_reply(conn_fd, "RCL", "PST", NULL); return 0; }
    if (state == 2) { send_reply(conn_fd, "RCL", "SLD", NULL); return 0; }

    // Fechar Evento (Criar END file com timestamp atual)
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    snprintf(path, sizeof(path), "Server/EVENTS/%s/END_%s.txt", eid, eid);
    
    FILE *fp = fopen(path, "w");
    if (fp) {
        fprintf(fp, "%02d-%02d-%04d %02d:%02d:%02d\n", 
            tm->tm_mday, tm->tm_mon+1, tm->tm_year+1900, tm->tm_hour, tm->tm_min, tm->tm_sec);
        fclose(fp);
        send_reply(conn_fd, "RCL", "OK", NULL);
    } else {
        send_reply(conn_fd, "RCL", "NOK", NULL);
    }
    return 1;
}

// --- LIST EVENTS (LST) ---
int handle_lst(int conn_fd) {
    DIR *d;
    struct dirent *dir;
    d = opendir("Server/EVENTS");
    if (!d) {
        send_reply(conn_fd, "RLS", "NOK", NULL);
        return 0;
    }

    // Protocolo: RLS OK [EID name state date]*
    char resp_buf[BUFFER_SIZE];
    int offset = snprintf(resp_buf, sizeof(resp_buf), "RLS OK");
    int has_events = 0;

    while ((dir = readdir(d)) != NULL) {
        if (dir->d_name[0] == '.') continue;
        if (strlen(dir->d_name) != 3) continue; // Filtra apenas diretorias EID (3 dígitos)

        char *eid = dir->d_name;
        EventMeta meta;
        if (!get_event_meta(eid, &meta)) continue;

        int res = get_reservations_count(eid);
        int state = get_event_state(eid, meta.date, meta.att_size, res);
        
        has_events = 1;
        
        // Formatar entrada: " EID name state date time"
        char entry[128];
        snprintf(entry, sizeof(entry), " %.6s %s %d %s", eid, meta.name, state, meta.date);
        
        // Buffer management: se encher, envia e limpa
        if (offset + strlen(entry) + 2 >= sizeof(resp_buf)) {
            write(conn_fd, resp_buf, offset);
            offset = 0;
        }
        strcat(resp_buf + offset, entry);
        offset += strlen(entry);
    }
    closedir(d);

    if (!has_events) {
        send_reply(conn_fd, "RLS", "NOK", NULL);
    } else {
        strcat(resp_buf + offset, "\n");
        write(conn_fd, resp_buf, offset + 1);
    }
    return 1;
}

// --- SHOW EVENT DETAILS (SED) ---
int handle_sed(int conn_fd, const char *args) {
    char eid[EID_SIZE+1];
    if (sscanf(args, "%s", eid) != 1) { send_reply(conn_fd, "RSE", "ERR", NULL); return 0; }

    char path[64]; snprintf(path, sizeof(path), "Server/EVENTS/%s", eid);
    if (!dir_exists(path)) { send_reply(conn_fd, "RSE", "NOK", NULL); return 0; }

    EventMeta meta;
    if (!get_event_meta(eid, &meta)) { send_reply(conn_fd, "RSE", "NOK", NULL); return 0; }

    int reserved = get_reservations_count(eid);
    
    // Obter tamanho do ficheiro
    char fpath[128];
    snprintf(fpath, sizeof(fpath), "Server/EVENTS/%s/DESCRIPTION/%s", eid, meta.fname);
    struct stat st;
    if (stat(fpath, &st) != 0) { send_reply(conn_fd, "RSE", "NOK", NULL); return 0; }
    int fsize = st.st_size;

    char header[512];
    snprintf(header, sizeof(header), "RSE OK %s %s %s %d %d %s %d ", 
        meta.uid, meta.name, meta.date, meta.att_size, reserved, meta.fname, fsize);
    
    write(conn_fd, header, strlen(header));

    // Enviar conteúdo do ficheiro
    int fd = open(fpath, O_RDONLY);
    if (fd >= 0) {
        char buf[BUFFER_SIZE];
        ssize_t n;
        while ((n = read(fd, buf, sizeof(buf))) > 0) {
            write(conn_fd, buf, n);
        }
        close(fd);
    }
    write(conn_fd, "\n", 1);
    return 1;
}

// --- RESERVE (RID) ---
int handle_rid(int conn_fd, const char *args) {
    char uid[UID_SIZE+1], pwd[PWD_SIZE+1], eid[EID_SIZE+1];
    int places;

    if (sscanf(args, "%s %s %s %d", uid, pwd, eid, &places) != 4) {
        send_reply(conn_fd, "RRI", "ERR", NULL); return 0;
    }

    int u_stat = check_uid(uid);
    if (u_stat == -1) { send_reply(conn_fd, "RRI", "NLG", NULL); return 0; }
    if (u_stat == -2) { send_reply(conn_fd, "RRI", "NOK", NULL); return 0; }
    if (check_pwd(uid, pwd) != 1) { send_reply(conn_fd, "RRI", "WRP", NULL); return 0; }

    char path[128];
    snprintf(path, sizeof(path), "Server/EVENTS/%s", eid);
    if (!dir_exists(path)) { send_reply(conn_fd, "RRI", "NOK", NULL); return 0; }

    EventMeta meta;
    get_event_meta(eid, &meta);
    int reserved = get_reservations_count(eid);
    
    // Verificar Estado para aceitar/rejeitar reserva
    int state = get_event_state(eid, meta.date, meta.att_size, reserved);

    if (state == 3) { send_reply(conn_fd, "RRI", "CLS", NULL); return 0; }
    if (state == 0) { send_reply(conn_fd, "RRI", "PST", NULL); return 0; }
    if (state == 2) { send_reply(conn_fd, "RRI", "SLD", NULL); return 0; }

    // Disponibilidade de lugares
    if (reserved + places > meta.att_size) {
        char avail_str[10];
        snprintf(avail_str, sizeof(avail_str), "%d", meta.att_size - reserved);
        send_reply(conn_fd, "RRI", "REJ", avail_str);
        return 0;
    }

    // 1. Atualizar contador RES
    reserved += places;
    snprintf(path, sizeof(path), "Server/EVENTS/%s/RES_%s.txt", eid, eid);
    FILE *fp = fopen(path, "w");
    if (fp) { fprintf(fp, "%d\n", reserved); fclose(fp); }

    // 2. Preparar dados para ficheiros de reserva
    char timestamp_fn[32]; // Para nome de ficheiro
    get_current_datetime_str(timestamp_fn);
    
    // Data legível para conteúdo
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date_content[32];
    snprintf(date_content, sizeof(date_content), "%02d-%02d-%04d %02d:%02d:%02d", 
        (unsigned)tm->tm_mday, (unsigned)tm->tm_mon+1, (unsigned)tm->tm_year+1900, (unsigned)tm->tm_hour, (unsigned)tm->tm_min, (unsigned)tm->tm_sec);

    // 3. Ficheiro em EVENTS (Formato: UID places date)
    // Nome: R-<uid>-<timestamp>.txt
    char fname[64];
    snprintf(fname, sizeof(fname), "R-%s-%s.txt", uid, timestamp_fn);
    
    snprintf(path, sizeof(path), "Server/EVENTS/%s/RESERVATIONS/%s", eid, fname);
    fp = fopen(path, "w");
    if (fp) { fprintf(fp, "%s %d %s\n", uid, places, date_content); fclose(fp); }

    // 4. Ficheiro em USERS (Formato Modificado: EID places date)
    // Isto permite que o comando 'myreservations' (UDP) funcione sem abrir ficheiros aleatórios.
    // O EID é guardado como primeira string para fácil leitura.
    snprintf(path, sizeof(path), "Server/USERS/%s/RESERVED", uid);
    mkdir(path, 0700); // Garante que pasta existe
    
    snprintf(path, sizeof(path), "Server/USERS/%s/RESERVED/%s", uid, fname);
    fp = fopen(path, "w");
    if (fp) { 
        // AQUI ESTÁ A CORREÇÃO CRUCIAL: Guardamos EID em vez de UID
        fprintf(fp, "%s %d %s\n", eid, places, date_content); 
        fclose(fp); 
    }

    send_reply(conn_fd, "RRI", "ACC", NULL);
    return 1;
}

// --- CHANGE PASSWORD (CPS) ---
int handle_cps(int conn_fd, const char *args) { 
    char uid[UID_SIZE+1], old_p[PWD_SIZE+1], new_p[PWD_SIZE+1];
    if (sscanf(args, "%s %s %s", uid, old_p, new_p) != 3) {
        send_reply(conn_fd, "RCP", "ERR", NULL); return 0;
    }

    int u_stat = check_uid(uid);
    if (u_stat == -2) { send_reply(conn_fd, "RCP", "NID", NULL); return 0; }
    if (u_stat == -1) { send_reply(conn_fd, "RCP", "NLG", NULL); return 0; }
    
    if (check_pwd(uid, old_p) != 1) { send_reply(conn_fd, "RCP", "NOK", NULL); return 0; }

    char path[64];
    snprintf(path, sizeof(path), "Server/USERS/%s/%s_pass.txt", uid, uid);
    FILE *fp = fopen(path, "w");
    if (fp) {
        fprintf(fp, "%s\n", new_p);
        fclose(fp);
        send_reply(conn_fd, "RCP", "OK", NULL);
    } else {
        send_reply(conn_fd, "RCP", "ERR", NULL);
    }
    return 1;
}