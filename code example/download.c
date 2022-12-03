#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <regex.h>
#include "./macros.h"
#include "./getip.c"
#include "./clientTCP.c"
#include "./auxFunctions.c"

#define HOST_REGEX_WHITH_USER_PASS "%*[^/]//%*[^@]@%[^/]"
#define PATH_REGEX_WHITH_USER_PASS "%*[^/]//%*[^/]%s"
#define USER_REGEX "%*[^/]//%[^:]"
#define PASSWORD_REGEX "%*[^/]//%*[^:]:%[^@]"

#define HOST_REGEX "%*[^/]//%[^/]"
#define PATH_REGEX "%*[^/]//%*[^/]%s"

typedef struct ftp {
    int control_socket_fd; // file descriptor to control socket
    int data_socket_fd; // file descriptor to data socket
} ftp;


void getArguments(char *argument, struct url *URL) {
    char hasArroba[MAX_BUF_SIZE];

    if (strchr(argument, '@') == NULL) { // No USER or PASSWORD in argument
        sscanf(argument, HOST_REGEX, URL->host);
        sscanf(argument, PATH_REGEX, URL->path);
        strcpy(URL->filename, strrchr(URL->path, '/') + 1);
        strcpy(URL->user, "anonymous");
        strcpy(URL->pass, "");

        printf("Hostname: %s\n", URL->host);
        printf("Path: %s\n", URL->path);
        printf("User: anonymous\n");
        printf("Password: %s\n", URL->pass);
        printf("Filename: %s\n", URL->filename);
    }
    else {
        sscanf(argument, HOST_REGEX_WHITH_USER_PASS, URL->host);
        sscanf(argument, PATH_REGEX_WHITH_USER_PASS, URL->path);
        sscanf(argument, USER_REGEX, URL->user);
        sscanf(argument, PASSWORD_REGEX, URL->pass);
        strcpy(URL->filename, strrchr(URL->path, '/') + 1);

        printf("Configurations of the link typed!\n");
        printf("Hostname: %s\n", URL->host);
        printf("Path: %s\n", URL->path);
        printf("User: %s\n", URL->user);
        printf("Password: %s\n", URL->pass);
        printf("Filename: %s\n\n", URL->filename);
    }
}


void getFileFromSeverFTP(struct url *URL) {
    // 1. Get the internet address using the node and service provideded in the function parameters (getaddrinfo(const char *restrict node, const char *restrict service, const struct addrinfo *restrict hints, struct addrinfo **restrict res)).
    // 2. Get the node and service names, possibly truncated to fit the specified buffer lengths (genameinfo(const struct sockaddr *restrict addr, socklen_t addrlen, char *restrict host, socklen_t hostlen, char *restrict serv, socklen_t servlen, int flags))
    // 3. After executing the function in the previous step, you will get the IP (in the host field). You can see this flag in a numeric form if you put the flag NI_NUMERICHOST in the flags field on the function provided in the previous step.

    int data_sockfd, sockfd;
	char address[MAX_BUF_SIZE];
	int bytes;
	struct addrinfo hints, *infoptr;

	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_INET; // AF_INET means IPv4 only addresses

	int result = getaddrinfo(URL->host, NULL, &hints, &infoptr);
	if (result) {
		printf("ERROR: getaddrinfo() failed!\n");
		exit(1);
	}

    struct hostent *h;
    char* ip = getip(URL->host, h); // Get the ip address of the provided host // IP Address: 193.137.29.15

    // Create socket and connect
    sockfd = createSocketAndConnect(ip, 21);

    // Get string from the server
    if (clientTCP(ip) < 0) return;

}


int main (int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./download <FILE>\n");
        exit(0);
    }

    struct url URL;
    struct ftp FTP;
    char *serverName, serverIP[16];
    
    // 1) Parse the link received and divide it into host, path, user, password and filename.
    getArguments(argv[1], &URL);

    // 2) Get the IP from the DNS
    getIPfromDNS(URL.host, serverIP);

    // 3) Establish the connection
    FTP.control_socket_fd = establishConnection(serverIP);

    char response[MAX_BUF_SIZE];
    receiveFromControlSocket(FTP.control_socket_fd, response, MAX_BUF_SIZE);

    // 4) Send instructions to the server
    if (loginControlConnection(FTP.control_socket_fd, &URL) < 0) {
        printf("ERROR: Login failed!\n");
        return -1;
    }

    if ((FTP.data_socket_fd = getServerPort(FTP.control_socket_fd)) < 0) {
        printf("ERROR: Getting server port failed!\n");
        return -1;
    }

    // send retr <filePath> command
    if (sendretr(FTP.control_socket_fd, URL.path) < 0) {
        printf("ERROR: Ssending command retr failed!\n");
        return -1;
    }

    if (downloadFileFromServer(FTP.data_socket_fd, FTP.control_socket_fd, URL.filename) < 0) {
        printf("ERROR: Download file '%s' from server failed!\n", URL.filename);
        return -1;
    }
    
    if (closeConnectionSocket(FTP.control_socket_fd) < 0) {
        printf("ERROR: Could not close connection socket! \n");
        return -1;
    }

    return 0;
}