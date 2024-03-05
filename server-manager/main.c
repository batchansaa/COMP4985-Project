#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ncurses.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>


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
    return 0;
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
    printw("2. Disconnect from server\n");
    printw("3. Send a message to the server\n");
    printw("4. Receive a message from the server\n");
    printw("5. Exit\n");
    printw("Enter your choice: ");
}

int main() {
    // Initialize NCurses
    initscr();  // Initialize the screen
    cbreak();   // Line buffering disabled, Pass on every character
    noecho();   // Don't echo while we do getch
    keypad(stdscr, TRUE); // Enable keypad input

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
        int choice = getch() - '0';
        printw("%d\n", choice);
        refresh();
        switch (choice) {
            case 1:
                if (connectToServer(ipAddress, portNumber) == 0) {
                    printw("Connected to server at %s:%d\n", ipAddress, portNumber);
                    // handle server here
                } else {
                    printw("Failed to connect to server at %s:%d\n", ipAddress, portNumber);
                }
                break;
            case 2:
                printw("Disconnecting from server...\n");
                break;
            case 3:
                printw("Enter the message to send: ");
                char *message = getInput();
                printw("\nYou entered: %s\n", message);
                break;
            case 4:
                printw("Receiving message from server...\n");
                break;
            case 5:
                printw("Exiting...\n");
                break;
            default:
                printw("Invalid choice\n");
        }
        refresh();
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