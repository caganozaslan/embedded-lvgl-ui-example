#include "header.h"
#include "lvgl/lvgl.h"
#include "style.h"
#include <vector>
#include <ctime>      
#include <cstdio>
#include "systemfunctions.h"
static lv_obj_t* clock_label = nullptr;
static lv_obj_t* wifi_icon = nullptr;
static lv_obj_t* wifi_status_label = nullptr;

static lv_timer_t* wifi_timer = nullptr;
static lv_timer_t* clock_timer = nullptr;

static std::vector<lv_obj_t*> all_clock_labels;
static std::vector<lv_obj_t*> all_wifi_icons;

static void update_time(lv_timer_t* timer) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    char buf[6];
    strftime(buf, sizeof(buf), "%H:%M", t);

    for (auto lbl : all_clock_labels) {
        if (lbl && lv_obj_is_valid(lbl)) {
            lv_label_set_text(lbl, buf);
        }
    }
}

static void update_wifi_icon(lv_timer_t* timer) {
    bool connected = is_wifi_connected_cached();

    for (auto icon : all_wifi_icons) {
        if (icon && lv_obj_is_valid(icon)) {
            lv_label_set_text(icon, connected ? LV_SYMBOL_WIFI : LV_SYMBOL_CLOSE);
        }
    }
}

void create_header(lv_obj_t* parent) {
    lv_obj_t* header = lv_obj_create(parent);
    lv_obj_set_size(header, lv_pct(100), 30);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_style(header, &style_header_bar, 0);
    lv_obj_set_style_bg_color(header, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_outline_width(header, 0, 0);

    wifi_icon = lv_label_create(header);
    lv_obj_add_style(wifi_icon, &style_status_icon, 0);
    lv_obj_align(wifi_icon, LV_ALIGN_LEFT_MID, 10, 0);
    all_wifi_icons.push_back(wifi_icon);

    lv_obj_t* battery_icon = lv_label_create(header);
    lv_label_set_text(battery_icon, LV_SYMBOL_BATTERY_FULL);
    lv_obj_add_style(battery_icon, &style_status_icon, 0);
    lv_obj_align(battery_icon, LV_ALIGN_LEFT_MID, 50, 0);

    clock_label = lv_label_create(header);
    lv_obj_add_style(clock_label, &style_clock_label, 0);
    lv_obj_align(clock_label, LV_ALIGN_RIGHT_MID, -10, 0);
    all_clock_labels.push_back(clock_label);

    wifi_status_label = lv_label_create(header);
    lv_obj_align(wifi_status_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_style(wifi_status_label, &style_status_icon, 0);
    lv_obj_add_flag(wifi_status_label, LV_OBJ_FLAG_HIDDEN);

    update_time(nullptr);
    update_wifi_icon(nullptr);

    if (!clock_timer)
        clock_timer = lv_timer_create(update_time, 10000, NULL);

    if (!wifi_timer)
        wifi_timer = lv_timer_create(update_wifi_icon, 3000, NULL);
}

void header_show_wifi_status(const std::string& text, lv_color_t color) {
    if (wifi_status_label && lv_obj_is_valid(wifi_status_label)) {
        lv_label_set_text(wifi_status_label, text.c_str());
        lv_obj_clear_flag(wifi_status_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_text_color(wifi_status_label, color, 0);
    }
}

void header_clear_wifi_status() {
    if (wifi_status_label && lv_obj_is_valid(wifi_status_label)) {
        lv_obj_add_flag(wifi_status_label, LV_OBJ_FLAG_HIDDEN);
    }
}
