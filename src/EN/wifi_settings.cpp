#include "lvgl/lvgl.h"
#include "wifi_settings.h"
#include "style.h"
#include "screen_manager.h"
#include "systemfunctions.h"
#include "header.h"
#include <thread>
#include <vector>
#include <string>
#include <sstream>

static lv_obj_t* wifi_container = nullptr;
static lv_obj_t* pass_ta = nullptr;
static lv_obj_t* ssid_dropdown = nullptr;
static lv_obj_t* status_label = nullptr;
static lv_obj_t* spinner = nullptr;
static lv_obj_t* keyboard = nullptr;
static lv_obj_t* result_icon = nullptr;

void update_wifi_dropdown() {
    if (!ssid_dropdown || !lv_obj_is_valid(ssid_dropdown)) return;

    lv_dropdown_set_options(ssid_dropdown, "Scanning...");

    std::thread([] {
        std::vector<std::string> ssids = scan_wifi_networks();
        std::stringstream ss;
        for (const auto& s : ssids) ss << s << "\n";
        std::string options = ss.str();

        lv_async_call([](void* data) {
            std::string* opts = static_cast<std::string*>(data);
            if (ssid_dropdown && lv_obj_is_valid(ssid_dropdown)) {
                lv_dropdown_set_options(ssid_dropdown, opts->c_str());
            }
            delete opts;
            }, new std::string(options));
        }).detach();
}

static void ta_event_cb(lv_event_t* e) {
    lv_obj_t* ta = lv_event_get_target_obj(e);

    if (!keyboard) {
        lv_obj_t* parent = lv_obj_get_parent(ta);
        keyboard = lv_keyboard_create(parent);
        lv_obj_set_size(keyboard, lv_disp_get_hor_res(NULL), 180);
        lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
        lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    }
    lv_keyboard_set_textarea(keyboard, ta);
    lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(keyboard);
}

static void show_result_icon(bool success, const std::string& msg) {
    if (result_icon && lv_obj_is_valid(result_icon)) {
        lv_obj_del(result_icon);
        result_icon = nullptr;
    }

    result_icon = lv_obj_create(wifi_container);
    lv_obj_set_size(result_icon, 50, 50);
    lv_obj_align(result_icon, LV_ALIGN_CENTER, 0, 150);
    lv_obj_set_style_radius(result_icon, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(result_icon, success ? lv_palette_main(LV_PALETTE_GREEN)
        : lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_bg_opa(result_icon, LV_OPA_COVER, 0);

    lv_obj_t* symbol = lv_label_create(result_icon);
    lv_label_set_text(symbol, success ? LV_SYMBOL_OK : LV_SYMBOL_CLOSE);
    lv_obj_center(symbol);
    lv_obj_set_style_text_color(symbol, lv_color_white(), 0);
    lv_obj_set_style_text_font(symbol, &lv_font_montserrat_22, 0);

    std::string label_text = success ? "Connection successful!\n" : "Connection failed!\n";
    label_text += msg;

    if (status_label && lv_obj_is_valid(status_label))
        lv_label_set_text(status_label, label_text.c_str());

    lv_timer_create([](lv_timer_t* t) {
        if (result_icon && lv_obj_is_valid(result_icon)) {
            lv_obj_del(result_icon);
            result_icon = nullptr;
        }
        if (status_label && lv_obj_is_valid(status_label)) {
            lv_label_set_text(status_label, "Status: Waiting...");
        }
        lv_timer_del(t);
        }, 15000, NULL);
}

void create_wifi_screen() {
    if (wifi_container) return;

    init_styles();
    wifi_container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(wifi_container, lv_pct(100), lv_pct(100));
    lv_obj_clear_flag(wifi_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(wifi_container, &style_screen_bg, 0);

    create_header(wifi_container);

    lv_obj_add_event_cb(wifi_container, [](lv_event_t* e) {
        if (keyboard && lv_obj_is_valid(keyboard)) {
            lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
        }
        }, LV_EVENT_CLICKED, NULL);

    lv_obj_t* title = lv_label_create(wifi_container);
    lv_label_set_text(title, "Wi-Fi Settings");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);

    lv_obj_t* label_ssid = lv_label_create(wifi_container);
    lv_label_set_text(label_ssid, "Networks:");
    lv_obj_add_style(label_ssid, &style_label_white, 0);
    lv_obj_align(label_ssid, LV_ALIGN_CENTER, -135, -40);

    ssid_dropdown = lv_dropdown_create(wifi_container);
    lv_obj_set_size(ssid_dropdown, 200, 40);
    lv_obj_align(ssid_dropdown, LV_ALIGN_CENTER, 30, -40);
    lv_dropdown_set_options(ssid_dropdown, "Scanning...");
    update_wifi_dropdown();

    lv_obj_t* label_pass = lv_label_create(wifi_container);
    lv_label_set_text(label_pass, "Password:");
    lv_obj_add_style(label_pass, &style_label_white, 0);
    lv_obj_align(label_pass, LV_ALIGN_CENTER, -120, 20);

    pass_ta = lv_textarea_create(wifi_container);
    lv_obj_set_size(pass_ta, 200, 40);
    lv_textarea_set_password_mode(pass_ta, true);
    lv_textarea_set_placeholder_text(pass_ta, "Enter password");
    lv_obj_align(pass_ta, LV_ALIGN_CENTER, 30, 20);
    lv_obj_add_event_cb(pass_ta, ta_event_cb, LV_EVENT_FOCUSED, NULL);

    lv_obj_t* btn = lv_btn_create(wifi_container);
    lv_obj_set_size(btn, 120, 50);
    lv_obj_align(btn, LV_ALIGN_CENTER, -70, 90);
    lv_obj_add_style(btn, &style_button, 0);
    lv_obj_add_event_cb(btn, [](lv_event_t* e) {
        char buf[64];
        lv_dropdown_get_selected_str(ssid_dropdown, buf, sizeof(buf));
        std::string ssid(buf);
        std::string pass(lv_textarea_get_text(pass_ta));

        if (status_label && lv_obj_is_valid(status_label))
            lv_label_set_text(status_label, "Connecting...");

        if (!spinner) {
            spinner = lv_spinner_create(wifi_container);
            lv_obj_set_size(spinner, 40, 40);
            lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 150);
        }

        connect_to_wifi(ssid, pass, [](std::string result) {
            lv_async_call([](void* result_ptr) {
                std::string result = *(std::string*)result_ptr;
                delete (std::string*)result_ptr;

                if (spinner && lv_obj_is_valid(spinner)) {
                    lv_obj_del(spinner);
                    spinner = nullptr;
                }

                if (!(status_label && lv_obj_is_valid(status_label))) return;

                bool success = result.find("Baglandi:") != std::string::npos;
                show_result_icon(success, result);
                }, new std::string(result));
            });
        }, LV_EVENT_CLICKED, NULL);
    lv_obj_t* btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Connect");
    lv_obj_center(btn_label);
    lv_obj_add_style(btn_label, &style_label_white, 0);

    lv_obj_t* disconnect_btn = lv_btn_create(wifi_container);
    lv_obj_set_size(disconnect_btn, 120, 50);
    lv_obj_align(disconnect_btn, LV_ALIGN_CENTER, +70, 90);

    static lv_style_t red_btn_style;
    lv_style_init(&red_btn_style);
    lv_style_set_bg_color(&red_btn_style, lv_palette_main(LV_PALETTE_RED));
    lv_style_set_bg_opa(&red_btn_style, LV_OPA_COVER);
    lv_style_set_radius(&red_btn_style, 10);
    lv_obj_add_style(disconnect_btn, &red_btn_style, 0);

    lv_obj_add_event_cb(disconnect_btn, [](lv_event_t* e) {
        if (!is_wifi_connected_cached()) {
            if (status_label && lv_obj_is_valid(status_label))
                lv_label_set_text(status_label, "Already disconnected.");
            return;
        }

        disconnect_wifi();
        if (status_label && lv_obj_is_valid(status_label)) {
            std::string msg = "Wi-Fi disconnected.\nIP: None";
            lv_label_set_text(status_label, msg.c_str());
            lv_obj_set_style_text_color(status_label, lv_palette_main(LV_PALETTE_RED), 0);
        }
        }, LV_EVENT_CLICKED, NULL);
    lv_obj_t* disconnect_lbl = lv_label_create(disconnect_btn);
    lv_label_set_text(disconnect_lbl, "Disconnect");
    lv_obj_center(disconnect_lbl);
    lv_obj_add_style(disconnect_lbl, &style_label_white, 0);

    status_label = lv_label_create(wifi_container);
    lv_label_set_text(status_label, "Status: Waiting...");
    lv_obj_add_style(status_label, &style_label_white, 0);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_timer_create([](lv_timer_t* t) {
        if (!status_label || !lv_obj_is_valid(status_label)) return;

        if (is_wifi_connected_cached()) {
            lv_label_set_text(status_label, LV_SYMBOL_WIFI " Wi-Fi: Connected");
            lv_obj_set_style_text_color(status_label, lv_palette_main(LV_PALETTE_GREEN), 0);
        }
        else {
            lv_label_set_text(status_label, LV_SYMBOL_CLOSE " Wi-Fi: Not connected");
            lv_obj_set_style_text_color(status_label, lv_palette_main(LV_PALETTE_RED), 0);
        }
        }, 5000, NULL);

    lv_obj_t* back_btn = lv_btn_create(wifi_container);
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

    ScreenManager::get_instance().register_screen(wifi_container, update_wifi_dropdown);
}
