#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ncurses.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>


#define MAX_MESSAGE_LENGTH 1024
#define CURRENT_VERSION 1

typedef struct {
    char* ipAddress;
    int portNumber;
} ServerInfo;

typedef struct {
    uint8_t version;
    uint16_t contentLength;
    char *content;
} Packet;

bool checkIPAddress(char *ipAddress);
void sendToServer(int sockfd, char *message);
Packet receiveFromServer(int sockfd);
int connectToServer(char *ipAddress, int portNumber);
void startServer(int sockfd);
void stopServer(int sockfd);
char* getInput();
void printMenu();
ServerInfo get_socket_information();


bool checkIPAddress(char *ipAddress) {
    /**
     * Check if the given string ipAddress is a valid IP address
     * @param ipAddress: The string to check
     * @return: True if the string is a valid IP address, False otherwise
    */

    struct sockaddr_in sa;
    return inet_pton(AF_INET, ipAddress, &(sa.sin_addr)) != 0;
}

void sendToServer(int sockfd, char *message) {
    /**
     * Send a message to the server
     * @param sockfd: The socket file descriptor
     * @param message: The message to send
    */

    int version = CURRENT_VERSION;
    int contentLength = strlen(message);
    char *packet = (char *)malloc(MAX_MESSAGE_LENGTH * sizeof(char));

    // Create the version packet
    uint8_t version_packet = CURRENT_VERSION;
    send(sockfd, &version_packet, sizeof(version_packet), 0);

    // Create the content length packet
    uint16_t content_length_packet = strlen(message);
    send(sockfd, &content_length_packet, sizeof(content_length_packet), 0);

    // Create the content packet
    send(sockfd, message, strlen(message), 0);
}

Packet receiveFromServer(int sockfd) {
    /**
     * Receive the password acknowledgement from the server
     * @param sockfd: The socket file descriptor
    */
    Packet packet;
    uint8_t version;
    uint16_t contentLength;

    // Receive the version packet
    if (recv(sockfd, &version, sizeof(version), 0) <= 0) {
        perror("recv");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    packet.version = version;

    // Receive the content length packet
    if (recv(sockfd, &contentLength, sizeof(contentLength), 0) <= 0) {
        perror("recv");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    packet.contentLength = ntohs(contentLength);

    // Allocate memory for the content
    packet.content = (char *)malloc(packet.contentLength + 1);
    if (packet.content == NULL) {
        perror("malloc");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Receive the content packet
    if (recv(sockfd, packet.content, packet.contentLength, 0) <= 0) {
        perror("recv");
        close(sockfd);
        free(packet.content);
        exit(EXIT_FAILURE);
    }
    packet.content[packet.contentLength] = '\0';

    return packet;
}

int connectToServer(char *ipAddress, int portNumber) {
    /**
     * Connect to the server at the given IP address
     * @param ipAddress: The IP address of the server
     * @return: 0 if the connection was successful, -1 otherwise
    */

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portNumber);
    if (inet_pton(AF_INET, ipAddress, &(server_addr.sin_addr)) <= 0) {
        perror("inet_pton");
        close(sockfd);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(sockfd);
        return -1;
    }

    printw("Connection successful\n");
    printw("Enter the password for the server: ");
    char *password = getInput();
    sendToServer(sockfd, password);
    Packet packet = receiveFromServer(sockfd);
    if (strcmp(packet.content, "ACCEPTED") == 0) {
        printw("Password accepted\n");
    } else {
        printw("Password rejected\n");
    }

    // Connection successful
    if (strcmp(packet.content, "ACCEPTED") == 0) {
        return sockfd;
    } else {
        return 0;
    }
}

void startServer(int sockfd) {
    /**
     * Start the server
     * @param sockfd: The socket file descriptor
    */

    // Send the start server command
    sendToServer(sockfd, "/s");

    // Receive the server status
    Packet packet = receiveFromServer(sockfd);
    if (strcmp(packet.content, "STARTED") == 0) {
        printw("Server started successfully\n");
    } else {
        printw("Server failed to start\n");
    }
}

void stopServer(int sockfd) {
    /**
     * Stop the server
     * @param sockfd: The socket file descriptor
    */

    // Send the stop server command
    sendToServer(sockfd, "/q");

    // Receive the server status
    Packet packet = receiveFromServer(sockfd);
    if (strcmp(packet.content, "STOPPED") == 0) {
        printw("Server stopped successfully\n");
    } else {
        printw("Server failed to stop\n");
    }
}

char* getInput() {
    char *input = (char *)malloc(100 * sizeof(char));
    int index = 0;
    int ch;
    int y, x;
    getyx(stdscr, y, x);

    while ((ch = getch()) != '\n') {
        if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) { // Backspace or Delete
            if (index > 0) {
                index--;
                move(y, x+index);
                // Replace character with space
                addch(' ');
                move(y, x+index); // Move cursor back to the correct position
                refresh();

            }
        } else if (index < 100) { // Normal character
            input[index++] = ch;
            printw("%c", ch); // Echo the character back to the screen
            refresh();
        }
    }
    input[index] = '\0'; // Null-terminate the string
    return input;
}

void printMenu() {
    printw("1. Connect to server\n");           
    printw("2. Start the server\n");      
    printw("3. Stop the server\n"); 
    printw("4. Exit the program\n");                    
    printw("Enter your choice: ");    
}

ServerInfo get_socket_information() {
    char *ipAddress;
    char *portNumberStr;
    int portNumber;
    ServerInfo serverInfo;

    // Print a message and wait for user input
    printw("Enter the server IP address: ");
    ipAddress = getInput();
    printw("\nYou entered: %s\n", ipAddress);
    printw("\nThe IP address %s is a %s!\n", ipAddress, checkIPAddress(ipAddress) ? "Valid IP Address" : "Invalid IP Address");
    serverInfo.ipAddress = ipAddress;

    // Get the port number
    printw("Enter the port number: ");
    portNumberStr = getInput();

    // Convert the port number to an integer
    portNumber = atoi(portNumberStr);
    printw("\nYou entered: %d\n", portNumber);
    serverInfo.portNumber = portNumber;

    return serverInfo;
}

int main() {
    initscr();  // Initialize the screen
    cbreak();   // Line buffering disabled, Pass on every character
    noecho();   // Don't echo while we do getch
    keypad(stdscr, TRUE); // Enable keypad input
    int sockfd;
    char *ipAddress;
    int portNumber;
    ServerInfo serverInfo;

    // Print a message and wait for user input
    printw("COMP 4985 Project: Server Manager Program\n");
    
    serverInfo = get_socket_information();
    ipAddress = serverInfo.ipAddress;
    portNumber = serverInfo.portNumber;

    while (true) {
        printMenu();
        int choice = getInput()[0] - '0';
        printw("\n%d\n", choice);
        refresh();

        switch(choice) {
            case 1:
                printw("Connecting to server...\n");
                sockfd = connectToServer(ipAddress, portNumber);
                break;
            case 2:
                printw("Starting the server...\n");
                if (sockfd == 0) {
                    printw("Please connect to the server first\n");
                } else {
                    startServer(sockfd);
                }
                break;
            case 3:
                printw("Stopping the server...\n");
                if (sockfd == 0) {
                    printw("Please connect to the server first\n");
                } else {
                    stopServer(sockfd);
                }
                break;
            case 4:
                printw("Exiting the program...\n");
                close(sockfd);
                free(ipAddress);
                break;
            default:
                printw("Invalid choice\n");
        }
    }

    getch();
    endwin();
    return 0;
}