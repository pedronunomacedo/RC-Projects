// Link layer protocol implementation

#include "../include/link_layer.h"
#include "fcntl.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <stdlib.h>
#include <signal.h>

int temp = 0;


#define FALSE 0
#define TRUE 1

#define FLAG 0x7E
#define A_SET 0x03
#define A_UA 0x03
#define C_SET 0x03
#define C_UA 0x07

#define C_RR0 0x05 // receiver ready (c = 0)
#define C_RR1 0x85 // receiver ready (c = 1)
#define C_REJ0 0x01 // receiver rejected (c = 0)
#define C_REJ1 0x81 // receiver rejected (c = 1)
#define C_S0 0x00 // transmitter -> receiver
#define C_S1 0x40 // receiver -> transmitter

#define FLAG1 0x7D
#define FLAG2 0x5E
#define FLAG3 0x5D

#define BUF_SIZE 256
#define BAUDRATE B38400

// Safe connectionParameters variable info
char serialPort[50];
LinkLayerRole role;
int baudRate;
int nRetransmissions;
int timeout;

unsigned char SET[5];
unsigned char UA[5];
unsigned char buf[BUF_SIZE];
char currentC = C_S1;

int fd; // serial file descriptor
LinkLayer connectionParametersGlobal;

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

int alarmEnabled = FALSE;
int alarmCount = 0;

// Alarm function handler
void alarmHandler(int signal) {
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}


void prepareSet() {
    SET[0] = FLAG;
    SET[1] = A_UA;
    SET[2] = C_SET;
    SET[3] = A_UA ^ C_SET;
    SET[4] = FLAG;
}

int sendSet(int fd) {
    int sentBytes = 0;
    sentBytes = write(fd, SET, 5);
    printf("Sent SET to reader [%x,%x,%x,%x,%x]\n", SET[0], SET[1], SET[2], SET[3], SET[4]);
    
    return sentBytes;
}

void prepareUA() {
    UA[0] = FLAG;
    UA[1] = A_UA;
    UA[2] = C_UA;
    UA[3] = A_UA ^ C_UA;
    UA[4] = FLAG;
}

int sendUA(int fd) {
    int sentBytes = 0;
    sentBytes = write(fd, UA, 5);
    printf("Sent UA to writer [%x,%x,%x,%x,%x]\n", UA[0], UA[1], UA[2], UA[3], UA[4]);
    return sentBytes;
}

enum state receiveUA (int fd, unsigned char A, unsigned char C) {
    alarmEnabled = TRUE; // ???????????????????????????
    enum state STATE = START;
    char buf;
    // printf("alarmEnabled: %d, state : %d\n", alarmEnabled, STATE);
    while (STATE != STOP && alarmEnabled == TRUE) {
        int bytes = read(fd, &buf, 1);

        if (bytes > 0) {
            STATE = changeState(STATE, buf, A, C);
            if (STATE == STOP) alarmEnabled = FALSE;
        }

        // Only to debug! ///////////////////////////////////////////////
        switch (STATE) {
            case START:
                printf("Received state START!\n");
                break;
            case FLAG_RCV:
                printf("Received state FLAG_RCV!\n");
                break;
            case A_RCV:
                printf("Received state A_RCV!\n");
                break;
            case C_RCV:
                printf("Received state C_RCV!\n");
                break;
            case BCC_OK:
                printf("Received state BCC_OK!\n");
                break;
            default:
                printf("Received enter default!\n");
                break;
        }
        printf("alarmEnabled = %d\n", alarmEnabled);
        ////////////////////////////////////////////////////////////////
    }

    return STATE;
}


void receiveSET (int fd, unsigned char A, unsigned char C) {
    enum state STATE = START;
    unsigned char buf;
    // printf("alarmEnabled: %d, state : %d\n", alarmEnabled, STATE);
    
    while (STATE != STOP) {
        int bytes = read(fd, &buf, 1);
        printf("buf = %c", buf);

        if (bytes > 0) {
            STATE = changeState(STATE, buf, A, C);
        }

        // Only to debug! ///////////////////////////////////////////////
        switch (STATE) {
            case START:
                printf("Received state START!\n");
                break;
            case FLAG_RCV:
                printf("Received state FLAG_RCV!\n");
                break;
            case A_RCV:
                printf("Received state A_RCV!\n");
                break;
            case C_RCV:
                printf("Received state C_RCV!\n");
                break;
            case BCC_OK:
                printf("Received state BCC_OK!\n");
                break;
            default:
                printf("Received enter default!\n");
                break;
        }
        printf("alarmEnabled = %d\n", alarmEnabled);
        ////////////////////////////////////////////////////////////////
    }
}

/*
void receiveARRAY(int fd, unsigned char SET[]) {
    int state = 0;
    unsigned char ch;

    while (state != 5) { // While it doesn't get to the end of the state machine
        read(fd, &ch, 1); // Read one char
        switch (state) {
            case 0: // Start node (waiting fot the FLAG)
                if (ch == SET[0]) {
                    state = 1; // Go to the next state
                } // else { stay in the same state }
                break;
            case 1: // State Flag RCV
                if (ch == SET[1]) {
                    state = 2; // Go to the next state
                }
                else if (ch == SET[0]) {
                    state = 1; // Stays on the same state
                }
                else {
                    state = 0; // other character received goes to the initial state
                }
                break;
            case 2: // State A RCV
                if (ch == SET[2]) {
                    state = 3; // Go to the next state
                }
                else if (ch == SET[0]) {
                    state = 1;
                }
                else {
                    state = 0;
                }
                break;
            case 3: // State C RCV
                if (ch == SET[3]) {
                    state = 4; // Go to the next state
                }
                else if (ch == SET[0]) {
                    state = 1;
                }
                else {
                    state = 0;
                }
                break;
            case 4: // State BCC_OK
                if (ch == SET[4]) {
                    state = 5; // Go to the final state
                }
                else {
                    state = 0;
                }
                break;
            default:
                break;
            }
    }
}
*/

int sendReadyToReceiveMsg(int fd) { // Send UA
    prepareUA();
    if (sendUA(fd) < 0) {
        printf("ERROR: sendReadyToReceiveMsg failed!\n");
        return -1;
    }
    return 0;
}

int sendReadyToTransmitMsg(int fd) { // send SET
    prepareSet();
    if (sendSet(fd) < 0) {
        printf("ERROR: sendReadyToReceiveMsg failed!\n");
        return -1;
    }
    return 0;
}

void saveConnectionParameters (LinkLayer connectionParameters) {
    for (int i = 0; i < 50; i++) {
        serialPort[i] = connectionParameters.serialPort[i];
    }

    role = connectionParameters.role;
    baudRate = connectionParameters.baudRate;
    nRetransmissions = connectionParameters.nRetransmissions;
    timeout = connectionParameters.timeout;
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    // Save the connectionParameters variable info into global variables
    saveConnectionParameters(connectionParameters);

    (void)signal(SIGALRM, alarmHandler);

    connectionParametersGlobal = connectionParameters;
    fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);

    if (fd < 0) {
        perror(connectionParameters.serialPort);
        return -1;
    }

    struct termios oldtio;
    struct termios newtio;
    
    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_sele/* code */
    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    switch(connectionParameters.role) {
        case LlTx:
            printf("Enter the TRANSMITTER!\n");
            alarmCount = 0;

            do {
                alarm(connectionParameters.timeout);
                enum state STATE;
                do {
                    if (alarmCount == 0 || alarmEnabled == TRUE) {
                        if (sendReadyToTransmitMsg(fd) < 0) {
                            printf("ERROR: Failed to send SET!\n");
                            continue;
                        }
                    }
                    // Read UA
                    prepareUA();
                    STATE = receiveUA(fd, A_UA, C_UA);
                } while (alarmEnabled == TRUE);
                printf("alarmCount = %d\n", alarmCount);
                if (STATE == STOP) break;
            } while (alarmCount < connectionParameters.nRetransmissions);

            if (alarmCount < connectionParameters.nRetransmissions) printf("Received UA from receiver successfully!\n");
            else {
                return -1;
            }
            break;
        case LlRx:
            printf("Enter the RECEIVER!\n");
            receiveSET(fd, A_SET, C_SET); // prepare UA

            if (sendReadyToReceiveMsg(fd) < 0) {
                printf("ERROR: Failed to send UA!\n");
            }
            break;
        default:
            printf("ERROR: Unknown role!\n");
            exit(1);
    }
    
    return fd;
}

int stuffing (unsigned char *frame, char byte, int i, int countStuffings) {
    printf("\n\n\n\n\n\n\n");
    printf("(link_layer - stuffing) The char readed from controlPacket was %02x\n", byte);
    if (byte == FLAG) {
        printf("frame[4 + %d + %d] = %02x (1st if - 1st)", i, countStuffings, FLAG);
        frame[4 + i + countStuffings] = FLAG1;
        printf("frame[4 + %d + %d + 1] = %02x (1st if - 2nd)", i, countStuffings, FLAG2);
        frame[4 + i + countStuffings + 1] = FLAG2;
        countStuffings++;
    }
    else if (byte == FLAG1) { // byte is equal to the octeto
        printf("frame[4 + %d + %d + 1] = %02x (2nd if)", i, countStuffings, FLAG3);
        frame[4 + i + countStuffings + 1] = FLAG3;
    }
    else {
        printf("frame[4 + %d + %d] = %02x (else)", i, countStuffings, byte);
        frame[4 + i + countStuffings] = byte;
    }

    return countStuffings;
}

/**
 * @brief 
 * 
 * @param buf (control Packet) C | T1 | L1 | V1 | T2 | L2 | V2 | ...
 * @param bufSize 
 * @param C 
 * @param infoFrame 
 * @return int 
 */

int prepareInfoFrame(const unsigned char *buf, int bufSize, char C, unsigned char *infoFrame) {
    infoFrame[0] = FLAG;
    infoFrame[1] = A_SET;
    infoFrame[2] = C; // Changes between C_S0 AND C_S1
    infoFrame[3] = A_SET ^ C;

    // Store data
    infoFrame[4] = buf[0]; // (buf[0] == 2) -> START controlPacket | (buf[0] == 3) -> END controlPacket
    char bcc2 = buf[0]; // Vaiable to store the XOR while going through the data
    
    int countStuffings = 0;
    for (int i = 1; i < bufSize; i++) {
        countStuffings += stuffing(infoFrame, buf[i], i, countStuffings);
        printf("countStuffings[%d] = %d\n", i, countStuffings);
        bcc2 ^= buf[i];
    }
    // Byte stuffing of the BCC2 before storing it in the array
    stuffing(infoFrame, bcc2, bufSize, countStuffings);
    printf("countStuffings = %d\n", countStuffings);

    infoFrame[13] = FLAG; // TEST_INFO_FRAME

    int totalBytes = 5 + bufSize + countStuffings; // FLAG | A | C | BCC1 | D1 ... Dn (bufSize) | BCC2 | FLAG

    infoFrame[totalBytes++] = FLAG; // Determines the end of the array

    printf("\n\n\n\n\n\n\n\n\n\n\n ---- Information Frame (prepareInfoFrame - after stuffing) ---- \n\n");
    for (int i = 0; i < totalBytes; i++) {
        printf("infoFrame[%d] = %02x\n", i, infoFrame[i]);
    }
    printf("\n\n\n\n\n\n\n\n\n\n\n");

    return totalBytes;
}


int readReceiverResponse(enum state *STATE2) {
    unsigned char buf;
    // printf("alarmEnabled: %d, state : %d\n", alarmEnabled, STATE);
    while (*STATE2 != STOP && alarmEnabled == TRUE) {
        int bytes = read(fd, &buf, 1);

        if (bytes > 0) {
            int res = changeStateSuperviseTrama(buf, STATE2);
            if (res == 0) continue;
            else if (res == 1 && currentC == C_S0) return 1;
            else if (res == 2 && currentC == C_S1) return 1;
            else if (res == 3 && currentC == C_S0) return -1;
            else if (res == 4 && currentC == C_S1) return -1;
            else return 0;
        }
    }

    return *STATE2;
}

int sendInfoFrame(unsigned char *infoFrame, int totalBytes) {
    alarmCount = 0;
    alarmEnabled = FALSE;

    do {
        enum state STATE;
        if (alarmEnabled == FALSE) {
            alarm(timeout);
            printf("totalBytes (link_layer : 425) = %d\n", totalBytes);
            int res = write(fd, infoFrame, totalBytes);
            printf("res (link_layer : 427) = %d\n", res);
            if (res < 0) {
                printf("ERROR: Failed to send infoFrame!\n");
                continue;
            }
        }

        // Read receiverResponse
        enum state STATE2 = START;
        int res = readReceiverResponse(&STATE2);
        switch (res) {
            case 0:
                break;
            case -1:
                alarmEnabled = FALSE;
                break;
            case 1:
                printf("Received infoFrame from llwrite() successfully!\n");
                return 0;
                
        }
        printf("alarmCount = %d\n", alarmCount);
        if (STATE == STOP) break;
    } while (alarmCount < nRetransmissions);

    return -1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////

/**
 * @brief 
 * 
 * @param buf (controlPacket created in the applicationLayer)
 * @param bufSize 
 * @return int 
 */
int llwrite(const unsigned char *buf, int bufSize) {
    // 1. Create the info frame
    // 2. Make byte stuffing of the data received (controlPacket - buf)

    unsigned char *infoFrame = malloc(sizeof(unsigned char) * (4 + (bufSize * 2) + 2));

    int totalBytes = prepareInfoFrame(buf, bufSize, currentC, infoFrame);

    printf("\n\n\n\n\n\n\n\n\n\n\n ---- Information Frame (llwrite) ---- \n\n");
    printf("totalBytes = %d\n", totalBytes);
    for (int i = 0; i < totalBytes; i++) {
        printf("infoFrame[%d] = %02x\n", i, infoFrame[i]);
    }
    printf("\n\n\n\n\n\n\n\n\n\n\n");

    if (sendInfoFrame(infoFrame, totalBytes) < 0) {
        printf("ERROR: Failed to write info frame!\n");
        return -1;
    }


    
    
    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////


int receiveInfoFrame(unsigned char *packet, unsigned char *buf) {
    int STATE = packSTART;
    char ch;
    int packetidx = 0;
    int currentPos = 0;
    int foundBCC1 = 0;

    while (STATE != packSTOP) {
        if (read(fd, &ch, 1) < 0) {
            printf("ERROR: Can't read from infoFrame!\n");
            continue;
        }

        printf("ch (link_layer : 504) = %02x\n", ch);
        STATE = changeInfoPacketState(STATE, ch, currentC, buf, &currentPos, &foundBCC1);
        printf("CURRENT_STATE = %d\n", STATE);
        printf("currentPos = %d\n", currentPos);


        switch (STATE) {
            case packSTOP:
                break;
            case packBCC1_RCV:
                printf("foundBCC1 ----------------\n");
                printf("foundBCC1 = %d\n", foundBCC1);
                if (foundBCC1 == 1) {
                    continue;
                }
                else {
                    printf("currentPos - 1 = %d | packetidx = %d\n", currentPos-1, packetidx);
                    packet[packetidx++] = buf[currentPos - 1];
                }
                break;
            case packTRANSPARENCY_RCV:
                break;
            case packERROR:
                break;
        }
    }


    for (int i = 0; i < 21; i++) {
        printf("buf[%d] = %02x\n", i, buf[i]);
    }

    return currentPos - 1;
}

int createRR(unsigned char *respondRR) {
    respondRR[0] = FLAG; // F
    respondRR[1] = A_SET; // A

    if (currentC == C_S0) { // C
        respondRR[2] = C_S1; 
        currentC = C_S1;
    }
    else if (currentC == C_S1) {
        respondRR[2] = C_S0; 
        currentC = C_S0;
    }
    else {
        printf("ERROR: Invalid currentC in createRR() function (link_layer.c)!\n");
        return -1;
    }

    respondRR[3] = respondRR[2]; // BCC1
    respondRR[4] = FLAG; // F

    return 0;
}

int sendRR(unsigned char *respondRR) {
    if (write(fd, respondRR, 5) < 0) {
        printf("ERROR: Failed to sendRR() (link_layer.c)!\n");
        return -1;
    }

    return 0;
}

void createREJ(unsigned char *respondREJ) {
    respondREJ[0] = FLAG; // F
    respondREJ[1] = A_SET; // A

    respondREJ[2] = currentC;

    respondREJ[3] = respondREJ[2]; // BCC1
    respondREJ[4] = FLAG; // F
}

int sendREJ(unsigned char *respondREJ) {
    if (write(fd, respondREJ, 5) < 0) {
        printf("ERROR: Failed to sendREJ() (link_layer.c)!\n");
        return -1;
    }

    return 0;
}

int llread(unsigned char *packet) {
    unsigned char buf[1000];

    int numBytesRead = receiveInfoFrame(packet, buf);


    // CHEGOU AQUI MUITO BEM!!!!!!!!!!!!!!!!!!

    // CREATE BCC2

    int bcc2Received = buf[numBytesRead + 1];
    int bcc2 = buf[0];
    for (int i = 1; i < numBytesRead; i++) {
        bcc2 ^= buf[i];
    }

    

    if (bcc2 == bcc2Received) { // Create RR
        unsigned char *respondRR;
        if (createRR(respondRR) < 0) {
            printf("ERROR: createRR() failed in llread (link_layer.c)!\n");
            return -1;
        }

        if (sendRR(respondRR) < 0) {
            printf("ERROR: sendRR() failed in llread (link_layer.c)!\n");
            return -1;
        }
    }
    else { // Create REJ
        unsigned char *respondREJ;
        createREJ(respondREJ);
        if (sendREJ(respondREJ) < 0) {
            printf("ERROR: sendREJ() failed in llread (link_layer.c)!\n");
            return -1;
        }
    }

    return numBytesRead;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    printf("------------------- LLCLOSE -------------------\n");

    alarmCount = 0;

    if (role == LlTx) {
        
    }

    return 1;
}
