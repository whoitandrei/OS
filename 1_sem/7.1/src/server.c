#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 12345
#define BUFFER_SIZE 1024
#define TRUE 1
#define IPV4 AF_INET
#define NO_FLAGS 0

void server_run(int sockfd) {
    struct sockaddr_in client_addr;
    char buffer[BUFFER_SIZE];

    while (TRUE) {
        socklen_t len = sizeof(client_addr);
        ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE, NO_FLAGS, (struct sockaddr *)&client_addr, &len);
        if (n < 0) {
            perror("recvfrom failed");
            continue;
        }

        buffer[n] = '\0';
        printf("-----\nReceived from %s:%d: %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buffer);

        ssize_t sendto_ret = sendto(sockfd, buffer, n, NO_FLAGS, (struct sockaddr *)&client_addr, len);
        if (sendto_ret < 0) {
            perror("sendto failed");
            continue;
        }

        printf("answer sent to: %s:%d: %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buffer);
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
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);

    int ret_bind = bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret_bind < 0) {
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("UDP Echo Server is running on port %d...\n", PORT);

    server_run(sockfd);

    close(sockfd);
    return EXIT_SUCCESS;
}
