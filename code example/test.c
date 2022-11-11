#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <regex.h>
#include "./macros.h"

#define REGEX ""

int main(int argc, char *argv[]) {
    char str[] = "/um/dois/tres/ficheiro"; // declare the size of character string  
    char reversedString[200];
    sscanf(str, REGEX, reversedString);
    return 0;  
}