// Read from serial port in non-canonical mode
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

int fd;
unsigned char UA[5];

#define BUF_SIZE 256

volatile int STOP = FALSE;

void prepareUA() {
    UA[0] = FLAG;
    UA[1] = A;
    UA[2] = C_UA;
    UA[3] = A ^ C_UA;
    UA[4] = FLAG;
}

void sendUA() {
    int sentBytes = 0;
    sentBytes = write(fd, UA, BUF_SIZE);
    printf("Sent to writer [%x,%x,%x,%x,%x]\n", UA[0], UA[1], UA[2], UA[3], UA[4]);
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

    // Open serial port device for reading and writing and not as controlling tty
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
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 1;  // Blocking read until 1 chars received

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

    // Loop for input
    unsigned char buf[BUF_SIZE]; // +1: Save space for the final '\0' char

    while (STOP == FALSE)
    {
        
        // Returns after 5 chars have been input
        int bytes = read(fd, buf, BUF_SIZE);
        printf("SET received = [%x,%x,%x,%x,%x]\n", buf[0], buf[1], buf[2], buf[3], buf[4]);

        if (buf[0] == FLAG && buf[1] == A && buf[2] == C_SET && buf[3] == (A ^ C_SET) && buf[4] == FLAG) STOP = TRUE;
    }

    if (STOP == TRUE) {
        printf("Received SET successfully!\n");
    }

    prepareUA();

    sendUA();

    sleep(1); // wait until all the bytes have been written to the receiver



    // The while() cycle should be changed in order to respect the specifications
    // of the protocol indicated in the Lab guide

    // TESTING: The reader is going to write in the writer

    // If I want the reader to send a sentence to the writer
    // int bytes = write(fd, buf, strlen(buf)+1);
    // printf("%d bytes written\n", bytes);

    // Wait until all bytes have been written to the serial port
    sleep(1);

    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1) 
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}
