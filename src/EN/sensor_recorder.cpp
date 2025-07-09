#include "serial_sensor.h"
#include "sensor_settings.h"

#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <atomic>
#include <ctime>
#include <iomanip>
#include <iostream>

#ifndef _WIN32
#include <unistd.h>
#endif

static std::atomic<bool> is_recording(false);
static std::atomic<bool> recorder_started(false);  // Used to avoid thread conflicts
static std::thread recorder_thread;

#ifdef _WIN32

sensor_data_t get_latest_sensor_data() {
    sensor_data_t data;

    static bool seeded = false;
    if (!seeded) {
        std::srand(std::time(nullptr));
        seeded = true;
    }

    data.value1 = rand() % 100;
    data.value2 = rand() % 100;
    data.value3 = rand() % 100;

    return data;
}

void start_sensor_recording(bool simulate, int interval_sec) {
    // No implementation needed for simulation mode
}

#else
// Linux: Real sensor data
static sensor_data_t latest_data;

sensor_data_t get_latest_sensor_data() {
    return latest_data;
}

void start_sensor_recording(bool simulate, int interval_sec) {

    if (recorder_thread.joinable()) {
        recorder_thread.join();
    }
    if (is_recording.load())
        return;

    is_recording.store(true);
    recorder_started.store(false);

    recorder_thread = std::thread([simulate, interval_sec]() {
        if (!simulate) {
            // You must configure this part according to your sensor and device.
            if (!init_serial("xxx", 11111)) {
                std::cerr << "[WARNING] Sensor is not connected.\n";
                is_recording.store(false);
                recorder_started.store(false);
                return;
            }
        }

        recorder_started.store(true);

        std::ofstream logfile("/etc/sensor_data.txt", std::ios::app);
        if (!logfile.is_open()) {
            perror("Failed to open data file");
            is_recording.store(false);
            if (!simulate) close_serial();
            recorder_started.store(false);
            return;
        }

        while (is_recording.load()) {
            sensor_data_t data;

            if (simulate) {
                data = simulatepoll2();
            }
            else {
                data = getpoll2();
            }

            if (data.value1 == 0.0f && data.value2 == 0.0f && data.value3 == 0.0f) {
                std::this_thread::sleep_for(std::chrono::seconds(interval_sec));
                continue;
            }

            std::time_t now = std::time(nullptr);
            char timestamp[32];
            std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

            logfile << std::fixed << std::setprecision(6);
            logfile << timestamp;
            if (show_temperature)  logfile << ", Temp: " << data.value1;
            if (show_conductivity) logfile << ", Cond: " << data.value2;
            if (show_pressure)     logfile << ", Pres: " << data.value3;
            logfile << "\n";
            logfile.flush();

            latest_data = data;
            std::this_thread::sleep_for(std::chrono::seconds(interval_sec));
        }

        logfile.close();
        if (!simulate)
            close_serial();

        recorder_started.store(false);
        });
}
#endif  // _WIN32

bool is_sensor_recording() {
    return is_recording.load();
}

void stop_sensor_recording() {
    if (is_recording.load()) {
        is_recording.store(false);

        if (recorder_started.load() && recorder_thread.joinable()) {
            recorder_thread.join();
        }

        recorder_started.store(false);
    }

#ifndef _WIN32
    latest_data = { 0.0f, 0.0f, 0.0f };
#endif
}
