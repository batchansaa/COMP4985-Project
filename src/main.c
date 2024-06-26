#include <arpa/inet.h>
#include <fcntl.h>
#include <ncurses.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_MESSAGE_LENGTH 1024    // Buffer length
#define CURRENT_VERSION 1          // Current version of the protocol
#define BUFFER_SIZE 100            // Buffer size
#define ASCII_BACKSPACE 127        // ASCII value for backspace
#define ASCII_DELETE 8             // ASCII value for delete
#define DECIMAL 10                 // Decimal base
#define MAX_PORT 65535             // Maximum port number

#define RESTRICTED_PORTS                                                                                                                                                                                                                                           \
    {                                                                                                                                                                                                                                                              \
        80, 443, 25, 22                                                                                                                                                                                                                                            \
    }

// #define PASSWORD_ACCEPTED "ACCEPTED"    // Password accepted message
// #define SERVER_STARTED "STARTED"        // Server started message
// #define SERVER_STOPPED "STOPPED"        // Server stopped message

#ifndef SOCK_CLOEXEC
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-macros"
    #define SOCK_CLOEXEC 0
    #pragma GCC diagnostic pop
#endif

typedef struct
{
    char *ipAddress;
    int   portNumber;
} ServerInfo;

typedef struct
{
    uint8_t  version;
    uint16_t contentLength;
    char    *content;
} Packet;

struct SharedData
{
    pthread_mutex_t mutex;
    pthread_cond_t  condVar;
    int             newDataFlag;
    char            newData[MAX_MESSAGE_LENGTH];    // Adjust buffer size as needed
    bool            running;
};

struct ThreadArgs
{
    int                sockfd;
    struct SharedData *sharedData;
};

bool       checkIPAddress(char *ipAddress);
bool       verifyMessageFormat(Packet packet);
bool       isRestrictedPort(long port);
char      *getInput(void);
void       printMenu(void);
void       sendToServer(int sockfd, const char *message);
void      *listenToServer(void *arg);
int        connectToServer(char *ipAddress, int portNumber);
Packet     receiveFromServer(int sockfd);
ServerInfo getSocketInformation(void);

void *listenToServer(void *arg)
{
    /**
     * Listen to the server for any incoming messages
     */

    struct ThreadArgs *args       = (struct ThreadArgs *)arg;
    int                sockfd     = args->sockfd;
    struct SharedData *sharedData = args->sharedData;

    while(sharedData->running)
    {
        Packet packet = receiveFromServer(sockfd);
        if(packet.version == 0)
        {
            printw("Thread function: exiting.\n");
            break;
        }
        pthread_mutex_lock(&sharedData->mutex);
        strncpy(sharedData->newData, packet.content, sizeof(sharedData->newData));

        if((strcmp(packet.content, "STOPPED") == 0 || strcmp(packet.content, "STOPPED\n") == 0))
        {
            printw("Thread function: Server stopped\n");
            sharedData->newDataFlag = 1;                  // Set the flag to indicate new data
            pthread_cond_signal(&sharedData->condVar);    // Signal the condition variable
        }
        else if((strcmp(packet.content, "STARTED") == 0 || strcmp(packet.content, "STARTED\n") == 0))
        {
            printw("Thread function: Server started\n");
            sharedData->newDataFlag = 1;                  // Set the flag to indicate new data
            pthread_cond_signal(&sharedData->condVar);    // Signal the condition variable
        }
        else if((strcmp(packet.content, "ACCEPTED") == 0) || (strcmp(packet.content, "ACCEPTED\n") == 0))
        {
            printw("Thread function: Password accepted\n");
            sharedData->newDataFlag = 1;                  // Set the flag to indicate new data
            pthread_cond_signal(&sharedData->condVar);    // Signal the condition variable
        }
        else if((strstr(packet.content, "/d") != NULL))
        {
            printw("----- Diagnosic: %s -----\n\n", packet.content);
            refresh();
            printMenu();
        }
        else
        {
            printw("Thread function: From server: %s\n", packet.content);
            sharedData->newDataFlag = 1;                  // Set the flag to indicate new data
            pthread_cond_signal(&sharedData->condVar);    // Signal the condition variable
        }
        pthread_mutex_unlock(&sharedData->mutex);
    }
    pthread_exit(NULL);
}

bool verifyMessageFormat(Packet packet)
{
    /**
     * Verify that the message is in the correct format
     * packet: The packet to verify
     * Return True if the message is in the correct format, False otherwise
     */
    printw("Verifying message format...\n");
    printw("Version: %d\n", packet.version);
    printw("Content Length: %d\n", packet.contentLength);
    printw("Content: %s\n", packet.content);

    return ((packet.version == CURRENT_VERSION) && (packet.contentLength <= MAX_MESSAGE_LENGTH) && (packet.contentLength == strlen(packet.content)) && (packet.content != NULL));
}

bool checkIPAddress(char *ipAddress)
{
    /**
     * Check if the given string ipAddress is a valid IP address
     * ipAddress: The string to check
     * return True if the string is a valid IP address, False otherwise
     */

    struct sockaddr_in socketAddress;
    return inet_pton(AF_INET, ipAddress, &(socketAddress.sin_addr)) != 0;
}

void sendToServer(int sockfd, const char *message)
{
    /**
     * Send a message to the server
     * sockfd: The socket file descriptor
     * message: The message to send
     */
    uint8_t  version_packet;
    uint16_t content_length_packet;

    // Create the version packet
    version_packet = (uint8_t)CURRENT_VERSION;
    send(sockfd, &version_packet, sizeof(version_packet), 0);
    printw("\nDebug: Sending version: %d\n", version_packet);

    content_length_packet = htons(strlen(message));
    printw("Debug: Sending string length: %d\n", strlen(message));
    send(sockfd, &content_length_packet, sizeof(uint16_t), 0);
    printw("Debug: Sending length: %u\n", ntohs(content_length_packet));

    // Create the content packet
    send(sockfd, message, strlen(message), 0);
    printw("Debug: Sending content: %s\n", message);
}

Packet receiveFromServer(int sockfd)
{
    /**
     * Receives a packet from the server.
     */
    Packet   packet;
    uint8_t  version;
    uint16_t contentLength;

    // Receive the version packet
    if(recv(sockfd, &version, sizeof(version), 0) > 0)
    {
        packet.version = version;
        printw("\nDebug: Received version: %d\n", packet.version);
    }
    else
    {
        close(sockfd);
        packet.version       = 0;
        packet.contentLength = 0;
        packet.content       = NULL;
        return packet;
    }

    // Receive the content length packet
    if(recv(sockfd, &contentLength, sizeof(contentLength), 0) > 0)
    {
        packet.contentLength = ntohs(contentLength);
        printw("Debug: Received Content Length: %u\n", packet.contentLength);
    }
    else
    {
        close(sockfd);
        packet.version       = 0;
        packet.contentLength = 0;
        packet.content       = NULL;
        return packet;
    }

    // Allocate memory for the content
    packet.content = (char *)malloc(packet.contentLength + 1);

    // Receive the content packet
    if(recv(sockfd, packet.content, packet.contentLength, 0) <= 0)
    {
        // Silent cleanup without printing error messages
        close(sockfd);
        free(packet.content);
        packet.version       = 0;    // Indicate failure or closed socket
        packet.contentLength = 0;
        packet.content       = NULL;
        return packet;
    }

    printw("Debug: Received Content: %s\n\n", packet.content);

    return packet;
}

int connectToServer(char *ipAddress, int portNumber)
{
    /**
     * Connect to the server at the given IP address
     * ipAddress: The IP address of the server
     * Return sockfd if successful, 0 otherwise
     */

    struct sockaddr_in server_addr;

    int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if(sockfd == -1)
    {
        perror("socket");
        return 0;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(portNumber);
    if(inet_pton(AF_INET, ipAddress, &(server_addr.sin_addr)) <= 0)
    {
        printw("inet_pton Error.\n");
        close(sockfd);
        return 0;
    }

    if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        printw("Connection failed\n");
        close(sockfd);
        return 0;
    }

    printw("Connection successful\n");
    return sockfd;
}

char *getInput(void)
{
    /**
     * Get input from the user
     * Return The input string
     */
    // nodelay(stdscr, TRUE);

    char *input;
    int   index = 0;

    input = (char *)malloc(BUFFER_SIZE * sizeof(char));

    if(input == NULL)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    while(1)
    {
        int ch;

        ch = getch();
        if(ch != ERR && ch != '\n')
        {
            if(ch == KEY_BACKSPACE || ch == ASCII_BACKSPACE || ch == ASCII_DELETE)
            {
                if(index > 0)
                {
                    index--;
                    printw("\b \b");
                    refresh();
                }
            }
            else if(index < BUFFER_SIZE - 1)
            {
                input[index++] = (char)ch;
                printw("%c", ch);
                refresh();
            }
        }
        else if(ch == '\n')
        {
            break;
        }
    }
    input[index] = '\0';
    return input;
}

void printMenu(void)
{
    /**
     * Print the menu options
     */
    printw("1. Connect to server\n");
    printw("2. Start the server\n");
    printw("3. Stop the server\n");
    printw("4. Exit the program\n");
    printw("Enter your choice: ");
    refresh();
}

bool isRestrictedPort(long port)
{
    /**
     * Check if the port is restricted
     * port: The port number to check
     * Return True if the port is restricted, False otherwise
     */
    int restrictedPorts[] = RESTRICTED_PORTS;
    for(size_t i = 0; i < sizeof(restrictedPorts) / sizeof(restrictedPorts[0]); i++)
    {
        if(port == restrictedPorts[i])
        {
            return true;
        }
    }
    return false;
}

ServerInfo getSocketInformation(void)
{
    /**
     * Get the IP address and port number of the server
     * Return The server information
     */
    char *ipAddress;
    // bool       isValidAddress;
    // int        portNumber = 0;
    char      *endPtr;
    ServerInfo serverInfo;

    while(1)
    {
        bool isValidAddress;
        printw("Enter the server IP address: ");
        ipAddress      = getInput();
        isValidAddress = checkIPAddress(ipAddress);
        if(!isValidAddress)
        {
            printw("\nThe IP address %s is not a valid IP address.\n", ipAddress);
        }
        else
        {
            printw("\nThe IP address %s is a valid IP address.\n", ipAddress);
            serverInfo.ipAddress = ipAddress;
            break;
        }
    }

    // Get the port number
    while(1)
    {
        char    *portNumberStr;
        long int portValue;

        printw("Enter the port number: ");
        portNumberStr = getInput();
        // Convert the port number to an integer
        portValue = strtol(portNumberStr, &endPtr, DECIMAL);

        // Check for conversion errors
        if(endPtr == portNumberStr || *endPtr != '\0' || portValue < 0 || portValue > MAX_PORT || isRestrictedPort(portValue))
        {
            // Handle conversion error
            printw("\nError: Invalid port number\n");
            free(portNumberStr);
        }
        else
        {
            free(portNumberStr);
            printw("\nThe port number %d is a valid port number.\n", (int)portValue);
            serverInfo.portNumber = (int)portValue;
            break;
        }
    }
    return serverInfo;
}

int main(void)
{
    /**
     * Main function.
     */
    int               sockfd;
    bool              running       = true;
    bool              serverRunning = false;
    pthread_t         listenThread;
    bool              passwordAccepted;
    struct SharedData sharedData;
    struct ThreadArgs args;

    initscr();                 // Initialize the screen
    scrollok(stdscr, TRUE);    // Enable scrolling
    cbreak();                  // Line buffering disabled, Pass on every character
    noecho();                  // Don't echo while we do getch
    keypad(stdscr, TRUE);      // Enable keypad input

    pthread_mutex_init(&sharedData.mutex, NULL);
    pthread_cond_init(&sharedData.condVar, NULL);
    sharedData.newDataFlag = 0;
    sharedData.running     = true;

    printw("--- COMP 4985 Project: Server Manager Program ---\n");

    sockfd           = 0;
    passwordAccepted = false;

    while(sockfd == 0)
    {
        int        portNumber;
        char      *ipAddress;
        ServerInfo serverInfo;

        serverInfo = getSocketInformation();
        ipAddress  = serverInfo.ipAddress;
        portNumber = serverInfo.portNumber;
        sockfd     = connectToServer(ipAddress, portNumber);
    }

    args.sockfd     = sockfd;
    args.sharedData = &sharedData;    // Pass a pointer to sharedData to the listening thread

    if(pthread_create(&listenThread, NULL, (void *(*)(void *))listenToServer, (void *)&args) != 0)
    {
        perror("pthread_create");
    }
    else
    {
        printw("Listening thread created\n");
    }

    printw("----------- Setup is complete. -----------\n");

    while(running)
    {
        int   choice;
        char *input;
        printMenu();
        input  = getInput();
        choice = input[0] - '0';

        switch(choice)
        {
            case 1:
            {
                char *password;

                printw("\nConnecting to server...\n");
                printw("Enter the password for the server: ");
                password = getInput();
                sendToServer(sockfd, password);
                pthread_mutex_lock(&sharedData.mutex);
                while(sharedData.newDataFlag == 0)
                {
                    pthread_cond_wait(&sharedData.condVar, &sharedData.mutex);
                }
                if((strcmp(sharedData.newData, "ACCEPTED") == 0) || (strcmp(sharedData.newData, "ACCEPTED\n") == 0))
                {
                    printw("-- Password accepted. You may send commands to start/stop the server. --\n\n");
                    passwordAccepted = true;
                }
                else
                {
                    printw("-- Password rejected. Please enter the right credentials. --\n");
                }
                sharedData.newDataFlag = 0;
                pthread_mutex_unlock(&sharedData.mutex);
                free(password);
                break;
            }
            case 2:
            {
                printw("\nStarting the server...\n");
                if(!passwordAccepted)
                {
                    printw("Please connect to the server first!\n");
                }
                else if(serverRunning)
                {
                    printw("The server is already running!\n");
                }
                else
                {
                    sendToServer(sockfd, "/s");
                    pthread_mutex_lock(&sharedData.mutex);
                    while(sharedData.newDataFlag == 0)
                    {
                        pthread_cond_wait(&sharedData.condVar, &sharedData.mutex);
                    }
                    // if(sharedData.newDataFlag)
                    // {
                    if((strcmp(sharedData.newData, "STARTED") == 0) || (strcmp(sharedData.newData, "STARTED\n") == 0))
                    {
                        printw("-- Server started. Server will be accepting incoming client connections. --\n");
                        serverRunning = true;
                    }
                    else
                    {
                        printw("-- Server failed to start. Please try again. --\n");
                    }
                    sharedData.newDataFlag = 0;
                    // }
                    pthread_mutex_unlock(&sharedData.mutex);
                }
                break;
            }
            case 3:
            {
                printw("\nStopping the server...\n");
                if(!passwordAccepted)
                {
                    printw("Please connect to the server first !\n");
                }
                else if(!serverRunning)
                {
                    printw("The server is already stopped !\n");
                }
                else
                {
                    sendToServer(sockfd, "/q");
                    pthread_mutex_lock(&sharedData.mutex);
                    while(sharedData.newDataFlag == 0)
                    {
                        pthread_cond_wait(&sharedData.condVar, &sharedData.mutex);
                    }
                    if((strcmp(sharedData.newData, "STOPPED") == 0) || (strcmp(sharedData.newData, "STOPPED\n") == 0))
                    {
                        printw("-- Server stopped. Server will not be accepting incoming client connections. --\n");
                        serverRunning = false;
                    }
                    else
                    {
                        printw("-- Server failed to stop. Please try again. --\n");
                    }
                    sharedData.newDataFlag = 0;
                    // }
                    pthread_mutex_unlock(&sharedData.mutex);
                }
                break;
            }
            case 4:
            {
                printw("\nExiting the program...\n");
                pthread_mutex_lock(&sharedData.mutex);
                sharedData.running = false;
                pthread_mutex_unlock(&sharedData.mutex);
                // free(ipAddress);
                close(sockfd);
                pthread_mutex_destroy(&sharedData.mutex);
                pthread_cond_destroy(&sharedData.condVar);
                running = false;
                printw("-- Cleanup is complete. --\n");
                break;
            }
            default:
            {
                printw("\nInvalid choice\n");
            }
        }
        free(input);
    }

    printw("Press any key to exit...\n");
    getch();
    endwin();
    return 0;
}
