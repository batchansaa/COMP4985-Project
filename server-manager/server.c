#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define ERROR_MSG "Error in socket creation\n"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP address> <port number>\n", argv[0]);
        return 1;
    }

    char *ip_address = argv[1];
    int port_number = atoi(argv[2]);

    int sockfd, newsockfd, client_len;
    struct sockaddr_in serv_addr, client_addr;
    char buffer[BUFFER_SIZE];

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror(ERROR_MSG);
        return 1;
    }

    // Initialize server address struct
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip_address);
    serv_addr.sin_port = htons(port_number);

    // Bind socket to address and port
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error in binding");
        return 1;
    }

    // Listen for connections
    listen(sockfd, 5);
    printf("Server listening on %s:%d\n", ip_address, port_number);

    // Accept incoming connections
    client_len = sizeof(client_addr);
    newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, (socklen_t *)&client_len);
    if (newsockfd < 0) {
        perror("Error in accepting connection");
        return 1;
    }

    // Read and write data
    memset(buffer, 0, BUFFER_SIZE);
    read(newsockfd, buffer, BUFFER_SIZE);
    printf("Message from client: %s\n", buffer);

    close(newsockfd);
    close(sockfd);

    return 0;
}
