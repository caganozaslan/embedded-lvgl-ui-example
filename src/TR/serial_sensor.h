#ifndef SERIAL_SENSOR_H
#define SERIAL_SENSOR_H

#include <stdbool.h>  // ✅ bool için gerekli

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct {
        float value1;
        float value2;
        float value3;
    } sensor_data_t;

    typedef struct {
        char** lines;
        int line_count;
    } poll3_result_t;

    bool init_serial(const char* device, int baudrate);  // ✅ Değiştirildi
    void close_serial();
    sensor_data_t getpoll2();
    poll3_result_t getpoll3();
    char* getpoll1();
    void free_poll3_result(poll3_result_t* result);

    char* simulatepoll1();
    sensor_data_t simulatepoll2();
    poll3_result_t simulatepoll3();

#ifdef __cplusplus
}
#endif

#endif
