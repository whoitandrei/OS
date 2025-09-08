// udp_client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_PORT 12345
#define BUFFER_SIZE 1024
#define TRUE 1
#define IPV4 AF_INET
#define NO_FLAGS 0

void client_run(int sockfd, struct sockaddr_in server_addr) {
    char buffer[BUFFER_SIZE];

    while (TRUE) {
        printf("enter message (or 'exit' for exit): ");

        char* fgetc_ret = fgets(buffer, sizeof(buffer), stdin);
        if (fgetc_ret == NULL) break;

        int strcmp_ret = strncmp(buffer, "exit", 4);
        if (strcmp_ret== 0) break;

        ssize_t sendto_ret = sendto(sockfd, buffer, strlen(buffer), NO_FLAGS, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (sendto_ret < 0) {
            perror("sendto failed");
            continue;
        }

        socklen_t len = sizeof(server_addr);
        ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE, NO_FLAGS, (struct sockaddr *)&server_addr, &len);
        
        if (n < 0) {
            perror("recvfrom failed");
            continue;
        }

        printf("answer from server: %s\n", buffer);
    }
}

int main() {
    struct sockaddr_in server_addr;
    int sockfd = socket(IPV4, SOCK_DGRAM, 0);

    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = IPV4;
    server_addr.sin_port = htons(SERVER_PORT);

    int inet_pton_ret = inet_pton(IPV4, "127.0.0.1", &server_addr.sin_addr);
    if (inet_pton_ret <= 0) {
        perror("invalid address");
        exit(EXIT_FAILURE);
    }

    client_run(sockfd, server_addr);

    close(sockfd);
    return EXIT_SUCCESS;
}
