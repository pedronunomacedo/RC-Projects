#pragma once
#include <stdio.h>
#include <string.h>

typedef struct {
    int role;
} stateMachineInfo;
enum state {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, STOP};
enum state changeState(enum state STATE, char ch, unsigned char A, unsigned char C);
void setMachineRole(int role);


