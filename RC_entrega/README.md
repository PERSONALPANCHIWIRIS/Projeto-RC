# Projeto RC: Sistema Cliente-Servidor de Eventos

## Descrição
Projeto desenvolvido pelo grupo 038 do turno L06 que implementa um sistema cliente-servidor para gestão de eventos via TCP/UDP.

- Estrutura de diretórios:
    Client/
    ├─ User.cpp
    └─ User.h
    Server/
    ├─ ES.c
    ├─ ES.h
    ├─ TCP_aux.c
    ├─ TCP_aux.h
    ├─ UDP_aux.c
    ├─ UDP_aux.h
    ├─ USERS/ # Diretórios criados dinamicamente
    └─ EVENTS/ # Diretórios criados dinamicamente

Para gerar os excutaveis: 'make'. Os mesmos serão criados na mesma diretoria que o makefile. 
'make clean' para limpar todos os arquivo objeto e executaveis

 - Iniciar o servidor:
./ES [-p <port>] [-v]
-p <port>: porta do servidor (se não for definido incializa com o port base 58038)
-v: modo verbose (opcional)

 - Iniciar o cliente:
 ./client [-n <IP_address>] [-p <port>]
 -p <port>: porta do servidor (se não for definido incializa com o port base 58038)
 -n <IP_address> endereço IP do servidor (caso não for definido, utiliza o endereço IP do localhost)

 - NOTAS:
    O servidor deve ser executado a partir do diretório raiz do projeto para que os paths
    relativos (USERS/ e EVENTS/) funcionem corretamente.
    Os ficheiros enviados pelo User ao Server devem-se encontrar na diretoria Client/ASSETS
    Os fichieros recebidos pelo User enviados pelo Server são guardados na diretoria Client/SHOW

 - Autores: 
    Francisco Ernesto Planas Pestana (ist109625)
    José Maria Jesus Tomás Luís (ist195615)
    Rita Pereira de Brito Líbano Monteiro (ist1110754)

 - Disciplina: Redes de Computadores (RC)

 - Ano letivo: 2025/2026