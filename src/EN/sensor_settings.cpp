#include "sensor_settings.h"
#include "style.h"
#include "screen_manager.h"
#include "header.h"
#include "sensor_recorder.h"

#include <fstream>
#include <string>
#include <cstdlib>

// In this file, platform detection between Windows simulator and Linux device is done using IS_LINUX.
// This is implemented to demonstrate an alternative approach.

#ifdef _WIN32
#   define IS_LINUX false
#else
#   define IS_LINUX true
#endif

// Global variable definitions
bool simulation_enabled = false;
int  polling_interval_seconds = 10;
bool show_temperature = false;
bool show_conductivity = false;
bool show_pressure = false;

// Static UI objects
static lv_obj_t* screen = nullptr;
static lv_obj_t* ta_poll = nullptr;
static lv_obj_t* sw_sim = nullptr;
static lv_obj_t* cb_temp = nullptr;
static lv_obj_t* cb_cond = nullptr;
static lv_obj_t* cb_press = nullptr;
static lv_obj_t* keyboard = nullptr;
static lv_obj_t* btn_start = nullptr;

static constexpr const char* settings_path = "/etc/sensor_settings.txt";

// Event callbacks

static void start_data_collection_cb(lv_event_t* /*e*/) {
#if !defined(_WIN32)
    stop_sensor_recording();
    start_sensor_recording(simulation_enabled, polling_interval_seconds);
#endif
}

static void stop_data_collection_cb(lv_event_t* /*e*/) {
#if !defined(_WIN32)
    stop_sensor_recording();
#endif
}

// Load settings from file (Linux only)
static void load_settings() {
    if (!IS_LINUX) return;

    std::ifstream file(settings_path);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        if (line.rfind("SIM=", 0) == 0) simulation_enabled = (line.substr(4) == "1");
        else if (line.rfind("INTERVAL=", 0) == 0) polling_interval_seconds = std::stoi(line.substr(9));
        else if (line.rfind("TEMP=", 0) == 0) show_temperature = (line.substr(5) == "1");
        else if (line.rfind("COND=", 0) == 0) show_conductivity = (line.substr(5) == "1");
        else if (line.rfind("PRESS=", 0) == 0) show_pressure = (line.substr(6) == "1");
    }
}

// Save settings to file (Linux only)
static void save_settings() {
    if (!IS_LINUX) return;

    simulation_enabled = lv_obj_has_state(sw_sim, LV_STATE_CHECKED);
    polling_interval_seconds = std::atoi(lv_textarea_get_text(ta_poll));
    show_temperature = lv_obj_has_state(cb_temp, LV_STATE_CHECKED);
    show_conductivity = lv_obj_has_state(cb_cond, LV_STATE_CHECKED);
    show_pressure = lv_obj_has_state(cb_press, LV_STATE_CHECKED);

    std::ofstream file(settings_path);
    if (!file.is_open()) return;

    file << "SIM=" << (simulation_enabled ? "1" : "0") << '\n'
        << "INTERVAL=" << polling_interval_seconds << '\n'
        << "TEMP=" << (show_temperature ? "1" : "0") << '\n'
        << "COND=" << (show_conductivity ? "1" : "0") << '\n'
        << "PRESS=" << (show_pressure ? "1" : "0") << '\n';
}

// Show on-screen numeric keyboard for polling interval input
static void ta_event_cb(lv_event_t* /*e*/) {
    if (!keyboard) {
        keyboard = lv_keyboard_create(lv_scr_act());
        lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_NUMBER);
        lv_obj_set_size(keyboard, lv_disp_get_hor_res(nullptr), 180);
        lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    }
    lv_keyboard_set_textarea(keyboard, ta_poll);
    lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(keyboard);
}

// Hide keyboard if visible
static void close_keyboard_cb(lv_event_t* /*e*/) {
    if (keyboard && lv_obj_is_valid(keyboard))
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
}

// Save settings when save button is clicked
static void save_event_cb(lv_event_t* /*e*/) { save_settings(); }

// Create the full screen UI
void create_sensor_settings_screen() {
    if (screen) return;           // Prevent re-creation
    load_settings();              // Load saved settings

    screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(screen, lv_pct(100), lv_pct(100));
    lv_obj_add_style(screen, &style_screen_bg, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    create_header(screen);
    lv_obj_add_event_cb(screen, close_keyboard_cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* title = lv_label_create(screen);
    lv_label_set_text(title, "Sensor Settings");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);

    lv_obj_t* label_sim = lv_label_create(screen);
    lv_label_set_text(label_sim, "Simulation Mode:");
    lv_obj_add_style(label_sim, &style_label_white, 0);
    lv_obj_align(label_sim, LV_ALIGN_CENTER, -60, -70);

    sw_sim = lv_switch_create(screen);
    if (simulation_enabled) lv_obj_add_state(sw_sim, LV_STATE_CHECKED);
    lv_obj_align(sw_sim, LV_ALIGN_CENTER, 60, -72);

    lv_obj_t* label_poll = lv_label_create(screen);
    lv_label_set_text(label_poll, "Polling Interval (s):");
    lv_obj_add_style(label_poll, &style_label_white, 0);
    lv_obj_align(label_poll, LV_ALIGN_CENTER, -80, -20);

    ta_poll = lv_textarea_create(screen);
    lv_obj_set_size(ta_poll, 80, 35);
    lv_textarea_set_text(ta_poll, std::to_string(polling_interval_seconds).c_str());
    lv_obj_add_event_cb(ta_poll, ta_event_cb, LV_EVENT_FOCUSED, nullptr);
    lv_obj_align(ta_poll, LV_ALIGN_CENTER, 60, -22);

    lv_obj_t* label_data = lv_label_create(screen);
    lv_label_set_text(label_data, "Data to be Read:");
    lv_obj_add_style(label_data, &style_label_white, 0);
    lv_obj_align(label_data, LV_ALIGN_CENTER, 0, 30);

    cb_temp = lv_checkbox_create(screen);
    lv_checkbox_set_text(cb_temp, "");
    if (show_temperature) lv_obj_add_state(cb_temp, LV_STATE_CHECKED);
    lv_obj_align(cb_temp, LV_ALIGN_CENTER, -100, 60);

    cb_cond = lv_checkbox_create(screen);
    lv_checkbox_set_text(cb_cond, "");
    if (show_conductivity) lv_obj_add_state(cb_cond, LV_STATE_CHECKED);
    lv_obj_align(cb_cond, LV_ALIGN_CENTER, 0, 60);

    cb_press = lv_checkbox_create(screen);
    lv_checkbox_set_text(cb_press, "");
    if (show_pressure) lv_obj_add_state(cb_press, LV_STATE_CHECKED);
    lv_obj_align(cb_press, LV_ALIGN_CENTER, 100, 60);

    lv_obj_t* lbl1 = lv_label_create(screen);
    lv_label_set_text(lbl1, "Temperature");
    lv_obj_add_style(lbl1, &style_label_white, 0);
    lv_obj_align(lbl1, LV_ALIGN_CENTER, -100, 85);

    lv_obj_t* lbl2 = lv_label_create(screen);
    lv_label_set_text(lbl2, "Conductivity");
    lv_obj_add_style(lbl2, &style_label_white, 0);
    lv_obj_align(lbl2, LV_ALIGN_CENTER, 0, 85);

    lv_obj_t* lbl3 = lv_label_create(screen);
    lv_label_set_text(lbl3, "Pressure");
    lv_obj_add_style(lbl3, &style_label_white, 0);
    lv_obj_align(lbl3, LV_ALIGN_CENTER, 100, 85);

    lv_obj_t* back_btn = lv_btn_create(screen);
    lv_obj_set_size(back_btn, 100, 40);
    lv_obj_add_style(back_btn, &style_button, 0);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_add_event_cb(back_btn, [](lv_event_t*) {
        ScreenManager::get_instance().show_screen(0);
        }, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "Back");
    lv_obj_center(back_lbl);
    lv_obj_add_style(back_lbl, &style_label_white, 0);

    lv_obj_t* save_btn = lv_btn_create(screen);
    lv_obj_set_size(save_btn, 100, 40);

    static lv_style_t green_style;
    lv_style_init(&green_style);
    lv_style_set_bg_color(&green_style, lv_palette_main(LV_PALETTE_GREEN));
    lv_style_set_bg_opa(&green_style, LV_OPA_COVER);
    lv_style_set_radius(&green_style, 10);
    lv_obj_add_style(save_btn, &green_style, 0);

    lv_obj_align(save_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_add_event_cb(save_btn, save_event_cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* save_lbl = lv_label_create(save_btn);
    lv_label_set_text(save_lbl, "Save");
    lv_obj_center(save_lbl);
    lv_obj_add_style(save_lbl, &style_label_white, 0);

    btn_start = lv_btn_create(screen);
    lv_obj_set_size(btn_start, 170, 40);
    lv_obj_align(btn_start, LV_ALIGN_BOTTOM_MID, -100, -10);
    lv_obj_set_style_bg_color(btn_start, lv_color_hex(0xFFA500), 0);
    lv_obj_add_event_cb(btn_start, start_data_collection_cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* label_start = lv_label_create(btn_start);
    lv_label_set_text(label_start, "Start Data Collection");
    lv_obj_center(label_start);
    lv_obj_add_style(label_start, &style_label_white, 0);

    lv_obj_t* btn_stop = lv_btn_create(screen);
    lv_obj_set_size(btn_stop, 170, 40);
    lv_obj_align(btn_stop, LV_ALIGN_BOTTOM_MID, 100, -10);
    lv_obj_set_style_bg_color(btn_stop, lv_color_hex(0xDC143C), 0);
    lv_obj_add_event_cb(btn_stop, stop_data_collection_cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* label_stop = lv_label_create(btn_stop);
    lv_label_set_text(label_stop, "Stop Data Collection");
    lv_obj_center(label_stop);
    lv_obj_add_style(label_stop, &style_label_white, 0);

    ScreenManager::get_instance().register_screen(screen);
}
