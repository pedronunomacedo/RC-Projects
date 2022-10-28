#pragma once
#include <stdio.h>
#include <string.h>

typedef struct {
    int role;
} stateMachineInfo;

enum state {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, STOP};
enum stateInfoPacket {packSTART, packFLAG1_RCV, packA_RCV, packC_RCV, packBCC1_RCV, packTRANSPARENCY_RCV, packDATA_RCV, packBCC2_RCV, packFLAG2_RCV, packSTOP, packERROR};

enum state changeState(enum state STATE, unsigned char ch, unsigned char A, unsigned char C);
enum stateInfoPacket changeInfoPacketState(enum stateInfoPacket STATE, unsigned char ch, int senderNumber, unsigned char *buf, int *currentPos, int *foundBCC1, int *transparencyElse);

/**
* Returns 0 if the entire flag wasn't read yet.
* Returns 1 if RR0 received.
* Returns 2 if RR1 received.
* Returns 3 if REJ0 received.
* Returns 4 if REJ1 received.
*/

int changeStateSuperviseTrama(unsigned char buf, enum state *STATE);
void setMachineRole(int role);


