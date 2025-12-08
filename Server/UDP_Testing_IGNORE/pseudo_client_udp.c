#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <server_ip> <server_port>\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    char buffer[BUF_SIZE];
    socklen_t addr_len = sizeof(server_addr);

    printf("UDP client started. Type commands to send to the server.\n");

    while (1) {
        printf("> ");
        fflush(stdout);

        if (!fgets(buffer, BUF_SIZE, stdin))
            break;

        buffer[strcspn(buffer, "\n")] = 0;  // remove newline

        if (strcmp(buffer, "exit") == 0)
            break;

        // send command
        if (sendto(sockfd, buffer, strlen(buffer), 0,
                   (struct sockaddr *)&server_addr, addr_len) < 0) {
            perror("sendto");
            break;
        }

        // receive server response
        int n = recvfrom(sockfd, buffer, BUF_SIZE - 1, 0,
                         (struct sockaddr *)&server_addr, &addr_len);
        if (n < 0) {
            perror("recvfrom");
            break;
        }

        buffer[n] = '\0';
        printf("Server: %s\n", buffer);
    }

    close(sockfd);
    return 0;
}
