// Link layer protocol implementation

#include "../include/link_layer.h"
#include "fcntl.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <termios.h>
#include <stdlib.h>

#define FALSE 0
#define TRUE 1

#define FLAG 0x7E
#define A 0x03
#define C_SET 0x03
#define C_UA 0x07

#define BUF_SIZE 256
#define BAUDRATE B38400

volatile int STOP = FALSE;
unsigned char SET[5];
unsigned char UA[5];
unsigned char buf[BUF_SIZE];
static int numTries = 0;
int role;
int fd; // serial file descriptor
LinkLayer connectionParametersGlobal;

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source


void prepareSet() {
    SET[0] = FLAG;
    SET[1] = A;
    SET[2] = C_SET;
    SET[3] = A ^ C_SET;
    SET[4] = FLAG;
}

int sendSet(int fd) {
    int sentBytes = 0;
    sentBytes = write(fd, SET, BUF_SIZE);
    printf("Sent to reader [%x,%x,%x,%x,%x]\n", SET[0], SET[1], SET[2], SET[3], SET[4]);

    return sentBytes;
}

void prepareUA() {
    UA[0] = FLAG;
    UA[1] = A;
    UA[2] = C_UA;
    UA[3] = A ^ C_UA;
    UA[4] = FLAG;
}

int sendUA(int fd) {
    int sentBytes = 0;
    sentBytes = write(fd, UA, BUF_SIZE);
    printf("Sent to writer [%x,%x,%x,%x,%x]\n", UA[0], UA[1], UA[2], UA[3], UA[4]);

    return sentBytes;
}

void receiveARRAY(int fd, unsigned char SET[]) {
    int state = 0;
    unsigned char ch;

    while (state != 5) { // While it doesn't get to the end of the state machine
        read(fd, &ch, 1); // Read one char
        switch (ch) {
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
                    STOP = TRUE;
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

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    connectionParametersGlobal = connectionParameters;
    int fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);

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
    newtio.c_cc[VTIME] = 1; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    switch(connectionParameters.role) {
        case TRANSMITTER:
            numTries = 0;
            role = TRANSMITTER;
            setMachineRole(TRANSMITTER);
            do {
                // Write SET
                if (sendReadyToTransmitMsg(fd) < 0) {
                    printf("ERROR: Failed to send SET!\n");
                }

                alarm(3);

                // Read UA
                role = RECEIVER;
                setMachineRole(TRANSMITTER);
                prepareUA();
                receiveARRAY(fd, UA);
            } while (numTries <= connectionParameters.nRetransmissions); // Falta o timeout
            break;
        case RECEIVER:
            numTries = 0;
            role = RECEIVER;
            setMachineRole(RECEIVER);
            prepareSet();

            do {
                receiveARRAY(fd, SET);
                if (sendReadyToReceiveMsg(fd) < 0) {
                    printf("ERROR: Failed to send UA!\n");
                }
            } while (numTries <= connectionParameters.nRetransmissions); // Falta o timeout

            break;
        default:
            printf("ERROR: Unknown role!\n");
            exit(1);
    }
    
    return fd;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    if (write(connectionParametersGlobal.role, buf, bufSize) < 0) return -1;
    
    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    // TODO

    return 1;
}
