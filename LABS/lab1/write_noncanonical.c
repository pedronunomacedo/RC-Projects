// Write to serial port in non-canonical mode
//
// Modified by: Eduardo Nuno Almeida [enalmeida@fe.up.pt]

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include "utils.c"


// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1
#define FLAG 0x7E
#define A 0x03
#define C_SET 0x03
#define C_UA 0x07

#define BUF_SIZE 256

volatile int STOP = FALSE;
unsigned char SET[5];
unsigned char UA[5];
int fd; // serial file descriptor

void prepareSet() {
    SET[0] = FLAG;
    SET[1] = A;
    SET[2] = C_SET;
    SET[3] = A ^ C_SET;
    SET[4] = FLAG;
}

void sendSet() {
    int sentBytes = 0;
    sentBytes = write(fd, SET, BUF_SIZE);
    printf("Sent to reader [%x,%x,%x,%x,%x]\n", SET[0], SET[1], SET[2], SET[3], SET[4]);
}

void receiveUA(int fd) {
    int state = 0;
    unsigned char ch;

    while (state != 5) { // While it doesn't get to the end of the state macgine
        read(fd, &ch, 1); // Read one char
        switch (ch)
        {
        case 0: // Start node (waiting fot the FLAG)
            if (ch == UA[0]) {
                state = 1; // Go to the next state
            } // else { stay in the same state }
            break;
        case 1: // State Flag RCV
            if (ch == UA[1]) {
                state = 2; // Go to the next state
            }
            else if (ch == UA[0]) {
                state = 1; // Stays on the same state
            }
            else {
                state = 0; // other character received goes to the initial state
            }
            break;
        case 2: // State A RCV
            if (ch == UA[2]) {
                state = 3; // Go to the next state
            }
            else if (ch == UA[0]) {
                state = 1;
            }
            else {
                state = 0;
            }
            break;
        case 3: // State C RCV
            if (ch == UA[3]) {
                state = 4; // Go to the next state
            }
            else if (ch == UA[0]) {
                state = 1;
            }
            else {
                state = 0;
            }
            break;
        case 4: // State BCC_OK
            if (ch == UA[4]) {
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

int main(int argc, char *argv[])
{
    // Program usage: Uses either COM1 or COM2
    const char *serialPortName = argv[1];

    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0],
               argv[0]);
        exit(1);
    }

    // Open serial port device for reading and writing, and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    fd = open(serialPortName, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
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

    // In non-canonical mode, '\n' does not end the writing.
    // Test this condition by placing a '\n' in the middle of the buffer.
    // The whole buffer must be sent even with the '\n'.
    // buf[5] = '\n';

    prepareSet(); // prepare the SET array
    
    sendSet(); // Send SET to the writer
    
    sleep(1); // wait until all the bytes have been written to the reader

    // Read UA
    // Loop for input
    unsigned char buf[BUF_SIZE]; // +1: Save space for the final '\0' char

    while (STOP == FALSE)
    {
        
        // Returns after 5 chars have been input
        int bytes = read(fd, buf, BUF_SIZE);
        printf("UA received = [%x,%x,%x,%x,%x]\n", buf[0], buf[1], buf[2], buf[3], buf[4]);

        if (buf[0] == FLAG && buf[1] == A && buf[2] == C_UA && buf[3] == (A ^ C_UA) && buf[4] == FLAG) STOP = TRUE;
    }

    if (STOP == TRUE) {
        printf("Received UA successfully!\n");
    }


    // Wait until all bytes have been written to the serial port
    //sleep(1);

    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }




    // If I want to read the sentence the reader sent me
    // while (STOP == FALSE)
    // {
    //     // Returns after 5 chars have been input
    //     int bytes = read(fd, buf, BUF_SIZE);

    //     printf(":%s:%d\n", buf, bytes);
    //     if (buf[bytes] == '\0')
    //         STOP = TRUE;
    // }

    close(fd);

    return 0;
}
