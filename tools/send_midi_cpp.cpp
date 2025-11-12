#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <thread>

int main(int argc, char** argv) {
    const char* port = "/dev/cu.usbserial-0001";
    if (argc > 1) port = argv[1];

    int fd = open(port, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    struct termios tty{};
    if (tcgetattr(fd, &tty) != 0) {
        perror("tcgetattr");
        close(fd);
        return 1;
    }

    cfmakeraw(&tty);
    // MIDI baudrate 31250 is not standard in termios; use B38400 then set custom divisor if supported.
    cfsetospeed(&tty, B38400);
    cfsetispeed(&tty, B38400);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
    tty.c_iflag &= ~IGNBRK; // disable break processing
    tty.c_lflag = 0; // no signaling chars, no echo, no canonical processing
    tty.c_oflag = 0; // no remapping, no delays
    tty.c_cc[VMIN]  = 0; // read doesn't block
    tty.c_cc[VTIME] = 5; // 0.5 seconds read timeout

    tty.c_cflag |= CLOCAL | CREAD; // ignore modem controls, enable reading
    tty.c_cflag &= ~(PARENB | PARODD); // no parity
    tty.c_cflag &= ~CSTOPB; // 1 stop bit
    tty.c_cflag &= ~CRTSCTS; // no flow control

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        close(fd);
        return 1;
    }

    // Prepare MIDI bytes: Note ON channel 0 = 0x90, note=48 (C3), vel=100
    uint8_t on[] = {0x90, 48, 100};
    uint8_t off[] = {0x80, 48, 0};

    ssize_t w = write(fd, on, sizeof(on));
    if (w != (ssize_t)sizeof(on)) perror("write on");
    else printf("Sent NOTE ON to %s\n", port);

    // wait 60ms
    std::this_thread::sleep_for(std::chrono::milliseconds(60));

    w = write(fd, off, sizeof(off));
    if (w != (ssize_t)sizeof(off)) perror("write off");
    else printf("Sent NOTE OFF to %s\n", port);

    close(fd);
    return 0;
}
