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

#define HOST_REGEX "%*[^/]//%*[^@]@%[^/]"
#define PATH_REGEX "%*[^/]//%*[^/]%s"
#define USER_REGEX "%*[^/]//%[^:]"
#define PASSWORD_REGEX "%*[^/]//%*[^:]:%[^@]"
#define FILENAME_REGEX "%[^\\]+$" // [^\\]+$

struct arguments {
    char host[MAX_BUF_SIZE];
    char path[MAX_BUF_SIZE];
    char user[MAX_BUF_SIZE];
    char pass[MAX_BUF_SIZE];
    char filename[MAX_BUF_SIZE];
};

void getArguments(char *argument, struct arguments *args) {
    sscanf(argument, HOST_REGEX, args->host);
    sscanf(argument, PATH_REGEX, args->path);
    sscanf(argument, USER_REGEX, args->user);
    sscanf(argument, PASSWORD_REGEX, args->pass);
    strcpy(args->filename, strrchr(args->path, '/') + 1);

    printf("Hostname: %s\n", args->host);
    printf("Path: %s\n", args->path);
    printf("User: %s\n", args->user);
    printf("Password: %s\n", args->pass);
    printf("Filename: %s\n", args->filename);
}

int main (int argc, char *argv[]) {
    
    if (argc != 2) {
        printf("Usage: ./download <FILE>\n");
        exit(0);
    }

    struct arguments args;
    
    getArguments(argv[1], &args);



    return 0;
}