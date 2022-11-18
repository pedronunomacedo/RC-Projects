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

#define HOST_REGEX_WHITH_USER_PASS "%*[^/]//%*[^@]@%[^/]"
#define PATH_REGEX_WHITH_USER_PASS "%*[^/]//%*[^/]%s"
#define USER_REGEX "%*[^/]//%[^:]"
#define PASSWORD_REGEX "%*[^/]//%*[^:]:%[^@]"

#define HOST_REGEX "%*[^/]//%[^/]"
#define PATH_REGEX "%*[^/]//%*[^/]%s"

char *PASSIVEMODE_PARAMETERS[] = {
    "%*[^(](%[^,]", 
    "%*[^(](%*[^,],%[^,]", 
    "%*[^(](%*[^,],%*[^,],%[^,]", 
    "%*[^(](%*[^,],%*[^,],%*[^,],%[^,]", 
    "%*[^(](%*[^,],%*[^,],%*[^,],%*[^,],%[^,]", 
    "%*[^(](%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%[^)]"
};

#define GET_PASSIVE_MODE_PARAMETER(n) PASSIVEMODE_PARAMETERS[n - 1]


struct arguments {
    char host[MAX_BUF_SIZE];
    char path[MAX_BUF_SIZE];
    char user[MAX_BUF_SIZE];
    char pass[MAX_BUF_SIZE];
    char filename[MAX_BUF_SIZE];
};

void getArguments(char *argument, struct arguments *args) {
    char hasArroba[MAX_BUF_SIZE];

    if (strchr(argument, '@') == NULL) { // No USER or PASSWORD in argument
        sscanf(argument, HOST_REGEX, args->host);
        sscanf(argument, PATH_REGEX, args->path);
        strcpy(args->filename, strrchr(args->path, '/') + 1);
        strcpy(args->user, "anonymous");
        strcpy(args->pass, "pass");

        printf("Hostname: %s\n", args->host);
        printf("Path: %s\n", args->path);
        printf("Filename: %s\n", args->filename);
    }
    else {
        sscanf(argument, HOST_REGEX_WHITH_USER_PASS, args->host);
        sscanf(argument, PATH_REGEX_WHITH_USER_PASS, args->path);
        sscanf(argument, USER_REGEX, args->user);
        sscanf(argument, PASSWORD_REGEX, args->pass);
        strcpy(args->filename, strrchr(args->path, '/') + 1);

        printf("Hostname: %s\n", args->host);
        printf("Path: %s\n", args->path);
        printf("User: %s\n", args->user);
        printf("Password: %s\n", args->pass);
        printf("Filename: %s\n", args->filename);
    }    
}

int main (int argc, char *argv[]) {
    
    if (argc != 2) {
        printf("Usage: ./download <FILE>\n");
        exit(0);
    }

    struct arguments args;
    
    getArguments(argv[1], &args);

    // struct hostent *h;
    // args. = getip(args.host, h);

    // int sockfd;
    // struct sockaddr_in server_addr;
    // char buf[] = "Mensagem de teste na travessia da pilha TCP/IP\n";
    // size_t bytes;

    // /*server address handling*/
    // bzero((char *) &server_addr, sizeof(server_addr));
    // server_addr.sin_family = AF_INET;
    // server_addr.sin_addr.s_addr = inet_addr(h);    /*32 bit Internet address network byte ordered*/
    // server_addr.sin_port = htons(SERVER_PORT);        /*server TCP port must be network byte ordered */

    // /*open a TCP socket*/
    // if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    //     perror("socket()");
    //     exit(-1);
    // }
    // /*connect to the server*/
    // if (connect(sockfd,
    //             (struct sockaddr *) &server_addr,
    //             sizeof(server_addr)) < 0) {
    //     perror("connect()");
    //     exit(-1);
    // }
    // /*send a string to the server*/
    // bytes = write(sockfd, buf, strlen(buf));
    // if (bytes > 0)
    //     printf("Bytes escritos %ld\n", bytes);
    // else {
    //     perror("write()");
    //     exit(-1);
    // }

    // if (close(sockfd)<0) {
    //     perror("close()");
    //     exit(-1);
    // }


    // Down here is for later! ----------------------------------------------------------------

    // char* buf = "227 Entering Passive Mode (193,137,29,15,229,46).";
    // char result1[MAX_BUF_SIZE], result2[MAX_BUF_SIZE], result3[MAX_BUF_SIZE], result4[MAX_BUF_SIZE], result5[MAX_BUF_SIZE],result6[MAX_BUF_SIZE];
    // sscanf(buf, GET_PASSIVE_MODE_PARAMETER(1), result1);
    // sscanf(buf, GET_PASSIVE_MODE_PARAMETER(2), result2);
    // sscanf(buf, GET_PASSIVE_MODE_PARAMETER(3), result3);
    // sscanf(buf, GET_PASSIVE_MODE_PARAMETER(4), result4);
    // sscanf(buf, GET_PASSIVE_MODE_PARAMETER(5), result5);
    // sscanf(buf, GET_PASSIVE_MODE_PARAMETER(6), result6);

    // printf("1st param: %s\n", result1);
    // printf("2nd param: %s\n", result2);
    // printf("3rd param: %s\n", result3);
    // printf("4th param: %s\n", result4);
    // printf("5th param: %s\n", result5);
    // printf("6th param: %s\n", result6);

    // End the later here! ----------------------------------------------------------------

    return 0;
}