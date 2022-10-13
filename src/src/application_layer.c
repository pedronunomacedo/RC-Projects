// Application layer protocol implementation

#include "../include/application_layer.h"
#define BUF_SIZE 256
void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    // Emissor
    LinkLayer defs;
    strcpy(defs.serialPort, serialPort);
    if(strcmp(role,"0")){
        defs.role = role[0]; // TRANSMITTER
    } else if(strcmp(role,"1")) {
        defs.role = role[1]; // RECEIVER
    }
    defs.baudRate = baudRate;
    defs.nRetransmissions = nTries;
    defs.timeout = timeout;

    // Open the connection between the 2 devices and prepare to send and receive
    if (llopen(defs) < 0) {
        printf("ERROR: Couldn't open serial port - fd incorrect!\n");
        return;
    }

    printf("Connection established successfully!\n");

    
    // if (llwrite(filename, BUF_SIZE) < 0) {
    //     return;
    // }

}
