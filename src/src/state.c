#include "../include/state.h"
#include <string.h>

stateMachineInfo stateMachine;

#define TRUE 1
#define FALSE 0

#define FLAG 0x7E
#define A_SET 0x03
#define A_UA 0x03
#define C_SET 0x03
#define C_UA 0x07

#define C_S0 0x00 // transmitter -> receiver
#define C_S1 0x40 // receiver -> transmitter
#define RR0 0x05
#define RR1 0x85
#define REJ0 0x01
#define REJ1 0x81

enum state changeState(enum state STATE, unsigned char ch, unsigned char A, unsigned char C) {
    switch (STATE) {
        case START: // Start node (waiting fot the FLAG)
            if (ch == FLAG) {
                STATE = FLAG_RCV; // Go to the next state
            } // else { stay in the same state }
            break;
        case FLAG_RCV: // State Flag RCV
            if (ch == A) {
                STATE = A_RCV; // Go to the next state
            }
            else if (ch == FLAG) {
                STATE = FLAG_RCV; // Stays on the same state
            }
            else {
                STATE = START; // other character received goes to the initial state
            }
            break;
        case A_RCV: // State A RCV
            if (ch == C) {
                STATE = C_RCV; // Go to the next state
            }
            else if (ch == FLAG) {
                STATE = FLAG_RCV;
            }
            else {
                STATE = START;
            }
            break;
        case C_RCV: // State C RCV
            if (ch == (A ^ C)) {
                STATE = BCC_OK; // Go to the next state
            }
            else if (ch == FLAG) {
                STATE = FLAG_RCV;
            }
            else {
                STATE = START;
            }
            break;
        case BCC_OK: // State BCC_OK
            if (ch == FLAG) {
                STATE = STOP; // Go to the final state
            }
            else {
                STATE = START;
            }
            break;
        default:
            break;
    }

    return STATE;
}


enum stateInfoPacket changeInfoPacketState(enum stateInfoPacket STATE, unsigned char ch, int senderNumber, unsigned char *buf, int *currentPos, int *foundBCC1, int *transparencyElse) {
    switch (STATE) {
        case packSTART:
            if (ch == FLAG) {
                STATE = packFLAG1_RCV;
            }
            break;
        case packFLAG1_RCV:
            if (ch == A_SET) {
                STATE = packA_RCV;
            }
            else if (ch == FLAG) {
                STATE = packFLAG1_RCV;
            }
            else {
                STATE = packSTART; // other character received goes to the initial state
            }
            break;
        case packA_RCV:
            if (ch == (senderNumber << 6)) {
                STATE = packC_RCV;
            }
            else if (ch == FLAG) {
                STATE = packFLAG1_RCV;
            }
            else {
                STATE = packSTART;
            }
            break;
        case packC_RCV:;
            char expectedBCC1 = (A_SET ^ (senderNumber << 6));
            if (ch == expectedBCC1) {
                *foundBCC1 = 1;
                STATE = packBCC1_RCV;
                currentPos = 0;
            }
            else if (ch == FLAG) {
                STATE = packFLAG1_RCV;
            }
            else {
                STATE = packSTART;
            }
            break;
        case packBCC1_RCV:
            if (ch == 0x7D) {
                STATE = packTRANSPARENCY_RCV;
            }
            else if (ch == FLAG) {
                STATE = packSTOP;
            }
            else {
                buf[(*currentPos)++] = ch;
                STATE = packBCC1_RCV;
            }
            break;
        case packTRANSPARENCY_RCV:
            if (ch == 0x5E) {
                STATE = packBCC1_RCV;
                buf[(*currentPos)++] = FLAG;
            }
            else if (ch == 0x5D) {
                STATE = packBCC1_RCV;
                buf[(*currentPos)++] = 0x7D; // octeto
            }
            else {
                STATE = packBCC1_RCV;
                buf[(*currentPos)++] = 0x7D;
                buf[(*currentPos)++] = ch;
                *transparencyElse = TRUE;
            }
            break;
        default:
            break;
    }

    return STATE;
}


unsigned char superviseTrama[2] = {0};

int changeStateSuperviseTrama(unsigned char buf, enum state *STATE) {
    switch ((*STATE)) {
        case START: // Start node (waiting fot the FLAG)
            if (buf == FLAG) {
                STATE = FLAG_RCV; // Go to the next state
            }
            break;
        case FLAG_RCV: // State Flag RCV
            if (buf == A_SET) {
                STATE = A_RCV; // Go to the next state
                superviseTrama[0] = buf;
            }
            else if (buf == FLAG) {
                STATE = FLAG_RCV; // Stays on the same state
            }
            else {
                STATE = START; // other character received goes to the initial state
            }
            break;
        case A_RCV: // State A RCV
            if (buf == RR1) {
                STATE = C_RCV; // Go to the next state
                superviseTrama[1] = buf;
            }
            else if (buf == FLAG) {
                STATE = FLAG_RCV;
            }
            else {
                STATE = START;
            }
            break;
        case C_RCV: // State C RCV
            if (buf == (superviseTrama[0] ^ superviseTrama[1])) {
                STATE = BCC_OK; // Go to the next state
            }
            else if (buf == FLAG) {
                STATE = FLAG_RCV;
            }
            else {
                STATE = START;
            }
            break;
        case BCC_OK: // State BCC_OK
            if (buf == FLAG) {
                STATE = START; // Go to the final state
                switch (superviseTrama[1]) {
                    case RR0:
                        return 1;
                    case RR1:
                        return 2;
                    case REJ0:
                        return 3;
                    case REJ1:
                        return 4;
                }
            }
            else {
                STATE = START;
            }
            break;
        default:
            printf("default\n");
            break;
    }

    return 0;
}