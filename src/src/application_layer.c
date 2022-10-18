// Application layer protocol implementation

#include "../include/application_layer.h"
#define BUF_SIZE 256
void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    // Emissor
    // printf("%s\n",role);
    int resTx = strcmp(role,"tx");
    // printf("%d\n",resTx);
    int resRx = strcmp(role,"rx");
    // printf("%d\n",resRx);
    LinkLayer defs;
    strcpy(defs.serialPort, serialPort);
    if(resTx == 0){
        defs.role = LlTx; // TRANSMITTER
    } else if(resRx == 0) {
        defs.role = LlRx; // RECEIVER
    } else {
        printf("ERROR: Invalid role!\n");
        return;
    }
    defs.baudRate = baudRate;
    defs.nRetransmissions = nTries;
    defs.timeout = timeout;

    // Open the connection between the 2 devices and prepare to send and receive
    if (llopen(defs) < 0) {
        printf("ERROR: Couldn't receive UA from receiver!\n");
        return;
    }

    printf("Connection established successfully!\n");

    
    // if (llwrite(filename, BUF_SIZE) < 0) {
    //     return;
    // }

}
