// Application layer protocol implementation

#include "../include/application_layer.h"
#define BUF_SIZE 256
#define BUF_SIZE2 400

int prepareControlPacket(unsigned char *controlPacket, int bufSize, unsigned char C, int fileSize, const char *filename) {
    controlPacket[0] = C;
    controlPacket[1] = 0;

    unsigned char len; // this will be used to hold strlens
    unsigned char sizeStr[10];
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

int prepareDataPacket(unsigned char *dataBytes, unsigned char *dataControlPacket, int numSequence, int numBytes) {
    dataControlPacket[0] = 0x01;
    dataControlPacket[1] = numSequence;
    dataControlPacket[2] = numBytes / 255;
    dataControlPacket[3] = numBytes % 255;

    for(int i=0;i<numBytes;i++) {
        dataControlPacket[i+4] = dataBytes[i];
    }

    return (numBytes + 4);
}


void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename) {
    // Emissor
    int resTx = strcmp(role,"tx");
    int resRx = strcmp(role,"rx");

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

    printf("\n\n-------------  Phase 1 : Establish connection --------------------\n\n");

    // Open the connection between the 2 devices and prepare to send and receive
    if (llopen(defs) < 0) {
        printf("ERROR: Couldn't receive UA from receiver!\n");
        return;
    }

    printf("\n(Connection established successfully)\n");



    printf("\n\n-------------  Phase 2 : Start reading the penguim file --------------------\n\n");

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

        printf("control packet: ");
        for (int i=0; i<controlPacketSize;i++){
            printf("%x ", controlPacket[i]);
        }
        printf("\n");
       
        if (llwrite(controlPacket, controlPacketSize) != 0) {
            printf("ERROR: llwrite() failed!\n");
            return;
        }

        printf("send control packet\n");

        unsigned char dataPacket[BUF_SIZE], dataBytes[BUF_SIZE], fileOver = FALSE;
        int dataPacketSize = 0;

        FILE* filePtr;

        int numSequence = 0, index = 0, packetSize = 100;

        filePtr = fopen(filename, "rb");
        if (filePtr == NULL) {
            printf("ERROR: Failed to read from file with name '%s'\n", filename);
            return;
        }

        // Start reading the file

        int totalBytesRead = 0;
        int numBytesRead = 0;
        while ((numBytesRead = fread(dataBytes, (size_t) 1, (size_t) 100, filePtr)) > 0) {
            // printf("\n===%d - %d===\n", numBytesRead, numSequence);
            // Create data packet
            int dataPacketSize = prepareDataPacket(dataBytes, dataPacket, numSequence++, numBytesRead);

            if (llwrite(dataPacket, dataPacketSize) < 0) {
                printf("ERROR: Failed to write data packet to llwrite!\n");
                return;
            }
            totalBytesRead += numBytesRead;
            //printf("total = %d\n", totalBytesRead);
        }

        // Create conyrol packet end
        int controlPacketSizeEnd = prepareControlPacket(controlPacket, BUF_SIZE, 3, fileSize, filename);

        if (llwrite(controlPacket, controlPacketSizeEnd) < 0) {
            printf("ERROR: Failed to write control packet end to llwrite!\n");
            return;
        }

        fclose(filePtr);
    }
    else if (resRx == 0) {
        // 1. Read the information frame
        // 2. Send the response (RR or REJ) to the transmitter
        
        unsigned char readInformation[BUF_SIZE2];
        unsigned char controlPacket[BUF_SIZE];
        int bytesReadCTRLPacket = 0;
        if ((bytesReadCTRLPacket = llread(&controlPacket)) > 0) {
            if (controlPacket[0] == 2) {
                printf("Received control packet successfully!\n");
            }
        }

        FILE* fileCreating = fopen(filename, "wb");

        int totalBytesRead = 0, totalFrames = 0;
        for (int i = 0; i < 10968/100 + 2; i++) {
            int bytesread = llread(&readInformation); // data + BCC2
            totalFrames++;
            if (readInformation[0] == 0x01)
                totalBytesRead += bytesread-5;
            //printf("\n===%d - %d -> %d===\n", bytesread, readInformation[1], readInformation[2] * 255 + readInformation[3]);
            //printf("total = %d\n", totalBytesRead);
            if (bytesread == -2) {
                printf("RECEIVED control packet END on application_layer!\n");
                break;
            }
            unsigned char fileData[bytesread - 5];

            for (int i = 4; i < bytesread - 1; i++) {
                fileData[i - 4] = readInformation[i]; // Just the penguin
            }
            /*if(readInformation[0]==0x03 ){
                printf("RECEIVED control packet END on application_layer!\n");
                break;
            }
            if(readInformation[0]==0x02 ){
                continue;
            }*/
            for (int i = 0; i < (bytesread - 5); i++) {
                fputc(fileData[i], fileCreating);
            }
        }
        //printf("\n\n\n\ntotalBytesRead: %d\n", totalBytesRead); // 11548 -> 10968
        //printf("totalFrames: %d\n", totalFrames);

        fclose(fileCreating);
    }
    else {
        printf("ERROR: Invalid role!\n");
        exit(1);
    }

    printf("\n\n-------------  Phase 3 : Close connection --------------------\n\n");

    llclose();
}
