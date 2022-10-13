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
#define FLAG 0x7E
#define A 0x03
#define C_SET 0x03
#define C_UA 0x07

#define START_STATE 0
#define FLAG_STATE 1
#define A_STATE 2
#define C_STATE 3
#define BCC_STATE 4
#define STOP_STATE 5

struct termios oldtio;
struct termios newtio;

int llopen(char* serialPortName) {
    // Open serial port device for reading and writing and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(serialPortName, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
    }

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
    newtio.c_cc[VMIN] = 1;  // Blocking read until 5 chars received

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

    return fd;
}


void ttclose (int fd) {
    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);
}


void read_cmd(int fd, int set) {
    int state = START_STATE;
    char curr_char;
    char* ret = malloc(sizeof(char)*2);
    
    while (state != STOP_STATE) {
        int res = read(fd, &curr_char, 1); // Read one character
        printf("read %x from state %d\n", curr_char, state);

        switch (state){
            case START_STATE:
                if (curr_char == FLAG) state = FLAG_STATE;
                break;
            case FLAG_STATE:
                if (curr_char == A) state = A_STATE;
                else if (curr_char == FLAG_STATE) state = FLAG_STATE;
                else state = START_STATE;
                break;
        case A_STATE:
            if ((set == 1 && curr_char == C_SET) || (set == 0 && curr_char == C_UA)) state = C_STATE;
            else if (curr_char == FLAG) state = FLAG_STATE;
            else state = START_STATE;
            break;
        case C_STATE:
            if ((set == 1 && curr_char == (A ^ C_SET)) || (set == 0 && curr_char == (A ^ C_UA))) state = BCC_STATE;
            else if (curr_char == FLAG) state = FLAG_STATE;
            else state = START_STATE;
            break;
        case BCC_STATE:
            if (curr_char == FLAG) state = STOP_STATE;
            else state = START_STATE;
            break;
        default:
            break;
        }
    }
}