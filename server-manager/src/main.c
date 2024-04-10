#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ncurses.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

// compile with: gcc -o main main.c -lncurses

// Define the maximum length of a message
#define MAX_MESSAGE_LENGTH 1024
#define CURRENT_VERSION 1

bool checkIPAddress(char *ipAddress) {
    /**
     * Check if the given string ipAddress is a valid IP address
     * @param ipAddress: The string to check
     * @return: True if the string is a valid IP address, False otherwise
    */

    struct sockaddr_in sa;
    return inet_pton(AF_INET, ipAddress, &(sa.sin_addr)) != 0;
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

    // Connection successful
    return sockfd;
}

char *getInput() {
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

int sendMessage(int sockfd, char *message) {
    int version = htonl(CURRENT_VERSION); // Convert version to network byte order
    int version_size = sizeof(version);
    int content_size = strlen(message);
    uint16_t content_size_nbo = htons((uint16_t)content_size); // Convert content size to network byte order

    // Send version, version size, content size, and message
    // if (send(sockfd, &version, version_size, 0) != version_size) {
    //     fprintf(stderr, "Error sending version\n");
    //     return -1;
    // }

    // if (send(sockfd, &content_size_nbo, sizeof(uint16_t), 0) != sizeof(uint16_t)) {
    //     fprintf(stderr, "Error sending content size\n");
    //     return -1;
    // }

    // if (send(sockfd, message, content_size, 0) != content_size) {
    //     fprintf(stderr, "Error sending message\n");
    //     return -1;
    // }

    for (int i = 0; i < 3; i++) {
        char packet[1024];
        snprintf(packet, sizeof(packet), "Packet %d", i+1);
        send(sockfd, packet, strlen(packet), 0);
    }

    return 0; // Message sent successfully
}

// char* receiveMessage(int sockfd) {
//     char message[MAX_MESSAGE_LENGTH];
//     int version = recv(sockfd, CURRENT_VERSION, sizeof(CURRENT_VERSION), 0);
//     int content_size = recv(sockfd, sizeof(uint16_t), sizeof(uint16_t), 0);
//     int content = recv(sockfd, message, MAX_MESSAGE_LENGTH, 0);
//     if (version + content_size + content < 0) {
//         perror("Error receiving message");
//     }
//     return message;
// }

int main() {
    // Initialize NCurses
    initscr();  // Initialize the screen
    cbreak();   // Line buffering disabled, Pass on every character
    noecho();   // Don't echo while we do getch
    keypad(stdscr, TRUE); // Enable keypad input
    int sockfd;

    // Print a message and wait for user input
    printw("COMP 4985 Project: Server Manager Program\n");
    printw("Enter the server IP address: ");


    // Read input character by character
    char* ipAddress = getInput();
    printw("\nYou entered: %s\n", ipAddress);

    printw("\nThe IP address %s is a %s!\n", ipAddress, checkIPAddress(ipAddress) ? "Valid IP Address" : "Invalid IP Address");

    // Get the port number
    printw("Enter the port number: ");
    char *portNumberStr = getInput();

    // Convert the port number to an integer
    int portNumber = atoi(portNumberStr);
    printw("\nYou entered: %d\n", portNumber);

    while (true) {
        printMenu();
        if (sockfd != 0) {
            printw("Connected to server\n");
            // while (true) {
            //     char *message = receiveMessage(sockfd);
            //     printw("Status: %s\n", message);
            // }
        }
        int choice = getInput()[0] - '0';
        printw("%d\n", choice);
        refresh();
        switch (choice) {
            case 1:
                sockfd = connectToServer(ipAddress, portNumber);
                if (sockfd != 0) {
                    printw("Connected to server at %s:%d\n", ipAddress, portNumber);
                    printw("Enter the password for the server: ");
                    char *password = getInput();
                    printw("Password: %s\n", password);
                    int sent = send(sockfd, password, strlen(password), 0);
                    if (sent < 0) {
                        perror("Error sending message");
                        exit(EXIT_FAILURE);
                    } else {
                        printw("\nPassword sent successfully\n");
                    }
                } else {
                    printw("Failed to connect to server at %s:%d\n", ipAddress, portNumber);
                }
                break;
            case 2:
                printw("Starting server...\n");
                if (sockfd == 0) {
                    printw("Not connected to server\n");
                    break;
                }
                if (send(sockfd, "/s", 2, 0)< 0) {
                    perror("Error sending message");
                    exit(EXIT_FAILURE);
                } else {
                    close(sockfd);
                    printw("Started server.\n");
                }
                break;
            case 3:
                printw("Quitting server...\n");
                if (sockfd == 0) {
                    printw("Not connected to server\n");
                    break;
                }
                if (sendMessage(sockfd, "/q") < 0) {
                    perror("Error sending message");
                    exit(EXIT_FAILURE);
                } else {
                    close(sockfd);
                    printw("Stopped server.\n");
                }
                break;
            case 4:
                printw("Exiting the program...\n");
                close(sockfd);
                printw("Disconnected from server\n");
                endwin();
                break;
            default:
                printw("Invalid choice\n");
        }
    }

    // Wait for a key press before exiting
    printw("Press any key to exit...");
    getch();



    // Clean up and exit
    free(ipAddress);
    free(portNumberStr);
    endwin(); // End NCurses mode
    return 0;
}