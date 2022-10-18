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

enum state changeState(enum state STATE, unsigned char ch, unsigned char A, unsigned char C) {
    printf("enter changeState()!\n");
    switch (STATE) {
        case START: // Start node (waiting fot the FLAG)
            printf("STATE = START\n");
            if (ch == FLAG) {
                printf("Received the FLAG!\n");
                STATE = FLAG_RCV; // Go to the next state
            } // else { stay in the same state }
            break;
        case FLAG_RCV: // State Flag RCV
            printf("STATE = FLAG_RCV\n");
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
            printf("STATE = A_RCV\n");
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
            printf("STATE = C_RCV\n");
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
            printf("STATE = BCC_OK\n");
            if (ch == FLAG) {
                STATE = STOP; // Go to the final state
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
    

    // Only to debug!
    switch (STATE)
    {
    case START:
        printf("Switched to state START!\n");
        break;
    case FLAG_RCV:
        printf("Switched to state FLAG_RCV!\n");
        break;
    case A_RCV:
        printf("Switched to state A_RCV!\n");
        break;
    case C_RCV:
        printf("Switched to state C_RCV!\n");
        break;
    case BCC_OK:
        printf("Switched to state BCC_OK!\n");
        break;
    default:
        printf("Enter STOP!\n");
        break;
    }
    ////////////////////////////////////////////////////////////////

    return STATE;
}
