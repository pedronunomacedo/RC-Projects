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
    

    // Only to debug!
    // switch (STATE)
    // {
    // case START:
    //     printf("Switched to state START!\n");
    //     break;
    // case FLAG_RCV:
    //     printf("Switched to state FLAG_RCV!\n");
    //     break;
    // case A_RCV:
    //     printf("Switched to state A_RCV!\n");
    //     break;
    // case C_RCV:
    //     printf("Switched to state C_RCV!\n");
    //     break;
    // case BCC_OK:
    //     printf("Switched to state BCC_OK!\n");
    //     break;
    // default:
    //     printf("Enter STOP!\n");
    //     break;
    // }
    ////////////////////////////////////////////////////////////////

    return STATE;
}


enum stateInfoPacket changeInfoPacketState(enum stateInfoPacket STATE, unsigned char ch, int senderNumber, unsigned char *buf, int *currentPos, int *foundBCC1) {
    switch (STATE) {
        case packSTART:
            if (ch == FLAG) {
                // printf("\n\nThe char %02x is going to the pack1FLAG1_RCV!\n\n", ch);
                STATE = packFLAG1_RCV;
                buf[(*currentPos)++] = FLAG;
            } // else { stay in the same state }
            break;
        case packFLAG1_RCV:
            if (ch == A_SET) {
                // printf("\n\nThe char %02x is going to the packA_RCV!\n\n", ch);
                STATE = packA_RCV;
                buf[(*currentPos)++] = A_SET;
            }
            else if (ch == FLAG) {
                // printf("\n\nThe char %02x is staying in the packFLAG1_RCV!\n\n", ch);
                STATE = packFLAG1_RCV;
            }
            else {
                // printf("\n\nThe char %02x is going to the packSTART!\n\n", ch);
                STATE = packSTART; // other character received goes to the initial state
            }
            break;
        case packA_RCV:
            if (ch == (senderNumber << 6)) {
                // printf("\n\nThe char %02x is going to the packC_RCV!\n\n", ch);
                STATE = packC_RCV;
                buf[(*currentPos)++] = ch;
            }
            else if (ch == FLAG) {
                // printf("\n\nThe char %02x is going to the packFLAG1_RCV!\n\n", ch);
                STATE = packFLAG1_RCV;
            }
            else {
                // printf("\n\nThe char %02x is going to the packSTART!\n\n", ch);
                STATE = packSTART;
            }
            break;
        case packC_RCV:
            char expectedBCC1 = (A_SET ^ (senderNumber << 6));
            if (ch == expectedBCC1) {
                // printf("\n\nThe char %02x is going to the packBCC1_RCV!\n\n", ch);
                *foundBCC1 = 1;
                STATE = packBCC1_RCV;
                buf[(*currentPos)++] = ch;
                currentPos = 0;
            }
            else if (ch == FLAG) {
                // printf("\n\nThe char %02x is going to the packFLAG_RCV!\n\n", ch);
                STATE = packFLAG1_RCV;
            }
            else {
                // printf("\n\nThe char %02x is going to the packSTART!\n\n", ch);
                STATE = packSTART;
            }
            break;
        case packBCC1_RCV:
            if (ch == 0x7D) {
                // printf("\n\nThe char %02x is going to the packTRANSPARENCY_RCV!\n\n", ch);
                STATE = packTRANSPARENCY_RCV;
            }
            else if (ch == FLAG) {
                // printf("\n\nThe char %02x is going to the packSTOP!\n\n", ch);
                STATE = packSTOP;
                buf[(*currentPos)++] = FLAG;
            }
            else {
                // printf("\n\nThe char %02x is going to the packBCC1_RCV!\n\n", ch);
                buf[(*currentPos)++] = ch;
                STATE = packBCC1_RCV;
            }
            break;
        case packTRANSPARENCY_RCV:
            if (ch == 0x5E) {
                // printf("\n\nThe char %02x is going to the packBCC1_RCV!\n\n", ch);
                STATE = packBCC1_RCV;
                buf[(*currentPos)++] = FLAG;
            }
            else if (ch == 0x5D) {
                // printf("\n\nThe char %02x is going to the packBCC1_RCV!\n\n", ch);
                STATE = packBCC1_RCV;
                buf[(*currentPos)++] = 0x7D; // octeto
            }
            else {
                // printf("\n\nThe char %02x is going to the packBCC1_RCV!\n\n", ch);
                STATE = packBCC1_RCV;
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
            printf("STATE = START\n");
            if (buf == FLAG) {
                printf("Received the FLAG!\n");

                STATE = FLAG_RCV; // Go to the next state
            } // else { stay in the same state }
            break;
        case FLAG_RCV: // State Flag RCV
            printf("STATE = FLAG_RCV\n");
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
            printf("STATE = A_RCV\n");
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
            printf("STATE = C_RCV\n");
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
            printf("STATE = BCC_OK\n");
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
                printf("STOP\n");
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