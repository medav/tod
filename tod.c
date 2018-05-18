#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <linux/serial.h>
#include <errno.h>

// command line args
#define EXPORT

static int fd = 0;

//
// The following functions, set_baud_rate_divisor and setup_serial_port,
// were adapted from a post on GitHub under the MIT license authored by 
// Cliff Brake.
//
// URL: https://github.com/cbrake/linux-serial-test/blob/master/linux-serial-test.c
//

void set_baud_divisor(int speed)
{
    // default baud was not found, so try to set a custom divisor
    struct serial_struct ss;
    if (ioctl(fd, TIOCGSERIAL, &ss) != 0) {
        printf("TIOCGSERIAL failed\n");
        exit(1);
    }

    ss.flags = (ss.flags & ~ASYNC_SPD_MASK) | ASYNC_SPD_CUST;
    ss.custom_divisor = (ss.baud_base + (speed/2)) / speed;
    int closest_speed = ss.baud_base / ss.custom_divisor;

    if (ioctl(fd, TIOCSSERIAL, &ss) < 0) {
        printf("TIOCSSERIAL failed\n");
        exit(1);
    }
}

int setup_serial_port(int baud)
{
    struct termios newtio;

    fd = open("/dev/ttyUSB0", O_RDWR | O_NONBLOCK);

    if (fd < 0) {
        printf("Error opening serial port \n");
        exit(1);
    }

    memset(&newtio, 0x00, sizeof(newtio));

    // man termios get more info on below settings
    newtio.c_cflag = baud | CS8 | CLOCAL | CREAD;

    newtio.c_iflag = 0;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;
    newtio.c_cc[VMIN] = 128;
    newtio.c_cc[VTIME] = 5;

    /* now clean the modem line and activate the settings for the port */
    tcflush(fd, TCIOFLUSH);
    tcsetattr(fd,TCSANOW,&newtio);
}

//
// Functions to be called from client code.
//
// N.B. EXPORT is #def'd as nothing but since I (Mike) am a Windows user, I put
//      this here to denote where a __declspec(dllexport) would be used on when
//      compiling this as a  Windows DLL (Obviously you couldn't use linux
//      serial though!)
//

EXPORT void Open() {
    setup_serial_port(B38400);

    //
    // The MRI Core motor controller operate at 256 kBaud which is non-standard
    // so we need to set a custom divisor to get close enough.
    //

    set_baud_divisor(256000);

    //
    // The MRI Core motor controllers have some magic setup bytes that I don't
    // quite remember the details of. These were captured using WireShark and
    // WinPcap to sniff USB IRPs.
    //

    unsigned char init[] = { 0x55, 0xAA, 0x80, 0x00, 0x03 };
    unsigned char idk[] = { 0x55, 0xAA, 0x80, 0x54, 0x02 };

    write(fd, init, 5);
    write(fd, idk, 5);
}

EXPORT void SetMotor(int channel, int value) {
    if (value < -100) value = -100;
    if (value > 100) value = 100;

    //
    // N.B. The payload layout here was reverse-engineered mostly by "guess and
    // check" with the actual MRI Core Library. Luckily the protocol was serial
    // and not obfuscated in any way so it was pretty easy to just scrape out
    // bytes and spit them out.
    //

    unsigned char payload[] = { 0x55, 0xAA, 0x00, 0x45 + channel, 0x01, (unsigned char)value };
    write(fd, payload, 6);
}

EXPORT void Close() {
    close(fd);
}
