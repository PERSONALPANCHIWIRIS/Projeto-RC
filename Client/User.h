 #include <chrono>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <vector>

using namespace std;
#define SERVER_PORT "58038"
#define BUFFER_SIZE 1024
#define MAX_SIZE  65535//tamanho máximo de uma mensagem UDP
#define UID_SIZE 6

/*verify_alphanumeric(string str)
Verifica se uma string é consítuida
por apenas elementos alfanuméricos*/
int verify_alphanumeric(string str){
    for(int i=0; i<(int) str.size(); i++){
        if(!isalnum(str[i])){
            return 0;
        }
    }
    return 1;
}

/*verify_alphanumeric(string str)
Verifica se uma string é consítuida
por apenas elementos alfanuméricos, (_) ou (-)*/
int verify_alphanumeric_name(string str){
    for(int i=0; i<(int) str.size(); i++){
        if(!isalnum(str[i]) && str[i]!='_' && str[i]!='-'){
            return 0;
        }
    }
    return 1;
}

/*verify_numeric(string str)
Verifica se uma string é constítuida
por apenas elementos numéricos*/
int verify_numeric(string str){
    for(int i=0; i<(int)str.size(); i++){
        if(!isdigit(str[i])){
            return 0;
        }
    }
    return 1;
}
/*verify_date(string str)
Verifica se uma string está no formato dd-mm-yyyy hh:mm   FIX ME IF NEEDED: verify_date(const const char* date)*/
int verify_date(string str){
    // check  size for "dd-mm-yyyy hh:mm" -> 16 chars
    if(str.size() != 16) return 0;

    // digits in their positions
    if(!isdigit(str[0]) || !isdigit(str[1]) ||
       !isdigit(str[3]) || !isdigit(str[4]) ||
       !isdigit(str[6]) || !isdigit(str[7]) ||
       !isdigit(str[8]) || !isdigit(str[9]) ||
       !isdigit(str[11]) || !isdigit(str[12]) ||
       !isdigit(str[14]) || !isdigit(str[15])) {
        return 0;
    }

    // separators at fixed positions
    if(str[2] != '-' || str[5] != '-' || str[10] != ' ' || str[13] != ':') return 0;

    // parse numeric fields (safe because we checked digits)
    int day = stoi(str.substr(0,2));
    int month = stoi(str.substr(3,2));
    int year = stoi(str.substr(6,4));
    int hour = stoi(str.substr(11,2));
    int minute = stoi(str.substr(14,2));

    if(year < 1 || month < 1 || month > 12 || hour < 0 || hour > 23 || minute < 0 || minute > 59)
        return 0;

    int mdays = 31;
    if(month == 4 || month == 6 || month == 9 || month == 11) mdays = 30;
    else if(month == 2){
        bool leap = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
        mdays = leap ? 29 : 28;
    }

    if(day < 1 || day > mdays) return 0;

    return 1;
}

/*verify_filename(string str)*/
int verify_filename(string str){
    for(int i=0; i<(int)str.size(); i++){
        //verificar se é alfanumérico ou '.' ou '_ ' ou '-'
        if(!isalnum(str[i]) && str[i]!='.' && str[i]!='_' && str[i]!='-'){
            return 0;
        }
    }
    //verificar se tem '.' e 3 caracteres alfanuméricos no fim
    int end=str.size()-1;
    if(str[end-3]=='.' && isalnum(str[end-2]) && isalnum(str[end-1]) && isalnum(str[end])){
        return 1;
    }
    return 0;
}

//classe que implementa as conecções do User ao servidor
class Networking{
public:
    Networking(string host, int port){
        this->host = host;
        this->port = port;
    }
protected:
    string host;
    int port;
};

//classe que implementa as conecções do User ao servidor do tipo UDP
class UDPuser : public Networking{
public:
    UDPuser(string host, int port) : Networking(host, port) {}
    string connect(string message){

        //Criação do socket UDP
        int fd = socket(AF_INET, SOCK_DGRAM, 0);
        //Verificação de erros na criação do socket
        if (fd < 0) {
            cout << "->Error creating socket..." << endl;
            return "error";
        }

        //Configuração do endereço do servidor destino
        struct addrinfo hints, *res;        //Estrutura de configuração do endereço
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET;          //IPv4
        hints.ai_socktype = SOCK_DGRAM;     //UDP socket

        //Obtenção de informação sobre o servidor destino
        int errcode = getaddrinfo(host.c_str(), to_string(port).c_str(), &hints, &res);
        //Verificação de erros na obtenção de informação sobre o servidor destino
        if (errcode != 0) {
            cout << "->Error getting address info: " << gai_strerror(errcode) << endl;
            close(fd);
            return "error";
        }
        //Configura um timeout para o socket
        timeval tv; //timeout
        tv.tv_sec = 5; //5 segundos
        tv.tv_usec = 0;

        //Configuração do timeout para o socket receção
        if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {//SO_RCVTIMEO
            perror("->Erro ao configurar o timeout");
            close(fd);
            return "error";
        }

        //Envio de mensagem para o servidor destino(string message)
        ssize_t sent = sendto(fd, message.c_str(), message.size(), 0, res->ai_addr, res->ai_addrlen);
        //Verificação de erros no envio de mensagem
        if (sent == -1) {
            cout << "->Error sending message" << endl;

            return "error";
        }
        
        //Receção de mensagem do servidor destino
        struct sockaddr_in addr;
        
        socklen_t addrlen = sizeof (addr);
        char buffer[MAX_SIZE];
        memset(buffer, 0, MAX_SIZE);

        string cmd="";
        //Receção de mensagem do servidor destino
        ssize_t received = recvfrom(fd, buffer, MAX_SIZE, 0, (struct sockaddr *) &addr, &addrlen);
        //Verificação de erros na receção de mensagem(Timeout se flags EAGAIN ou EWOULDBLOCK forem ativadas)
        if (received == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK){//Timeout atingido
                cout << "->Error: timeout" << endl;
                close(fd);
                return "error";    
            }
            else{//Erro na receção de mensagem
                cout << "->Error receiving message" << endl;
                close(fd);
                return "error"; 
            }
        }
        for (int i = 0; i < MAX_SIZE; i++) {
            cmd+=buffer[i];
        }
        //se a mensagem recebida não tiver o \n no fim é inválida
        if (cmd.find("\n") == string::npos) {
            cout << "->Error: invalid message" << endl;
            close(fd);
            return "error";
        }
        // remover o texto recebido que não faz parte da resposta até ao \n
        ssize_t linepos = cmd.find("\n");
        string response = cmd.substr(0, linepos + 1);
        cout << "->UDP connection established!" << endl;
        close(fd);
        return response;
    }
};

//classe que implementa as conecções do User ao servidor do tipo TCP
class TCPuser : public Networking {
public:
    TCPuser(string host, int port) : Networking(host, port) {}
    //função que estabelece a conecção TCP com o servidor destino e envia uma 
    //mensagem e recebe uma resposta que termina em \n
    string server_connect(string message){
        
        //Criação do socket TCP
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            cout << "->Error creating socket" << std::endl;
            return "error";
        }

        struct addrinfo hints, *res;
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET;          //IPv4
        hints.ai_socktype = SOCK_STREAM;    //TCP socket

        //Obtenção de informação sobre o servidor destino
        int errcode = getaddrinfo(host.c_str(), to_string(port).c_str(), &hints, &res);
        if (errcode != 0) {
            cout << "->Error getting address info: " << gai_strerror(errcode) << std::endl;
            close(fd);
            return "error";
        }
        
        //Conexão ao servidor destino
        int n = connect(fd, res->ai_addr, res->ai_addrlen);
        if (n == -1) {
            cout << "->Error connecting to server" << std::endl;
            close(fd);
            return "error";
        }

        struct timeval tv;
        tv.tv_sec = 5; // 5 seconds
        tv.tv_usec = 0;

        //Configuração do timeout para o socket receção
        if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))<0) {
            cout << "->Error setting timeout" << std::endl;
            close(fd);
            return "error";
        }

        //Envio de mensagem para o servidor destino
        int w_n = send(fd, message.c_str(), message.size(), 0);
        //Verificação de erros no envio de mensagem
        if (w_n == -1) {
            cout << "->Error sending message" << std::endl;
            close(fd);
            return "error";
        }

        string comd="";
        char buffer[BUFFER_SIZE];
        //Receção de mensagem do servidor destino
        while(comd.find("\n")==string::npos){ //lê enquanto não encontrar o fim de mensagem \n
            //Receção de mensagem do servidor destino
            memset(buffer, 0, BUFFER_SIZE);
            int r_n = recv(fd, buffer, BUFFER_SIZE, 0);
            //Verificação de erros na receção de mensagem
            if (r_n < 0) {
                if(errno == EAGAIN || errno == EWOULDBLOCK){//Timeout atingido
                    cout << "->Error: timeout" << endl;
                    close(fd);
                    return "error";
                }
                cout << "->Error receiving message" << std::endl;
                close(fd);
                return "error";
            }
            //se a conexão for fechada pelo servidor sem enviar mensagem com \n
            if(r_n==0){
                cout << "->Error receiving message" << endl;
                close(fd);
                return "error";
            }
            for (int i = 0; i < r_n ; i++) {//lê apenas os caracteres recebidos(pode ser menos que 1024)
                comd+=buffer[i];
            }
        }
        // remover o texto recebido que não faz parte da resposta
        ssize_t linepos = comd.find("\n");
        string response = comd.substr(0, linepos + 1);
        close(fd);
        // indicar estabelencimento da conecção
        cout << "->TCP connection established!" << std::endl;
        return response;
    }

    //função que estabelece a conecção TCP com o servidor destino e envia uma
    //mensagem de pedido de asset e recebe o asset RSA(3) STATUS(3) FNAME(24) FSIZE(8) FDATA(?)
    string connect_assets(string message){

        //Criação do socket TCP
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            cout << "->Error creating socket" << std::endl;
            return "error";
        }

        struct addrinfo hints, *res;
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET;          //IPv4
        hints.ai_socktype = SOCK_STREAM;    //TCP socket

        //Obtenção de informação sobre o servidor destino
        int errcode = getaddrinfo(host.c_str(), to_string(port).c_str(), &hints, &res);
        if (errcode != 0) {
            cout << "->Error getting address info: " << gai_strerror(errcode) << std::endl;
            close(fd);
            return "error";
        }

        //Conexão ao servidor destino
        int n = connect(fd, res->ai_addr, res->ai_addrlen);
        if (n == -1) {
            cout << "->Error connecting to server" << std::endl;
            close(fd);
            return "error";
        }

        struct timeval tv;
        tv.tv_sec = 5; // 5 seconds
        tv.tv_usec = 0;

        if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))<0) {
            cout << "->Error setting timeout" << std::endl;
            close(fd);
            return "error";
        }

        //Envio de mensagem para o servidor destino
        int w_n = send(fd, message.c_str(), message.size(), 0);
        //Verificação de erros no envio de mensagem
        if (w_n == -1) {
            cout << "->Error sending message" << std::endl;
            close(fd);
            return "error";
        }

        //Receção de mensagem do servidor destino ínicio (até fsize)
        char buffer_start[1];
        string command;
        int space=4;// 4 espaços até ao fsize
        while(space>0){
            int r_n = recv(fd, buffer_start, 1, 0);
            if (r_n == -1) {
                if(errno == EAGAIN || errno == EWOULDBLOCK){//Timeout atingido
                    cout << "->Error: timeout" << endl;
                    close(fd);
                    return "error";
                }
                cout << "->Error receiving message" << endl;
                close(fd);
                return "error";
            }
            //se a conexão for fechada pelo servidor sem enviar mensagem com \n
            if(r_n==0){
                cout << "->Error receiving message" << endl;
                close(fd);
                return "error";
            }
            if(buffer_start[0]==' '){
                space--;
            }
            //se encontrar o \n antes de encontrar os 4 espaços(RSA NOK)
            if(space>0 && buffer_start[0]=='\n'){
                command+=buffer_start[0];
                close(fd);
                return command;
            }
            if(r_n>0){// se não ler nada não faz append
                command+=buffer_start[0];
            }
        }

        //Receção de mensagem do servidor destino (fsize)
        string control=command;
        istringstream iss(control);
        string rsa, status, fname, fsize;
        iss >> rsa >> status >> fname >> fsize;
        //verificar se a mensagem recebida é válida
        if(!verify_alphanumeric(rsa) || !verify_alphanumeric(status) || !verify_filename(fname)){
            cout << "->Error: invalid message" << endl;
            close(fd);
            return "error";
        }
        //verificar se o fsize é numérico
        if(!verify_numeric(fsize) && fsize.size()>8){//verificar se o fsize é numérico
            cout << "->Error: invalid message" << endl;
            close(fd);
            return "error";
        }
        int size=stoi(fsize);//tamanho do ficheiro (fdata)

        //Receção de mensagem do servidor destino (fdata)
        char buffer_mid[BUFFER_SIZE];
        while(size > BUFFER_SIZE) {//se o ficheiro for maior que o buffer
            memset(buffer_mid, 0, BUFFER_SIZE);
            int r_n = recv(fd, buffer_mid, BUFFER_SIZE, 0);
            if (r_n == -1) {
                if(errno == EAGAIN || errno == EWOULDBLOCK){//Timeout atingido
                    cout << "->Error: timeout" << endl;
                    close(fd);
                    return "error";
                }
                cout << "->Error receiving message" << endl;
                close(fd);
                return "error";
            }
            //se a conexão for fechada pelo servidor sem enviar mensagem com \n
            if(r_n==0){
                cout << "->Error receiving message" << endl;
                close(fd);
                return "error";
            }
            for(int i=0; i<r_n; i++){//lê apenas os caracteres recebidos(pode ser menos que 1024)
                command+=buffer_mid[i];
            }
            size-=r_n;
        }

        if(size==0){
            close(fd);
            cout << "->TCP connection established!" << std::endl;
            return command;
        }

        char buffer_fin[size];
        while(size>0){
            memset(buffer_fin, 0, size);
            int r_n = recv(fd, buffer_fin, size, 0);
            if (r_n == -1) {
                if(errno == EAGAIN || errno == EWOULDBLOCK){//Timeout atingido
                    cout << "->Error: timeout" << endl;
                    close(fd);
                    return "error";
                }
                cout << "->Error receiving message" << endl;
                close(fd);
                return "error";                
            }
            //se a conexão for fechada pelo servidor sem enviar mensagem com \n
            if(r_n==0){
                cout << "->Error receiving message" << endl;
                close(fd);
                return "error";
            }
            for(int i=0; i< r_n ; i++){
            command+=buffer_fin[i];
            }
            size-=r_n;
        }    
        cout << "->TCP connection established!" << std::endl;
        close(fd);
        return command;
    }
};

