#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <unistd.h>
#include "./macros.h"

#define SERVER_PORT 21

struct url {
    char host[MAX_BUF_SIZE];
    char path[MAX_BUF_SIZE];
    char user[MAX_BUF_SIZE];
    char pass[MAX_BUF_SIZE];
    char filename[MAX_BUF_SIZE];
};

char *PASSIVEMODE_PARAMETERS[] = {
    "%*[^(](%[^,]", 
    "%*[^(](%*[^,],%[^,]", 
    "%*[^(](%*[^,],%*[^,],%[^,]", 
    "%*[^(](%*[^,],%*[^,],%*[^,],%[^,]", 
    "%*[^(](%*[^,],%*[^,],%*[^,],%*[^,],%[^,]", 
    "%*[^(](%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%[^)]"
};

#define GET_PASSIVE_MODE_PARAMETER(n) PASSIVEMODE_PARAMETERS[n - 1]


void getIPfromDNS(char *serverName, char *serverIP) {
    struct hostent *host;

    if ((host = gethostbyname(serverName)) == NULL) {
        printf("ERROR: gethostbyname() failed\n");
    }

    char* ip = (char*) malloc(200 * sizeof(char));
    ip = inet_ntoa(*((struct in_addr *) host->h_addr));
    strcpy(serverIP, ip);

    printf("Host name  : %s\n", host->h_name);

    printf("IP Address : %s\n", ip); // A numeric host is in the following format: __:__:__:__ (example: 193.137.29.15)
    printf("Press ENTER to continue...\n");
	getc(stdin);
}

void writeOnSocket(int socket, char *stringToWrite) {
    size_t bytesWritten = write(socket, stringToWrite, strlen(stringToWrite));

    if (bytesWritten < 0) {
        printf("ERROR: Write to socket failed!\n");
        exit(-1);
    }
}

int sendToControlSocket(int socket, char *cmdHeader, char *cmdBody) {
    printf("Sending to control Socket > %s %s\n", cmdHeader, cmdBody);
    int bytes = write(socket, cmdHeader, strlen(cmdHeader));
    if (bytes != strlen(cmdHeader))
        return -1;
    bytes = write(socket, " ", 1);
    if (bytes != 1)
        return -1;
    bytes = write(socket, cmdBody, strlen(cmdBody));
    if (bytes != strlen(cmdBody))
        return -1;
    bytes = write(socket, "\n", 1);
    if (bytes != 1)
        return -1;
    
    return 0;
}

int receiveFromControlSocket(int socket, char *string, int size) {
    printf("Receiving from control Socket \n");
    FILE *fp = fdopen(socket, "r");

    do {
        memset(string, 0, size);
        fgets(string, size, fp); // reads one line from the server
        printf("%s", string);
    } while (!('1' <= string[0] && string[0] <= '5') || string[3] != ' '); // Continues reading until the line received has no HTTP code

    return 0;
}

int createSocketAndConnect(char *address, int port) {
    int sockfd;
    struct sockaddr_in server_addr;

    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(address);    /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(port);        /*server TCP port must be network byte ordered - You can find the SERVER_PORT (in this case, is 21), in "Guião da aula prática" on the section "Experiência de FTP (transferência de ficheiro)" */

    /*open a TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(-1);
    }

    /*connect to the server*/
    if (connect(sockfd,
                (struct sockaddr *) &server_addr,
                sizeof(server_addr)) < 0) {
        perror("connect()");
        exit(-1);
    }

    return sockfd;
}

/*
    HTTP codes:
        - 1xx informational response: the request was received, continuing process
        - 2xx successful: the request was successfully received, understood, and accepted
        - 3xx redirection: further action needs to be taken in order to complete the request
        - 4xx client error: the request contains bad syntax or cannot be fulfilled
        - 5xx server error: the server failed to fulfil an apparently valid request
*/

int sendCommandInterpretResponse(int socket, char *cmdHeader, char *cmdBody, char* response, int responseLength, int readingFile) {
    if (sendToControlSocket(socket, cmdHeader, cmdBody) < 0) {
        printf("Error Sending Command  %s %s\n", cmdHeader, cmdBody);
        return -1;
    }

    int code;
    while (1) {
        receiveFromControlSocket(socket, response, responseLength);
        code = response[0] - '0'; // Turns the first character of the string response into an integer number substracting the ASCII code of the number 0
        
        switch (code) {
            case 1:
                // expecting another reply
                if (readingFile == 1) return 2;
                else break;
            case 2:
                // request action success
                return 2;
            case 3:
                // needs aditional information
                return 3;
            case 4:
                // try again
                if (sendToControlSocket(socket, cmdHeader, cmdBody) < 0) {
                    printf("[auxFunctions.c - sendCommandInterpretResponse()] > Error Sending Command  %s %s\n", cmdHeader, cmdBody);
                    return -1;
                }
                break;
            case 5:
                // error in sending command, closing control socket , exiting application
                printf("[auxFunctions.c - sendCommandInterpretResponse()] > Command not accepted... \n");
                close(socket);
                exit(-1);
                break;
            default:
                break;
        }
    }
}

int loginControlConnection(int socket, struct url *URL) {
    printf("\n----- LOGIN ----\n");
    printf("Sending username...\n\n");

    char response[MAX_BUF_SIZE];

    int rtr = sendCommandInterpretResponse(socket, "user", URL->user, response, MAX_BUF_SIZE, 0);
    if (rtr == 3) {
        printf("Sent username!\n\n");
    }
    else {
        printf("Error sending Username...\n\n");
        return -1;
    }

    rtr = sendCommandInterpretResponse(socket, "pass", URL->pass, response, MAX_BUF_SIZE, 0);
    if (rtr == 2) {
        printf("Sent password!\n\n");
    }
    else {
        printf("Error sending Password...\n\n");
        return -1;
    }

    return 0;
}

int getServerPort(int control_socket) {
    char firstB[10], secondB[10];
    memset(firstB, 0, sizeof(firstB)); memset(secondB, 0, sizeof(secondB));

    char serverResponse[MAX_BUF_SIZE];

    int rtr = sendCommandInterpretResponse(control_socket, "pasv", "", serverResponse, MAX_BUF_SIZE, 0);

    char byte1[5], byte2[5], byte3[5], byte4[5], byte5[5], byte6[5];

    sscanf(serverResponse, GET_PASSIVE_MODE_PARAMETER(1), byte1);
    sscanf(serverResponse, GET_PASSIVE_MODE_PARAMETER(2), byte2);
    sscanf(serverResponse, GET_PASSIVE_MODE_PARAMETER(3), byte3);
    sscanf(serverResponse, GET_PASSIVE_MODE_PARAMETER(4), byte4);
    sscanf(serverResponse, GET_PASSIVE_MODE_PARAMETER(5), byte5);
    sscanf(serverResponse, GET_PASSIVE_MODE_PARAMETER(6), byte6);

    char ip[MAX_BUF_SIZE];
    sprintf(ip, "%s.%s.%s.%s", byte1, byte2, byte3, byte4);

    int port = atoi(byte5) * 256 + atoi(byte6);

    int data_socket_fd = createSocketAndConnect(ip, port);

    return data_socket_fd;
}

int sendretr(int control_socket, char *filePath) {
    char serverResponse[MAX_BUF_SIZE];

    if (sendCommandInterpretResponse(control_socket, "retr", filePath, serverResponse, MAX_BUF_SIZE, 1) < 0) {
        printf("ERROR: Failed to send retr command to server! \n");
        return -1;
    }

    return 0;
}

int downloadFileFromServer(int data_socket, int control_socket, char *filename) {
    // Create file to write on
    FILE *fileCreated = fopen(filename, "w");

    if (fileCreated == NULL) {
        printf("ERROR: Could not create file %s! \n", filename);
        return -1;
    }

    char fileBuffer[MAX_BUF_SIZE];
    int bytesRead = 0;

    printf("Starting reading the file packets from the server...\n");
    while ((bytesRead = read(data_socket, fileBuffer, MAX_BUF_SIZE)) > 0) {
        printf("Readed %d bytes from file %s! \n", bytesRead, filename);
        if (bytesRead < 0) {
            printf("ERROR: Could not read from file %s of the server! \n", filename);
            return -1;
        }

        if (fwrite(fileBuffer, bytesRead, 1, fileCreated) < 0) {
            printf("ERROR: Could not write to recent file created! \n");
            return -1;
        }
    }
    printf("End reading the file packets from the server (%d)!\n", bytesRead);
    
    printf("Download file %s from server successfully! \n", filename);

    // Close the created file and the data socket port
    if (fclose(fileCreated) < 0) {
        printf("ERROR: Could not close file created %s! \n", filename);
        return -1;
    }

    close(data_socket);

    char serverResponse[MAX_BUF_SIZE];
    
    if (receiveFromControlSocket(control_socket, serverResponse, MAX_BUF_SIZE) < 0) {
        printf("ERROR: Failed to receive quit command from the server! \n");
        return -1;
    }

    if (serverResponse[0] != '2') { // If the first number of response is = 2, then the server could request action successfully.
        return -1;
    }

    return 0;
}

int closeConnectionSocket(int control_socket) {
    char serverResponse[MAX_BUF_SIZE];

    if (sendCommandInterpretResponse(control_socket, "quit", "", serverResponse, MAX_BUF_SIZE, 0) < 0) {
        printf("ERROR: Failed to send quit command to server! \n");
        return -1;
    }
    
    return 0;
}

int establishConnection(char *serverIP) {
    int sockfd;
    struct sockaddr_in server_addr;

    /*server address handling*/
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(serverIP);    /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(SERVER_PORT);        /*server TCP port must be network byte ordered */

    /*open a TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(-1);
    }

    /*connect to the server*/ // Stuck in this function!
    if (connect(sockfd,
                (struct sockaddr *) &server_addr,
                sizeof(server_addr)) < 0) {
        perror("connect()");
        exit(-1);
    }

    return sockfd;
}