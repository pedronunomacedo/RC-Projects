#include "../include/state.h"
#include <string.h>

stateMachineInfo stateMachine;

#define TRUE 1
#define FALSE 0

#define FLAG 0x7E
#define A_SET 0x03
#define A_UA 0x01
#define C_SET 0x03
#define C_UA 0x07

enum state changeState(enum state STATE, char ch, unsigned char A, unsigned char C) {

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
            printf("ERROR: Invalid state!\n");
            return -1;
            break;
    }

    return STATE;
}
