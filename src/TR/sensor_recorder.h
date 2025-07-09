#pragma once
#include "serial_sensor.h"


void start_sensor_recording(bool simulate, int interval_sec);

void stop_sensor_recording();

bool is_sensor_recording();
sensor_data_t get_latest_sensor_data();
extern sensor_data_t latest_data;
