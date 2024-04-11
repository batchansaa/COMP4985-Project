#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define DIAGNOSTIC_INTERVAL 5

typedef struct
{
    uint8_t  version;
    uint16_t length;
    char    *content;
} Packet;

Packet receivePacket(int client_socket)
{
    Packet packet;

    // Receive version
    if(recv(client_socket, &packet.version, sizeof(packet.version), 0) == -1)
    {
        perror("Receive version failed");
        exit(EXIT_FAILURE);
    }

    // Receive length
    if(recv(client_socket, &packet.length, sizeof(packet.length), 0) == -1)
    {
        perror("Receive length failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Received length: %d\n", packet.length);
    }

    // Allocate memory for content
    packet.content = (char *)malloc(packet.length * sizeof(char));

    // Receive content
    if(recv(client_socket, packet.content, packet.length, 0) == -1)
    {
        perror("Receive content failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Received content: %s\n", packet.content);
    }

    return packet;
}

void *sendDiagnostic(void *arg)
{
    // sleep for a minute
    int sockfd = *((int *)arg);
    sleep(20);

    while(true)
    {
        // Create and send the diagnostic packet
        // Packet packet;
        // packet.version = 1;
        // packet.content = "Diagnostic data...";
        // packet.length = strlen(packet.content);

        uint8_t  version_packet;
        uint16_t content_length_packet;

        // Create the version packet
        version_packet = (uint8_t)1;
        send(sockfd, &version_packet, sizeof(version_packet), 0);
        printf("Debug: Sending version: %d\n", version_packet);

        content_length_packet = htons(strlen("/d 3"));
        send(sockfd, &content_length_packet, sizeof(content_length_packet), 0);
        printf("Debug: Sending length: %d\n", content_length_packet);

        // Create the content packet
        send(sockfd, "/d 3", strlen("/d 3"), 0);
        printf("Debug: Sending content: %s\n", "/d 3");

        // Sleep for the specified interval
        sleep(DIAGNOSTIC_INTERVAL);
    }
    pthread_exit(NULL);
}

void sendPacket(int client_socket, char *content)
{
    Packet packet;
    packet.version = 1;
    packet.content = content;
    packet.length  = strlen(packet.content);

    printf("Sending packet with content: %s\n", packet.content);
    printf("Sending packet with length: %d\n", packet.length);

    // Send version
    // if (send(client_socket, &packet.version, sizeof(packet.version), 0) == -1) {
    //     perror("Send version failed");
    //     exit(EXIT_FAILURE);
    // }

    // // Send length
    // if (send(client_socket, &packet.length, sizeof(packet.length), 0) == -1) {
    //     perror("Send length failed");
    //     exit(EXIT_FAILURE);
    // }

    uint8_t  version_packet;
    uint16_t content_length_packet;

    // Create the version packet
    version_packet = (uint8_t)1;
    send(client_socket, &version_packet, sizeof(version_packet), 0);
    printf("Debug: Sending version: %d\n", version_packet);

    content_length_packet = htons(strlen(content));
    send(client_socket, &content_length_packet, sizeof(content_length_packet), 0);
    printf("Debug: Sending length: %d\n", content_length_packet);

    // Send content
    if(send(client_socket, packet.content, packet.length, 0) == -1)
    {
        perror("Send content failed");
        exit(EXIT_FAILURE);
    }
}

void handleClient(int client_socket)
{
    Packet packet;

    // Receive password packet
    packet         = receivePacket(client_socket);
    char *password = packet.content;
    if(strcmp(password, "password") != 0)
    {
        sendPacket(client_socket, "DENIED");
    }
    else
    {
        sendPacket(client_socket, "ACCEPTED");
    }

    int       sockfd = client_socket;
    pthread_t diagnostic_thread;
    pthread_create(&diagnostic_thread, NULL, sendDiagnostic, &sockfd);

    while(1)
    {
        // Receive message packet
        printf("Waiting for command...\n");
        packet = receivePacket(client_socket);
        if(strcmp(packet.content, "/s") == 0)
        {
            sendPacket(client_socket, "STARTED");
        }
        else if(strcmp(packet.content, "/q") == 0)
        {
            sendPacket(client_socket, "STOPPED");
        }
        else
        {
            sendPacket(client_socket, "UNKNOWN COMMAND");
        }
    }
}

int main()
{
    int                sockfd, newsockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t          client_len;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(PORT);

    // Bind socket
    if(bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if(listen(sockfd, 5) == -1)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while(1)
    {
        // Accept incoming connection
        client_len = sizeof(client_addr);
        newsockfd  = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        if(newsockfd == -1)
        {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        // Handle client request
        handleClient(newsockfd);

        // Close client socket
        close(newsockfd);
    }

    // Close server socket
    close(sockfd);

    return 0;
}
