# Application Architecture

## Project Purpose

This educational sample project aims to create a full-screen, touch-enabled user interface for embedded Linux devices using LVGL (Light and Versatile Graphics Library). It covers components such as Wi-Fi configuration, sensor data monitoring, system information, style management, settings, and data logging.

## Overall System Structure

The project consists of three main modules:

1. **User Interface (GUI):**
   - Pages written with LVGL  
   - Screen transitions managed via `ScreenManager`  
   - Centralized style management in `style.cpp`

2. **Background System Functions:**
   - Wi-Fi connection and network scanning (`systemfunctions.cpp`)  
   - System-time synchronization, IP retrieval, shutdown operations  
   - Auto-connect mechanism and logging

3. **Sensor Subsystem:**
   - Data acquisition over RS-485 (`serial_sensor.cpp`)  
   - Simulation-mode support  
   - Writing live and historical data to file (`sensor_recorder.cpp`)  
   - Sensor settings (`sensor_settings.cpp`)

## File and Layer Structure

| File                   | Description                                                   |
|------------------------|---------------------------------------------------------------|
| `main.cpp`             | Application entry point, loads the first screen              |
| `maingui.cpp`          | Main home screen shown at startup                            |
| `screen_manager.cpp`   | Singleton class that handles screen switching                |
| `style.cpp`            | LVGL style definitions and centralized color/font control     |
| `header.cpp`           | Header bar displayed at the top of every screen              |
| `wifi_settings.cpp`    | UI for Wi-Fi scanning, connecting, and password entry        |
| `sensor_settings.cpp`  | Sensor-configuration page (simulation, interval, data types) |
| `sensor_recorder.cpp`  | Thread that reads and logs real/simulated sensor data        |
| `serial_sensor.cpp`    | RS-485 data-acquisition functions                            |
| `live_data.cpp`        | Screen displaying live sensor values                         |
| `average_data.cpp`     | Average data, user-defined statistics, and charts            |
| `systeminfo.cpp`       | Device information (CPU, Wi-Fi, time, kernel, etc.)          |
| `settings_screen.cpp`  | Central page for application-wide settings                   |
| `systemfunctions.cpp`  | Wi-Fi management, time sync, device shutdown, logging        |

## Interface Architecture

- All pages are created full-screen using `lv_obj_create()`.
- `create_header()` adds the top header bar to each page.
- Screens are registered via `ScreenManager::register_screen()`.
- `style.cpp` contains predefined styles like `style_button`, `style_title`, etc.
- Platform-specific logic is handled with `#ifdef _WIN32` blocks.

## Wi-Fi Management

- `wifi_settings.cpp` scans and lists available networks.
- Users can enter SSID and password to connect.
- Saved networks are stored in `/etc/known_networks.txt`.
- Connection status is checked every 15 seconds using `wifi_status_monitor_cb()` in `systemfunctions.cpp`.
- Auto-reconnection is supported.

## Sensor System

- On real devices, data is read via RS-485 from `/dev/ttyUSB0`.
- Simulation mode can be used on Windows or Linux for testing purposes.
- Data is logged to `/etc/sensor_data.txt`.
- Data types: temperature, conductivity, pressure.
- A background thread in `sensor_recorder.cpp` handles data acquisition.

## Data Visualization

### Live Data (`live_data.cpp`)

- Displays the most recent sensor values in large font.
- Reads data from a background polling thread.

### Average Data (`average_data.cpp`)

- Calculates averages over the last X entries or last X minutes.
- Users can select which data (e.g., temperature, pressure) to include.
- Uses LVGL chart widget for graphical display.

## System Information

- `systeminfo.cpp` lists essential system data:
  - Device model, CPU cores, Wi-Fi SSID, time, IP address, kernel version
- Data is retrieved via Linux commands using `popen` with tools like `grep` or `awk`.

## Memory and Power Settings

- The `settings_screen.cpp` allows users to configure auto Wi-Fi, SSH access, screen timeout, night mode, and more.
- All settings are saved to `/etc/settings.txt`.

## Flow Diagram

1. `main.cpp` → Initializes `ScreenManager` → Displays main GUI from `maingui.cpp`  
2. User navigates between screens using buttons  
3. UI elements modify settings or call system functions  
4. In the background:  
   - Periodic timers update Wi-Fi status and system info  
   - Threads log sensor data continuously

## Multitasking and Concurrency

- Sensor data is acquired using a dedicated `std::thread`.
- Wi-Fi connection is handled asynchronously, with a callback invoked on completion (`on_done`).
- GUI updates are triggered with `lv_async_call()` to ensure safe rendering.

## Platform Compatibility

- Windows simulation is supported via `#ifdef _WIN32`.
- Simulation mode uses dummy data.
- On real devices, Linux tools like `/dev`, `ifconfig`, and `wpa_supplicant` are used for system access.

## File Communication

| File Path                 | Contents                             |
|---------------------------|--------------------------------------|
| `/etc/settings.txt`       | All user settings                    |
| `/etc/sensor_data.txt`    | Timestamped sensor data log         |
| `/etc/logs.txt`           | System logs and warnings            |
| `/etc/known_networks.txt` | Stored SSID:Password pairs          |

## Development Notes

- LVGL version: **9.1.0**
- The application is still under **active development**
- Fonts used: `lv_font_montserrat_20` and `22`
- The project uses **CMake** and supports both real hardware and Windows simulation
