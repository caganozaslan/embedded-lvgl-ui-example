#include "lvgl/lvgl.h"
#include "screen_manager.h"
#include "style.h"
#include "systeminfo.h"  
#include <thread>
#include <array>
#include <string>
#include "header.h"

#ifndef _WIN32
#include <cstdio>
#endif

static lv_obj_t* sysinfo_container = nullptr;
static std::array<lv_obj_t*, 7> info_labels;

#ifndef _WIN32
// Linux section
static std::string run_command(const char* cmd) {
    std::string result;
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "Command error";
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        result += buffer;
    pclose(pipe);
    result.erase(result.find_last_not_of("\n") + 1);
    return result;
}
#endif


void update_sysinfo_timer(lv_timer_t* timer) {
    std::array<std::string, 7> info_texts;

#ifdef _WIN32

    info_texts = {
        "Device: Windows Simulation",
        "CPU: Intel(R) Core(TM) i7",
        "Wi-Fi SSID: Simulated_WiFi",
        "IP: 192.168.1.100",
        "Time: 2025-06-17 14:25:00",
        "Kernel: Win32 NT",
        "Application: v0.1-dev"
    };
#else
    // Linux section
    auto safe_cmd = [](const char* cmd) {
        std::string out = run_command(cmd);
        return out.empty() ? "Data unavailable" : out;
        };

    info_texts = {
        "Device: " + safe_cmd("cat /proc/device-tree/model"),
        "CPU: " + safe_cmd("echo \"$(nproc)x $(grep 'CPU part' /proc/cpuinfo | head -n1 | awk '{print $4}')\""),
        "Wi-Fi SSID: " + safe_cmd("iw dev wlan0 link | grep SSID | awk '{print $2}'"),
        "IP: " + safe_cmd("ifconfig wlan0 | grep 'inet ' | awk '{print $2}'"),
        "Time: " + safe_cmd("date '+%Y-%m-%d %H:%M'"),
        "Kernel: " + safe_cmd("uname -r"),
        "Application: v0.1.0-dev"
    };
#endif

    for (size_t i = 0; i < info_labels.size(); ++i) {
        if (info_labels[i] && lv_obj_is_valid(info_labels[i])) {
            lv_label_set_text(info_labels[i], info_texts[i].c_str());
        }
    }
}


void update_sysinfo() {
    update_sysinfo_timer(nullptr);
}
void create_system_info_screen() {
    if (sysinfo_container) return;

    init_styles();
    sysinfo_container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(sysinfo_container, lv_pct(100), lv_pct(100));
    lv_obj_clear_flag(sysinfo_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(sysinfo_container, &style_screen_bg, 0);


    create_header(sysinfo_container);


    lv_obj_t* title = lv_label_create(sysinfo_container);
    lv_label_set_text(title, "System Info");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 35);


    lv_obj_t* info_container = lv_obj_create(sysinfo_container);
    lv_obj_set_size(info_container, lv_pct(80), LV_SIZE_CONTENT);
    lv_obj_align(info_container, LV_ALIGN_CENTER, 0, 20);
    lv_obj_clear_flag(info_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(info_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_column(info_container, 10, 0);
    lv_obj_set_style_bg_opa(info_container, LV_OPA_TRANSP, 0);


    for (size_t i = 0; i < info_labels.size(); ++i) {
        info_labels[i] = lv_label_create(info_container);
        lv_obj_add_style(info_labels[i], &style_label_white, 0);
    }

    lv_obj_t* back_btn = lv_btn_create(sysinfo_container);
    lv_obj_set_size(back_btn, 100, 40);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_add_style(back_btn, &style_button, 0);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        ScreenManager::get_instance().show_screen(0);
        }, LV_EVENT_CLICKED, NULL);

    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_center(back_label);
    lv_obj_add_style(back_label, &style_label_white, 0);


    ScreenManager::get_instance().register_screen(sysinfo_container, update_sysinfo);
    update_sysinfo();
    lv_timer_create(update_sysinfo_timer, 15000, NULL);
}
