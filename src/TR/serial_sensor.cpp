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
        perror("Seri port açılamadı");
        return false;
    }

    struct termios tty;
    if (tcgetattr(serial_fd, &tty) != 0) {
        perror("tcgetattr hatası");
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
        perror("tcsetattr hatası");
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
    const char* command = "1 poll2\r\n";
    write(serial_fd, command, strlen(command));
    sleep(1);

    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    int n = read(serial_fd, buffer, sizeof(buffer) - 1);

    sensor_data_t data = { 0 };

    if (n <= 0) {
        fprintf(stderr, "Veri okunamadı! read() sonucu: %d\n", n);
        return data;
    }

    buffer[n] = '\0';

    int count = sscanf(buffer, "%f %f %f", &data.value1, &data.value2, &data.value3);
    if (count != 3) {
        fprintf(stderr, "sscanf başarısız! Okunan float sayısı: %d\n", count);
        return data;
    }

    return data;
}

poll3_result_t getpoll3() {
    const char* command = "1 poll3\r\n";
    write(serial_fd, command, strlen(command));
    sleep(1);

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    int n = read(serial_fd, buffer, sizeof(buffer) - 1);
    if (n <= 0) {
        fprintf(stderr, "[POLL3] Sensörden veri okunamadı!\n");
        poll3_result_t empty = { nullptr, 0 };
        return empty;
    }

    buffer[n] = '\0';

    poll3_result_t result;
    result.lines = nullptr;
    result.line_count = 0;

    char* line = strtok(buffer, "\r\n");
    while (line) {
        result.lines = static_cast<char**>(
            realloc(result.lines, sizeof(char*) * (result.line_count + 1))
            );
        result.lines[result.line_count] = strdup(line);
        result.line_count++;
        line = strtok(nullptr, "\r\n");
    }

    return result;
}

char* getpoll1() {
    const char* command = "1 poll1\r\n";
    write(serial_fd, command, strlen(command));
    sleep(1);

    static char buffer[128];
    memset(buffer, 0, sizeof(buffer));
    int n = read(serial_fd, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n] = '\0';
        return buffer;
    }
    else {
        return nullptr;
    }
}

void free_poll3_result(poll3_result_t* result) {
    for (int i = 0; i < result->line_count; ++i) {
        free(result->lines[i]);
    }
    free(result->lines);
    result->lines = nullptr;
    result->line_count = 0;
}

#endif // !_WIN32

// ------------------
// WINDOWS tarafı (simülasyon)
// ------------------

char* simulatepoll1() {
    static char response[] = "419B8BFF41F5DE5DB69FF0CD";
    return response;
}

sensor_data_t simulatepoll2() {
    sensor_data_t d;
    srand(time(nullptr));
    d.value1 = 19.0f + ((rand() % 1000) / 1000.0f);
    d.value2 = 30.5f + ((rand() % 500) / 1000.0f);
    d.value3 = -0.000010f + ((rand() % 20 - 10) / 1000000.0f);
    return d;
}

poll3_result_t simulatepoll3() {
    poll3_result_t result;
    result.line_count = 25;
    result.lines = static_cast<char**>(malloc(sizeof(char*) * result.line_count));

    result.lines[0] = strdup("EC is active");
    result.lines[1] = strdup("EXC V: 2.000000");
    result.lines[2] = strdup("EXC FREQ: 20000.000000");
    result.lines[3] = strdup("EXC setup time: 10.000000");
    result.lines[4] = strdup("EXC hold time: 1.000000");
    result.lines[5] = strdup("TEMP COEF: 2.000000");
    result.lines[6] = strdup("TEMP CUTOFF: 0.000000");
    result.lines[7] = strdup("cell K: 0.630000");

    srand(time(nullptr));
    int adc0 = 32000 + rand() % 1000;
    int adc1 = 11000 + rand() % 500;

    double ipp_pos = -2.0e-6 + ((rand() % 100) - 50) / 1e7;
    double ipp_neg = -1.9e-6 + ((rand() % 100) - 50) / 1e7;
    double vpp_pos = -7.3e-4 + ((rand() % 100) - 50) / 1e5;
    double vpp_neg = 3.1e-2 + ((rand() % 100) - 50) / 1e3;
    double temp = 30.6 + ((rand() % 100) - 50) / 100.0;
    double cond = 2.1e-6 + ((rand() % 100) - 50) / 1e7;
    double pres = 19.4 + ((rand() % 100) - 50) / 100.0;

    char buf[128];
    snprintf(buf, sizeof(buf), "ADC0 hits: %d", adc0);
    result.lines[8] = strdup(buf);
    result.lines[9] = strdup("+I gain: 1");
    snprintf(buf, sizeof(buf), "+Ip-p: %.6e", ipp_pos);
    result.lines[10] = strdup(buf);
    result.lines[11] = strdup("-I gain: 1");
    snprintf(buf, sizeof(buf), "-Ip-p: %.6e", ipp_neg);
    result.lines[12] = strdup(buf);
    result.lines[13] = strdup("+V gain: 1");
    snprintf(buf, sizeof(buf), "+Vp-p: %.6e", vpp_pos);
    result.lines[14] = strdup(buf);
    result.lines[15] = strdup("-V gain: 1");
    snprintf(buf, sizeof(buf), "-Vp-p: %.6e", vpp_neg);
    result.lines[16] = strdup(buf);
    snprintf(buf, sizeof(buf), "ADC1 hits: %d", adc1);
    result.lines[17] = strdup(buf);
    result.lines[18] = strdup("RTD: PT100");
    result.lines[19] = strdup("RTD wire: 3 wire");
    result.lines[20] = strdup("");
    snprintf(buf, sizeof(buf), "temperature: %.6f", temp);
    result.lines[21] = strdup(buf);
    snprintf(buf, sizeof(buf), "conductivity: %.6e", cond);
    result.lines[22] = strdup(buf);
    snprintf(buf, sizeof(buf), "pressure: %.6f", pres);
    result.lines[23] = strdup(buf);
    result.lines[24] = strdup("");

    return result;
}
