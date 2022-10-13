#pragma once

#define FALSE 0
#define TRUE 1

#define TRANSMITTER 0
#define RECEIVER 1

#define MSG_FLAG 0x7E
#define OCT_ESCAPE 0x7d

// A - Campo de Endereço
#define MSG_A_TRANSMITTER_CMD 0x03
#define MSG_A_RECEIVER_RESP 0x03
#define MSG_A_RECEIVER_CMD 0x01
#define MSG_A_TRANSMITTER_RESP 0x01

// C - Campo de Controlo
#define MSG_C_SET 0x03 // Set up
#define MSG_C_DISC 0x0b // Disconnect
#define MSG_UA 0x07 // Unnumbered acknolegment
#define MSG_RR(n) ((n == 0) ? 0x05 : 0x85) // Receiver ready / Positive ACK :
                                           //   - if (n == 0) then (message is going to be sent on the Transmitter)
                                           //   - if (n == 1) then (message is going to be sent on the Receiver)
#define MSG_REJ(n) ((n == 0) ? 0x01 : 0x81) // Rejected / Negative ACK :
                                           //   - if (n == 0) then (message is going to be sent on the Transmitter)
                                           //   - if (n == 1) then (message is going to be sent on the Receiver)
#define MSG_BCC(n) ((n == 0) ? 0x01 : 0x81) // Campo de Proteção (cabeçalho)
