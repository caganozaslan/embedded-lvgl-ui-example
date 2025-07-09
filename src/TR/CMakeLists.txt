cmake_minimum_required(VERSION 3.10)
project(lvgl)

set(CMAKE_C_STANDARD 99)#C99 # lvgl officially support C99 and above
set(CMAKE_CXX_STANDARD 17)#C17
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

add_subdirectory(lvgl)
target_include_directories(lvgl PUBLIC ${PROJECT_SOURCE_DIR})

add_executable(main
    main.cpp
    wifi_settings.cpp
    systemfunctions.cpp
    mouse_cursor_icon.c
    lvgl/src/drivers/evdev/lv_evdev.c
    style.cpp
    maingui.cpp
    screen_manager.cpp
    header.cpp
    systeminfo.cpp
    sensor_settings.cpp
    sensor_recorder.cpp
    serial_sensor.cpp
    live_data.cpp
    average_data.cpp
    settings_screen.cpp
    lvgl/src/others/snapshot/lv_snapshot.c
)

target_link_libraries(main lvgl lvgl::examples lvgl::demos lvgl::thorvg ${SDL2_LIBRARIES} m pthread)
add_custom_target (run COMMAND ${EXECUTABLE_OUTPUT_PATH}/main DEPENDS main)

