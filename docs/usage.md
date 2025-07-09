# Usage Guide

## Introduction

This document explains how to build, run, and use the LVGL-based embedded UI project. The system is designed to work both on real Linux-based embedded devices and in a Windows simulator. The interface has a modular structure, consisting of multiple screens, each responsible for specific system functions or data display.

## Requirements

### For Linux-Based Embedded Devices

All project testing has been performed using a custom Yocto-based Linux distribution. While the project is compatible with any Linux system, make sure that the required tools and permissions are available.

- Required packages: `iproute2`, `iw`, `wpa-supplicant`, `psmisc`, `net-tools`, `coreutils`, `grep`, `gawk`, `nano`
- LVGL version 9.1
- CMake version ≥ 3.13
- GCC / G++ with C++17 support
- Linux framebuffer support (e.g., `/dev/fb0`)
- Write permissions to the `/etc` directory

### For Windows Simulator

- Visual Studio IDE (tested with 2022 version)
  - "Desktop development with C++" workload
  - Windows 10 SDK or Windows 11 SDK
- Minimum 8 GB RAM
- LVGL version 9.1

## Building and Running

### For Linux Environment

You can follow the steps below to download and build the sample project.

```bash
git clone --recurse-submodules https://github.com/caganozaslan/embedded-lvgl-ui-example/src
cd PROJECT-FOLDER
mkdir build
cmake -B build -S .
make -C build -j
```

The compiled output will be located under the `bin` directory as an executable named **main**.

### For Windows Environment

Navigate to the folder where you want to set up the Windows simulator, then:

```bash
git clone --recurse-submodules https://github.com/lvgl/lv_port_pc_visual_studio.git
```

Follow the Windows simulator setup steps provided in the relevant repository to ensure the simulation environment is properly configured.

Open the **LvglWindowssimulator.cpp** file and locate the following lines. Then, comment them out.

```bash
lv_demo_widgets();
lv_demo_benchmark();
```

Next, include all the necessary header files from the project (in this case, all files containing `screen` definitions) at the top of the file using the `#include "live_data.h"` format.

From this point on, you can replace the commented-out demo section with your own function call.

The complete version of the modified file is shown below:


```cpp
#include <Windows.h>

#include <LvglWindowsIconResource.h>
#include "screen_manager.h"
#include "my_app.h"
#include "maingui.h"
#include "lvgl/lvgl.h"
#include "lvgl/examples/lv_examples.h"
#include "lvgl/demos/lv_demos.h"
#include "systeminfo.h"
#include "sensor_settings.h"
#include "live_data.h"
#include "average_data.h"
#include "settings_screen.h"


extern "C" {
    #include "maingui.h"
}
int main()
{
    lv_init();

    /*
     * Optional workaround for users who wants UTF-8 console output.
     * If you don't want that behavior can comment them out.
     *
     * Suggested by jinsc123654.
     */
#if LV_TXT_ENC == LV_TXT_ENC_UTF8
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

    int32_t zoom_level = 100;
    bool allow_dpi_override = false;
    bool simulator_mode = true;
    lv_display_t* display = lv_windows_create_display(
        L"LVGL Windows Simulator Display 1",
        800,
        480,
        zoom_level,
        allow_dpi_override,
        simulator_mode);
    if (!display)
    {
        return -1;
    }

    HWND window_handle = lv_windows_get_display_window_handle(display);
    if (!window_handle)
    {
        return -1;
    }

    HICON icon_handle = LoadIconW(
        GetModuleHandleW(NULL),
        MAKEINTRESOURCE(IDI_LVGL_WINDOWS));
    if (icon_handle)
    {
        SendMessageW(
            window_handle,
            WM_SETICON,
            TRUE,
            (LPARAM)icon_handle);
        SendMessageW(
            window_handle,
            WM_SETICON,
            FALSE,
            (LPARAM)icon_handle);
    }

    lv_indev_t* pointer_indev = lv_windows_acquire_pointer_indev(display);
    if (!pointer_indev)
    {
        return -1;
    }

    lv_indev_t* keypad_indev = lv_windows_acquire_keypad_indev(display);
    if (!keypad_indev)
    {
        return -1;
    }

    lv_indev_t* encoder_indev = lv_windows_acquire_encoder_indev(display);
    if (!encoder_indev)
    {
        return -1;
    }

    //lv_demo_widgets();
    //lv_demo_benchmark();
    
	
    create_main_screen();
    create_wifi_screen();
    create_system_info_screen();
    create_sensor_settings_screen(); 
    create_live_data_screen();
    create_average_data_screen();
    create_settings_screen();
    ScreenManager::get_instance().show_screen(0); 
    while (1)
    {
        uint32_t time_till_next = lv_timer_handler();
        lv_delay_ms(time_till_next);
    }

    return 0;
}
```

From this point on, place all project source files into the folder where the simulator is located, and start the project in **DEBUG** mode using Visual Studio IDE. The simulation window should now appear.

## Important Notes

- The sample application is configured for **framebuffer-based rendering**. Therefore, a desktop environment **should not be running** on the target device.  
  If you plan to use a desktop environment, you must switch the rendering backend to **SDL**.

- In framebuffer-based applications, **console output and the GUI share the same screen**.  
  To view the interface properly, console output should be disabled.

- If you add a new source file or remove an existing one, **don’t forget to update the list of compiled files in `CMakeLists.txt`**.

- This application has been tested on **Raspberry Pi 4**.  
  It has **not** been tested on lower-end hardware.

- Some backend operations use **C standard libraries**.  
  The application is **not solely dependent** on the LVGL library.
