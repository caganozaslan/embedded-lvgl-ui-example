#include "serial_sensor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef _WIN32
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdbool.h>
#endif

#ifndef _WIN32

static int serial_fd = -1;

bool init_serial(const char* device, int baudrate) {
    serial_fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
    if (serial_fd < 0) {
        perror("Failed to open serial port");
        return false;
    }

    struct termios tty;
    if (tcgetattr(serial_fd, &tty) != 0) {
        perror("tcgetattr error");
        close(serial_fd);
        serial_fd = -1;
        return false;
    }

    cfsetospeed(&tty, B19200);
    cfsetispeed(&tty, B19200);
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag &= ~IGNBRK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 10;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr(serial_fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr error");
        close(serial_fd);
        serial_fd = -1;
        return false;
    }

    return true;
}

void close_serial() {
    if (serial_fd >= 0) {
        close(serial_fd);
        serial_fd = -1;
    }
}

sensor_data_t getpoll2() {
    const char* command = "xxxx"; // You should replace this with the actual command to send to your sensor
    write(serial_fd, command, strlen(command));
    sleep(1);

    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    int n = read(serial_fd, buffer, sizeof(buffer) - 1);

    sensor_data_t data = { 0 };

    if (n <= 0) {
        fprintf(stderr, "Failed to read data! read() result: %d\n", n);
        return data;
    }

    buffer[n] = '\0';

    int count = sscanf(buffer, "%f %f %f", &data.value1, &data.value2, &data.value3);
    if (count != 3) {
        fprintf(stderr, "sscanf failed! Parsed float count: %d\n", count);
        return data;
    }

    return data;
}

#endif

// ------------------
// Shared section for both Windows and Linux
// ------------------

sensor_data_t simulatepoll2() {
    sensor_data_t d;
    srand(time(nullptr));
    d.value1 = 19.0f + ((rand() % 1000) / 1000.0f);
    d.value2 = 30.5f + ((rand() % 500) / 1000.0f);
    d.value3 = -0.000010f + ((rand() % 20 - 10) / 1000000.0f);
    return d;
}
