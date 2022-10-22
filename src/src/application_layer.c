// Application layer protocol implementation

#include "../include/application_layer.h"
#define BUF_SIZE 256

int prepareControlPacket(unsigned char *controlPacket, int bufSize, char C, int fileSize, const char *filename) {
    /*
    infoFrame = malloc(sizeof(unsigned char) * (4 + (bufSize * 2) + 2));

    infoFrame[0] = FLAG;
    infoFrame[1] = A_SET;
    infoFrame[2] = C; // Changes between C_S0 AND C_S1
    infoFrame[3] = A_SET ^ C;

    // Store data
    infoFrame[4] = buf[0];
    char bcc2 = buf[0]; // Vaiable to store the XOR while going through the data
    
    int countStuffings = 0;
    for (int i = 1; i < bufSize; i++) {
        stuffing(infoFrame, buf[i], i, countStuffings);
        bcc2 ^= buf[i];
    }
    // Byte stuffing of the BCC2 before storing it in the array
    stuffing(infoFrame, bcc2, bufSize, countStuffings);

    int totalBytes = 5 + bufSize + countStuffings; // FLAG | A | C | BCC1 | D1 ... Dn (bufSize) | BCC2 | FLAG

    infoFrame[totalBytes] = FLAG; // Determines the end of the array

    printf("\n\n\n\n\n\n\n\n\n\n\n");
    for (int i = 0; i < sizeof(infoFrame); i++) {
        printf("infoFrame[%d] = %d\n", i, infoFrame[i]);
    }
    printf("\n\n\n\n\n\n\n\n\n\n\n");

    return totalBytes;

    */

    controlPacket[0] = C;
    controlPacket[1] = 0;

    unsigned char len; // this will be used to hold strlens
    char sizeStr[10];
    sprintf(sizeStr, "%d", fileSize); // stores the size of the file in a string
    unsigned char length = strlen(sizeStr); // Stores the length of the sizeStr variable

    len = strlen(sizeStr); // Size of the sizeStr variable

    controlPacket[2] = len;

    memcpy(controlPacket + 3, sizeStr, len); // (controlPacket + 3) is positioned on the first byte of the V1 field

    unsigned int currentPos=3+len; // Position the head of the array in the T2 field

    controlPacket[currentPos] = 1; // Indicates the V2 field contains the name of the file
    currentPos++; // Position the head of the array in the L2 field
    
    len = strlen(filename); // Size of the variable filename
    controlPacket[currentPos] = len; // Indicates the size of the variable to be read in the V2 field
    currentPos++; // Position the head of the array in the first byte of the V2 field
    memcpy(controlPacket+currentPos, filename, len);

    currentPos += len; // Corresponds to the size of the controlPacket array

    return currentPos; // Goes to llwrite!
}


void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename) {
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



    printf("-------------  Phase 2 : Start reading the penguim file --------------------\n");

    unsigned char controlPacket[600];
    struct stat st;
    stat(filename, &st);
    int fileSize = st.st_size;
    int start = 0;

    if (resTx == 0) {
        // 1. Create controlPacket
        // 2. Call the llwrite to create the info frame
        // 2. Make byte stuffing on the array
        // 3. Send the information frame to the receiver
        int controlPacketSize = prepareControlPacket(controlPacket, BUF_SIZE, 2, fileSize, filename);

        // Print control packet
        /*
        printf("--------- CONTROL PACKET ----------\n");
        for (int i = 0; i < controlPacketSize; i++) {
            printf("controlPacket[%d]: %02x\n", i, controlPacket[i]);
        }
        */
       
        if (llwrite(controlPacket, controlPacketSize) != 0) {
            printf("ERROR: llwrite() failed!\n");
            return;
        }
    }
    else if (resRx == 0) {
        unsigned char *readedControlPacket;
        if (llread(readedControlPacket) < 0) {
            printf("ERROR: llread() failed!\n");
        }
    }
    else {
        printf("ERROR: Invalid role!\n");
        exit(1);
    }
    

}
