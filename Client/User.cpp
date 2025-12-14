#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include "User.h"
#include <vector>
#include <cmath>
//std namespace
using namespace std;

//escreve as opções para o utilizador que está logado
void print_options_logged(string serverIP, string serverPORT, string UID, string password) {
    cout << "-----User Application-----" << endl;
    cout << "->Server IP: " << serverIP << endl;
    cout << "->Server Port: " << serverPORT << endl;
    cout << "--------Logged in---------" << endl;
    cout << "->UID: " << UID << endl;
    cout << "->Password: " << password << endl;
    cout << "-------User Options-------" << endl;
    cout << "->Logout" << endl;
    cout << "->Unregister" << endl;
    cout << "->Create" << endl;
    cout << "->Close" << endl;
    cout << "->MyEvents" << endl;
    cout << "->MyReservations" << endl;
    cout << "->List" << endl;
    cout << "->ChangePassword" << endl;
    cout << "->Reserve" << endl;
    cout << "->Show_event" << endl;
    cout << "--------------------------" << endl;
}

//escreve as opções para o utilizador que não está logado
void print_options_nlogged(string serverIP, string serverPORT) {
    cout << "-----User Application-----" << endl;
    cout << "->Server IP: " << serverIP << endl;
    cout << "->Server Port: " << serverPORT << endl;
    cout << "-------User Options-------" << endl;
    cout << "->Login" << endl;
    cout << "->Show_event" << endl;
    cout << "->List" << endl;
    cout << "->Exit" << endl;
    cout << "--------------------------" <<endl;
}

//LOGIN(udp, UID, password), UDP
int login_cmd(string UID, string password, UDPuser udp){
    cout << "->login..." << endl;
    //mensagem a enviar ao servidor
    string message = "LIN " + UID + " " + password + "\n";
    string response = udp.connect(message);
    //verificar se houve erro na conexão
    if(response == "error"){
        cout << "->Error connecting to server" << endl;
        return 0;
    }
    //verificar se o login foi bem sucedido ou não
    else if(response == "RLI NOK\n"){
        cout << "->Login failed" << endl;
        return 0;
    }
    else if(response == "RLI OK\n"){
        cout << "->Login successful" << endl;
        return 1;
    }
    else if(response == "RLI REG\n"){
        cout << "->Registration complete, Login successful" << endl;
        return 1;
    }
    else{//RLI ERR
        cout << "->Error" << endl;
        return 0;
    }
}

//LOGOUT(udp, UID, password), UDP
int logout_cmd(string UID, string password, UDPuser udp){
    cout << "->logout..." << endl;
    //mensagem a enviar ao servidor
    string message = "LOU " + UID + " " + password + "\n";
    string response = udp.connect(message);
    //verificar se houve erro na conexão
    if(response=="error"){
        cout << "->Error connecting to server" << endl;
        return 1;
    }
    //verificar se o logout foi bem sucedido ou não
    else if(response=="RLO NOK\n"){
        cout << "->Logout failed" << endl;
        return 1;
    }
    else if(response=="RLO OK\n"){
        cout << "->Logout successful" << endl;
        return 0;
    }
    else if(response=="RLO WRP\n"){
        cout << "->Password incorrect" << endl;
        return 0;
    }
    else if(response=="RLO UNR\n"){
        cout << "->User not registered" << endl;
        return 0;
    }
    else{//RLO ERR
        cout << "->Error" << endl;
        return 1;
    }
}

//UNREGISTER(udp, UID, password), UDP
int unregister_cmd(std::string UID, std::string password, UDPuser udp){
    cout << "->unregistering..." << endl;
    //mensagem a enviar ao servidor
    string message = "UNR " + UID + " " + password + "\n";
    string response = udp.connect(message);
    //verificar se houve erro na conexão
    if(response=="error"){
        cout << "->Error connecting to server" << endl;
        return 1;
    }
    //verificar se o unregister foi bem sucedido ou não
    else if(response=="RUR NOK\n"){
        cout << "->User not logged in" << endl;
        return 1;
    }
    else if(response=="RUR OK\n"){
        cout << "->Unregister successful" << endl;
        return 0;
    }
    else if(response=="RUR UNR\n"){
        cout << "->User not registered" << endl;
        return 0;
    }
    else if(response=="RUR WRP\n"){
        cout << "->Password incorrect" << endl;
        return 0;
    }
    else{//RUR ERR
        cout << "->Error" << endl;
        return 1;
    }
    return 0;
}

//MYEVENTS(udp, UID), UDP
void my_events_cmd(string UID, UDPuser udp, string password){
    cout << "->myEvents..." << endl;
    //mensagem a enviar ao servidor
    string message = "LME " + UID + " " + password + "\n";
    string response = udp.connect(message);
    //verificar se houve erro na conexão
    if(response=="error"){
        cout << "->Error connecting to server" << endl;
    }
    //verificar se o myEvents foi bem sucedido ou não
    else if(response=="RME NLG\n"){
        cout << "->Error: user not logged in" << endl;
    }
    else if(response=="RME WRP\n"){
        cout << "->Password incorrect" << endl;
    }
    else if(response=="RME NOK\n"){
        cout << "->No events" << endl;
    }
    else if(response.substr(0,6)=="RME OK"){
        string events=response.substr(7);
        istringstream iss(events);
        string eid;
        string status;
        while (iss >> eid >> status){
            //verificar se o EID e o status são válidos
            if(eid.size()!=3 || !verify_numeric(eid)){
                cout << "->Error: incorrect EID format" << endl;
                return;
            }
            if(status.size()!=1 || !verify_numeric(status)){
                cout << "->Error: incorrect status format" << endl;
                return;
            }
            //imprimir o EID e o status
            cout << "->Event ID: " << eid;
            if(status=="0"){
                cout << " Event has ended" << endl;
            }
            else if(status=="1"){
                cout << " Reservations still available" << endl;
            }
            else if(status=="2"){
                cout << " Event is sold out" << endl;
            }
            else if(status=="3"){
                cout << " Event has been closed" << endl;
            }
        }
    }
    else{//RME ERR
        cout << "->Error occurred!" << endl;
    }
}

//MYRESERVATIONS(udp, UID), UDP
void my_reservations_cmd(string UID, UDPuser udp, string password){
    cout << "->myReservations..." << endl;
    //mensagem a enviar ao servidor
    string message = "LMR " + UID + " " + password + "\n";
    string response = udp.connect(message);
    //verificar se houve erro na conexão
    if(response=="error"){
        cout << "->Error connecting to server" << endl;
    }
    //verificar se o myBids foi bem sucedido ou não
    else if(response=="RMR NLG\n"){
        cout << "->Error: user not logged in" << endl;
    }
    else if(response=="RMR NOK\n"){
        cout << "->No events" << endl;
    }
    else if(response=="RMR WRP\n"){
        cout << "->Password incorrect" << endl;
    }
    else if(response.substr(0,6)=="RMR OK"){
        string events=response.substr(7);
        istringstream iss(events);
        string eid;
        string date;
        string time;
        string value; // value between 1 and 999
        while (iss >> eid >> date >> time >> value){
            //verificar se o EID a data e o tempo são válidos (format: dd-mm-yyyy hh:mm:ss)
            if(eid.size()!=3 || !verify_numeric(eid)){
                cout << "->Error: incorrect EID format" << endl;
                return;
            }
            //if(date.size()!=19 ||!isalnum(date[0]) || !isalnum(date[1]) || date[2]!='-' || !isalnum(date[3]) || !isalnum(date[4]) || date[5]!='-' || !isalnum(date[6]) || !isalnum(date[7]) || !isalnum(date[8]) || !isalnum(date[9]) || date[10]!=' ' || !isalnum(date[11]) || !isalnum(date[12]) || date[13]!=':' || !isalnum(date[14]) || !isalnum(date[15]) || date[16]!=':' || !isalnum(date[17]) || !isalnum(date[18])){
            //Antes 'date' estava a guardar só os dias e não a hora de reserva
            if(date.size() !=10 ||!isalnum(date[0]) || !isalnum(date[1]) || date[2]!='-' || !isalnum(date[3]) || !isalnum(date[4]) || date[5]!='-' || !isalnum(date[6]) || !isalnum(date[7]) || !isalnum(date[8]) || !isalnum(date[9]) || time.size()!=8 || !isalnum(time[0]) || !isalnum(time[1]) || time[2]!=':' || !isalnum(time[3]) || !isalnum(time[4]) || time[5]!=':' || !isalnum(time[6]) || !isalnum(time[7])){
                cout << "->Error: incorrect date format" << endl;
                return;
            }
            //Com a condição anterior dava sempre erro se o valor fosse 1 ou 2 dígitos
            //if((value.size()!=3 || value.size()!=2 || value.size()!=1) || !verify_numeric(value))
            //O value aqui é assentos reservados, e não o numero de lugares em total
            if ((value.size()<1 || value.size()>3) || !verify_numeric(value)){
                cout << "->Error: incorrect value format" << endl;
                return;
            }
            //imprimir o EID e o status
            cout << "->Event ID: " << eid;
            cout << " Reservation Date: " << date << " " << time << endl;
            cout << " Reservation Value: " << value << endl;
        }
    }
    else{//RMB ERR
        cout << "->Error occurred!" << endl;
    }
}
//CRE UID password name event_date attendance_size Fname Fsize Fdata
//CREATE(tcp, UID, password, name, event_date, attendance_size, asset), TCP
void create_cmd(string UID, string password, TCPuser tcp, string inputs){
    cout << "->creating event..." << endl;
    istringstream iss(inputs);
    string command, name, event_date, attendance_size, asset, fsize, fdata;
    //verificar se os inputs são válidos
    iss >> command;
    iss >> name;
    if(name.size()>10 || !verify_alphanumeric_name(name)){
        cout << "->Error: incorrect name format" << endl;
        return;
    }
    iss >> asset;
    if(asset.size()>24 || !verify_filename(asset)){
        cout << "->Error: incorrect asset format" << endl;
        return;
    }
    //event_date indicating the date and time (dd-mm-yyyy hh:mm) of the event
    iss >> event_date;
    if(event_date.size()!=16 || !verify_date(event_date)){
        cout << "->Error: incorrect event_date format" << endl;
        return;
    }
    iss >> attendance_size;
    if(attendance_size.size()>3 || attendance_size.size()<2 || !verify_numeric(attendance_size)){
        cout << "->Error: incorrect attendance_size format" << endl;
        return;
    }
    //diretório onde se encontra o asset
    string filename="Client/ASSETS/" + asset;
    //verificar se o asset existe
    ifstream file(filename);
    if(!file){
        cout << "->Error: asset file does not exist" << endl;
        return;
    }
    else{
        //ler o asset
        stringstream buff; 
        buff << file.rdbuf();
        fdata=buff.str();
    }
    //verificar se o asset tem o tamanho correto
    if(fdata.size()>10*pow(10,6)){
        cout << "->Error: asset file is too big" << endl;
        return;
    }
    fsize=to_string(fdata.size());
    file.close();
    //mensagem a enviar ao servidor
    string message = "CRE " + UID + " " + password + " " +
     name + " " + event_date + " " + attendance_size + " " + asset + " " + fsize + " " + fdata + "\n";

    string response = tcp.server_connect(message);
    //verificar se houve erro na conexão
    if(response=="error"){
        cout << "->Error connecting to server" << endl;
    }
    else if(response=="RCE NLG\n"){
        cout << "->Error: user not logged in" << endl;
    }
    else if(response=="RCE NOK\n"){
        cout << "->Error: event couldn't be created" << endl;
    }
    else if(response=="RCE WRP\n"){
        cout << "->Password incorrect" << endl;
    }
    else if(response.substr(0,6)=="RCE OK"){
        string record=response.substr(7);
        istringstream iss(record);
        string EID;
        iss >> EID;
        if (EID.size()!=3 || !verify_numeric(EID)){
            cout << "->Error: incorrect EID format" << endl;
            return;
        }
        cout << "->Event opened successfully! New event id: " << EID << endl;
    }
    else{//RCE ERR
        cout <<"->Error occurred!" << endl;
    }
}

//CLOSE(tcp, UID, password, EID), TCP
void close_cmd(string UID, string password,TCPuser tcp, string inputs){
    cout << "->closing event..." << endl;
    istringstream iss(inputs);
    string command, EID;
    iss >> command;
    iss >> EID;
    //verificar se o EID é válido
    if(EID.size()!=3 || !verify_numeric(EID)){
        cout << "->Error: incorrect EID format" << endl;
        return;
    }
    //mensagem a enviar ao servidor
    string message = "CLS " + UID + " " + password + " " + EID + "\n";
    string response = tcp.server_connect(message);
    //verificar se houve erro na conexão
    if(response=="error"){
        cout << "->Error connecting to server" << endl;
    }
    //verificar se o close foi bem sucedido ou não
    else if(response=="RCL NLG\n"){
        cout << "->Error: user not logged in" << endl;
    }
    else if(response=="RCL NOE\n"){
        cout << "->Error: no such event exists" << endl;
    }
    else if(response=="RCL EOW\n"){
        cout << "->Error: event doesn't belong to you" << endl;
    }
    else if(response=="RCL SLD\n"){
        cout << "->Error: event is already sold out" << endl;
    }
    else if(response=="RCL PST\n"){
        cout << "->Error: event has already ended" << endl;
    }
    else if(response=="RCL CLO\n"){
        cout << "->Error: event has already been closed" << endl;
    }
    else if(response=="RCL OK\n"){
        cout << "->Event closed successfully!" << endl;
    }
    else if(response=="RCL NOK\n"){
        cout << "->Error: user doesn't exist or password is incorrect" << endl;
    }
    else{//RCL ERR
        cout <<"->Error occurred!" << endl;
    }
}

//LIST(tcp), TCP
void list_cmd(string UID, TCPuser tcp){
    //mensagem a enviar ao servidor
    cout << "->list..." << endl;
    //mensagem a enviar ao servidor
    string message = "LST\n";
    string response = tcp.connect_assets(message);
    //verificar se houve erro na conexão
    if(response=="error"){
        cout << "->Error connecting to server" << endl;
    }
    //verificar se o list foi bem sucedido ou não
    else if(response=="RLS NOK\n"){
        cout << "->No Events" << endl;
    }
    else if(response.substr(0,6)=="RLS OK"){
        string events = response.substr(7); 
        istringstream iss(events);
        string eid, name, state, event_date;
        while (iss >> eid >> name >> state >>  event_date){
            //verificar se o EID e o status são válidos
            if(eid.size() != 3 || !verify_numeric(eid)){
                cout << "->Error: incorrect EID format" << endl;
                return;
            }
            // verify name (spec: single word, max 10 alphanumeric)
            else if(name.size() > 10 || !verify_alphanumeric_name(name)){
                cout << "->Error: incorrect name format" << endl;
                return;
            }
            // verify state
            else if(state.size() != 1 || !verify_numeric(state)){
                cout << "->Error: incorrect state format" << endl;
                return;
            }
            else if(state != "0" && state != "1" && state != "2" && state != "3"){
                cout << "->Error: unknown state value" << endl;
                return;
            }
            // verify date+time using verify_date (expects "dd-mm-yyyy hh:mm")
            if(event_date.size()!=16 || !verify_date(event_date)){
                cout << "->Error: incorrect event_date format" << endl;
                return;
            }

            // print result using the same style as original file
            cout << "->Event ID: " << eid << " Name: " << name << " Date: " << event_date << endl;
            if(state == "0") cout << " Past" << endl;
            else if(state == "1") cout << " Future - Open" << endl;
            else if(state == "2") cout << " Future - Sold-out" << endl;
            else if(state == "3") cout << " Closed by owner" << endl;
            else cout << " Unknown state" << endl;
        }
    }
    else{//RLS ERR
        cout <<"->Error occurred!" << endl;
    }
}

//SHOWRECORD(tcp, EID), TCP
void show_cmd(TCPuser tcp, string inputs){
    cout << "->show..." << endl;
    istringstream iss(inputs);
    string command, EID, owner, name, event_date, attendance, reserved, fname, fsize, rest;
    iss >> command >> EID;
    if(EID.size()!=3 || !verify_numeric(EID)){
        cout << "->Error: incorrect EID format" << endl;
        return;
    }
    string message = "SED " + EID + "\n";
    string response = tcp.connect_assets(message);
    if(response=="error"){
        cout << "->Error connecting to server" << endl;
        return;
    }
    else if(response=="RSE NOK\n"){
        cout << "->Error: no such event exists" << endl;
        return;
    }
    else if(response.substr(0,6)=="RSE OK"){
        string inputs = response.substr(7);
        istringstream iss(inputs);
        iss >> owner >> name >> event_date >> attendance >> reserved >> fname >> fsize >> rest;

        if(fname.size()>24 || !verify_filename(fname)){
            cout << "->Error: incorrect filename format" << endl;
            return;
        }
        else if(fsize.size()>8 || !verify_numeric(fsize)){
            cout << "->Error: incorrect fsize format" << endl;
            return;
        }
        else if(fsize.size() > 10*pow(10,6)){
            cout << "->Error: file too big" << endl;
            return;
        }
        else if (event_date.size()!=16 || !verify_date(event_date)){
            cout << "->Error: incorrect event_date format" << endl;
            return;
        }
        else if(attendance.size()>3 || attendance.size()<2 || !verify_numeric(attendance)){
            cout << "->Error: incorrect attendance format" << endl;
            return;
        }
        else if(reserved.size()>3 || reserved.size()<2 || !verify_numeric(reserved)){
            cout << "->Error: incorrect reserved format" << endl;
            return;
        }
        //na directoria /SHOW colocar o asset com
        string filename = "Client/SHOW/" + fname;
        //apagar ficheiro se já existir (substiuição)
        remove(filename.c_str());
        //criar ficheiro
        ofstream file(filename);
        if(file){
            int spaces=8;
            while(spaces>0 && !response.empty()){
                if(response[0]==' ') spaces--;
                response.erase(0,1);
            }
            file << response;
            file.close();
        } else {
            cout << "->Error: couldn't open file" << endl;
            return;
        }

        cout << "->Event owner: " << owner << endl;
        cout << "->Event name: " << name << endl;
        cout << "->Event date: " << event_date << endl;
        cout << "->Attendance size: " << attendance << endl;
        cout << "->Seats reserved: " << reserved << endl;
        cout << "->File saved: " << filename << endl;
        cout << "->File size: " << fsize << " bytes" << endl;
        cout << "->File Content: " << rest << endl;
    }
    else{//RSE ERR
        cout << "->Error occurred!" << endl;
    }
}


//RESERVE(tcp, UID, password, EID, value), TCP
void reserve(string UID, string password, TCPuser tcp, string inputs){
    cout << "->reserving..." << endl;
    istringstream iss(inputs);
    string command, EID, people;
    iss >> command;
    iss >> EID;
    //verificar se o EID e o value são válidos
    if(EID.size()!=3 || !verify_numeric(EID)){
        cout << "->Error: incorrect EID format" << endl;
        return;
    }
    iss >> people;
    if(people.size()>4 || people.size()<1 || !verify_numeric(people)){
        cout << "->Error: incorrect value format" << endl;
        return;
    }
    //mensagem a enviar ao servidor
    string message = "RID " + UID + " " + password + " " + EID + " " + people + "\n";
    string response = tcp.server_connect(message);
    //verificar se houve erro na conexão
    if(response=="error"){
        cout << "->Error connecting to server" << endl;
    }
    else if(response=="RRI NLG\n"){
        cout << "->Error: user not logged in" << endl;
    }
    else if(response=="RRI ACC\n"){
        cout << "->Event accepted, best of luck!" << endl;
        cout << "->People: " << people << endl;
    }
    else if (response=="RRI WRP\n"){
        cout << "->Password incorrect" << endl;
    }
    else if(response=="RRI REJ\n"){
        cout << "->Error: not enough seats available" << endl;
    }
    else if(response=="RRI PST\n"){
        cout << "->Error: event has passed" << endl;
    }
    else if(response=="RRI CLS\n"){
        cout << "->Error: event is closed" << endl;
    }
    else if(response=="RRI SLD\n"){
        cout << "->Error: event is sold out" << endl;
    }
    else if(response=="RRI NOK\n"){
        cout << "->Error: event not active" << endl;
    }
    else{//RRI ERR
        cout <<"->Error occurred!" << endl;
    }
}


//CHANGEPASS(tcp, UID, old_password, new_password), TCP
void change_pass_cmd(string UID, string old_password, string new_password, TCPuser tcp){
    cout << "->changing password..." << endl;
    //mensagem a enviar ao servidor
    string message = "CPS " + UID + " " + old_password + " " + new_password + "\n";
    string response = tcp.server_connect(message);
    //verificar se houve erro na conexão
    if(response=="error"){
        cout << "->Error connecting to server" << endl;
    }
    //verificar se o change_password foi bem sucedido ou não
    else if(response=="RCP NLG\n"){
        cout << "->Error: user not logged in" << endl;
    }
    else if(response=="RCP WRP\n"){
        cout << "->Error: old password incorrect" << endl;
    }
    else if(response=="RCP OK\n"){
        cout << "->Password changed successfully!" << endl;
    }
    else if(response=="RCP NOK\n"){
        cout << "->Error: user doesn't exist" << endl;
    }
    else{//RCP ERR
        cout <<"->Error occurred!" << endl;
    }
}

int main(int argc, char *argv[]){
    string serverIP;
    
    //obter local host
    char hostname[1024];
    
    if (gethostname(hostname, 1024) == -1) {
        perror("gethostname");
        exit(1);
    }

    //obter endereço ip
    struct hostent *host_entry;
    host_entry = gethostbyname(hostname);
    char *ip_buffer;
    ip_buffer = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0]));
    serverIP = ip_buffer;

    string serverPort = SERVER_PORT;
    
    int logged_in = 0;  //flag que indica se o utilizador está logado ou não
    string Inputs, command, UID, password, EID;
    
    //verificar argumentos (-n ASIP -p ASport)
    if(argc == 5){
        if(strcmp(argv[1],"-n")==0 && strcmp(argv[3],"-p")==0){
            serverIP=argv[2];
            serverPort=argv[4];
        }
        else if(strcmp(argv[1],"-p")==0 && strcmp(argv[3],"-n")==0){
            serverIP=argv[4];
            serverPort=argv[2];
        }
        else{
            cout << "->Invalid arguments" << endl;
            return 0;
        }
    }

    else if(argc==3){
        if(strcmp(argv[1],"-n")==0){
            serverIP=argv[2];
        }
        else if(strcmp(argv[1],"-p")==0){
            serverPort=argv[2];
        }
    }

    if(!verify_numeric(serverPort)){
        cout << "->Invalid port" << endl;
        return 0;
    }
    
    //criar objetos UDPuser e TCPuser
    UDPuser udp(serverIP, stoi(serverPort));
    TCPuser tcp(serverIP, stoi(serverPort));
    
    while(1){
        //escrever as opções para o utilizador
        if(logged_in==1){
                print_options_logged(serverIP, serverPort, UID, password);
        }
        else{
            print_options_nlogged(serverIP, serverPort); 
        }

        getline(std::cin, Inputs);
        istringstream iss(Inputs);
        iss >> command;
        //LOGIN (udp)
        if (command=="login" && logged_in==0){
            iss >> UID;
            //verifica se UID é válido
            if(UID.size()!=UID_SIZE || !verify_numeric(UID)){
                cout << "->Invalid UID" << std::endl;
                UID.clear();
                command.clear();
                continue;
            }
            iss >> password;
            //verifica se password é válida
            if(password.size()!=8 || !verify_alphanumeric(password)){
                cout << "->Invalid password" << std::endl;
                UID.clear();
                password.clear();
                continue;
            }
            logged_in=login_cmd(UID, password, udp);
        }
        //LOGOUT(udp)
        else if(command=="logout" && logged_in==1){
            logged_in = logout_cmd(UID, password, udp);
            if (logged_in==0){
                UID.clear();
                password.clear();
            }
            command.clear();
        }
        //UNREGISTER(udp)
        else if(command=="unregister" && logged_in==1){
            logged_in = unregister_cmd(UID, password, udp);
            if (logged_in==0){
                UID.clear();
                password.clear();
            }
            command.clear();
        }

        //EXIT(udp)
        else if(command=="exit"){
            if(logged_in==1){
                cout << "->You need to logout first!" << endl;
                command.clear();
                continue;
            }
            UID.clear();
            password.clear();
            command.clear();
            cout << "->exiting ..." << std::endl;
            break;
        }
        //MYEVENTS(udp)
        else if((command=="myevents" || command=="mye") && logged_in==1){
            my_events_cmd(UID, udp, password);
            command.clear();
        }
        //MYRESERVATIONS(udp)
        else if((command=="myreservations" || command=="myr") && logged_in==1){
            my_reservations_cmd(UID, udp, password);
            command.clear();
        }
        //CHANGEPASS(tcp)
        else if(command=="changePass" && logged_in==1){
            string new_password;
            iss >> new_password;
            //verifica se a nova password é válida
            if(new_password.size()!=8 || !verify_alphanumeric(new_password)){
                cout << "->Invalid new password" << std::endl;
                command.clear();
                continue;
            }
            change_pass_cmd(UID, password, new_password, tcp);
            //atualizar a password
            password=new_password;
            command.clear();
        }
        
        //CREATE(tcp)
        else if(command=="create"&& logged_in==1){
            create_cmd(UID, password, tcp, Inputs);
            command.clear();
        }
        //CLOSE(tcp)
        else if(command=="close" && logged_in==1){
            close_cmd(UID, password, tcp, Inputs);
            command.clear();
        }
        
        //LIST(tcp)
        else if((command=="list" )){
            list_cmd(UID, tcp);
            command.clear();
        }
        //SHOW(tcp)
        else if((command=="show" )){
            show_cmd(tcp, Inputs);
            command.clear();
        }
        //RESERVE(tcp)
        else if((command=="reserve" )&& logged_in==1){
            reserve(UID, password, tcp, Inputs);
            command.clear();
        }
        else{
            cout << "->Invalid command" << std::endl;  
        }
    }

    return 0;
}