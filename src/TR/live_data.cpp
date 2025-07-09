#include "lvgl/lvgl.h"
#include "screen_manager.h"
#include "style.h"
#include "sensor_recorder.h"
#include "sensor_settings.h"
#include "header.h"
#include "live_data.h"

static lv_obj_t* screen = nullptr;
static lv_obj_t* temp_label = nullptr;
static lv_obj_t* cond_label = nullptr;
static lv_obj_t* pres_label = nullptr;

static lv_obj_t* status_label = nullptr;
static lv_obj_t* progressbar = nullptr;
static lv_obj_t* error_icon = nullptr;

static lv_obj_t* debug_temp_label = nullptr;
static lv_obj_t* debug_cond_label = nullptr;
static lv_obj_t* debug_pres_label = nullptr;

static lv_timer_t* update_timer = nullptr;

static lv_obj_t* temp_circle = nullptr;
static lv_obj_t* cond_circle = nullptr;
static lv_obj_t* pres_circle = nullptr;

static void update_live_data_cb(lv_timer_t* timer) {
    sensor_data_t latest = get_latest_sensor_data();

    char buf[64];

    if (temp_label && show_temperature) {
        snprintf(buf, sizeof(buf), "%f", latest.value1);
        lv_label_set_text(temp_label, buf);
    }

    if (cond_label && show_conductivity) {
        snprintf(buf, sizeof(buf), "%f", latest.value2);
        lv_label_set_text(cond_label, buf);
    }

    if (pres_label && show_pressure) {
        snprintf(buf, sizeof(buf), "%f", latest.value3);
        lv_label_set_text(pres_label, buf);
    }

    if (debug_temp_label) {
        snprintf(buf, sizeof(buf), "T: %f", latest.value1);
        lv_label_set_text(debug_temp_label, buf);
    }
    if (debug_cond_label) {
        snprintf(buf, sizeof(buf), "C: %f", latest.value2);
        lv_label_set_text(debug_cond_label, buf);
    }
    if (debug_pres_label) {
        snprintf(buf, sizeof(buf), "P: %f", latest.value3);
        lv_label_set_text(debug_pres_label, buf);
    }

    bool sensor_ok = (latest.value1 != 0 || latest.value2 != 0 || latest.value3 != 0);
    if (sensor_ok) {
        lv_label_set_text(status_label, "Sensor Okunuyor...");
        lv_obj_clear_flag(progressbar, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(error_icon, LV_OBJ_FLAG_HIDDEN);
    }
    else {
        lv_label_set_text(status_label, "Sensor Okunmuyor");
        lv_obj_add_flag(progressbar, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(error_icon, LV_OBJ_FLAG_HIDDEN);
    }


    static bool toggle = false;
    lv_color_t color = sensor_ok ? lv_palette_main(LV_PALETTE_GREEN) : lv_palette_main(LV_PALETTE_RED);

    for (lv_obj_t* circle : { temp_circle, cond_circle, pres_circle }) {
        if (circle) {
            lv_obj_set_style_border_color(circle, color, 0);
            lv_obj_set_style_border_opa(circle, toggle ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        }
    }

    toggle = !toggle;
}

void create_live_data_screen() {
    if (screen) return;
    init_styles();
    screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(screen, lv_pct(100), lv_pct(100));
    lv_obj_add_style(screen, &style_screen_bg, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    create_header(screen);

    lv_obj_t* title = lv_label_create(screen);
    lv_label_set_text(title, "Anlik Veriler");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);

    int circle_size = 140;
    lv_coord_t circle_radius = LV_RADIUS_CIRCLE;


    temp_circle = lv_obj_create(screen);
    lv_obj_set_size(temp_circle, circle_size, circle_size);
    lv_obj_set_style_radius(temp_circle, circle_radius, 0);
    lv_obj_set_style_bg_color(temp_circle, lv_color_hex(0x001933), 0);
    lv_obj_set_style_border_width(temp_circle, 2, 0);
    lv_obj_set_style_border_opa(temp_circle, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(temp_circle, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(temp_circle, LV_ALIGN_CENTER, -180, -20);

    lv_obj_t* temp_label_title = lv_label_create(screen);
    lv_label_set_text(temp_label_title, "Sicaklik");
    lv_obj_add_style(temp_label_title, &style_label_white, 0);
    lv_obj_align_to(temp_label_title, temp_circle, LV_ALIGN_OUT_TOP_MID, 0, -10);

    temp_label = lv_label_create(temp_circle);
    lv_label_set_text(temp_label, "-");
    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(temp_label, lv_color_white(), 0);
    lv_label_set_long_mode(temp_label, LV_LABEL_LONG_CLIP);
    lv_obj_center(temp_label);


    cond_circle = lv_obj_create(screen);
    lv_obj_set_size(cond_circle, circle_size, circle_size);
    lv_obj_set_style_radius(cond_circle, circle_radius, 0);
    lv_obj_set_style_bg_color(cond_circle, lv_color_hex(0x001933), 0);
    lv_obj_set_style_border_width(cond_circle, 2, 0);
    lv_obj_set_style_border_opa(cond_circle, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(cond_circle, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(cond_circle, LV_ALIGN_CENTER, 180, -20);

    lv_obj_t* cond_label_title = lv_label_create(screen);
    lv_label_set_text(cond_label_title, "Iletkenlik");
    lv_obj_add_style(cond_label_title, &style_label_white, 0);
    lv_obj_align_to(cond_label_title, cond_circle, LV_ALIGN_OUT_TOP_MID, 0, -10);

    cond_label = lv_label_create(cond_circle);
    lv_label_set_text(cond_label, "-");
    lv_obj_set_style_text_font(cond_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(cond_label, lv_color_white(), 0);
    lv_label_set_long_mode(cond_label, LV_LABEL_LONG_CLIP);
    lv_obj_center(cond_label);


    pres_circle = lv_obj_create(screen);
    lv_obj_set_size(pres_circle, circle_size, circle_size);
    lv_obj_set_style_radius(pres_circle, circle_radius, 0);
    lv_obj_set_style_bg_color(pres_circle, lv_color_hex(0x001933), 0);
    lv_obj_set_style_border_width(pres_circle, 2, 0);
    lv_obj_set_style_border_opa(pres_circle, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(pres_circle, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(pres_circle, LV_ALIGN_CENTER, 0, 120);

    lv_obj_t* pres_label_title = lv_label_create(screen);
    lv_label_set_text(pres_label_title, "Basinc");
    lv_obj_add_style(pres_label_title, &style_label_white, 0);
    lv_obj_align_to(pres_label_title, pres_circle, LV_ALIGN_OUT_TOP_MID, 0, -10);

    pres_label = lv_label_create(pres_circle);
    lv_label_set_text(pres_label, "-");
    lv_obj_set_style_text_font(pres_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(pres_label, lv_color_white(), 0);
    lv_label_set_long_mode(pres_label, LV_LABEL_LONG_CLIP);
    lv_obj_center(pres_label);


    status_label = lv_label_create(screen);
    lv_label_set_text(status_label, "Sensor Okunuyor...");
    lv_obj_add_style(status_label, &style_label_white, 0);
    lv_obj_align_to(status_label, pres_circle, LV_ALIGN_OUT_TOP_MID, 0, -105);

    progressbar = lv_bar_create(screen);
    lv_obj_set_size(progressbar, 120, 10);
    lv_obj_align_to(progressbar, status_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_bar_set_range(progressbar, 0, 100);
    lv_bar_set_value(progressbar, 0, LV_ANIM_OFF);

    lv_timer_create([](lv_timer_t* t) {
        static int val = 0;
        val = (val + 5) % 100;
        lv_bar_set_value(progressbar, val, LV_ANIM_ON);
        }, 100, NULL);

    error_icon = lv_label_create(screen);
    lv_label_set_text(error_icon, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(error_icon, lv_color_hex(0xFF4C4C), 0);
    lv_obj_set_style_text_font(error_icon, &lv_font_montserrat_28, 0);
    lv_obj_align_to(error_icon, status_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_add_flag(error_icon, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t* btn_back = lv_btn_create(screen);
    lv_obj_set_size(btn_back, 100, 40);
    lv_obj_add_style(btn_back, &style_button, 0);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_add_event_cb(btn_back, [](lv_event_t* e) {
        if (update_timer) {
            lv_timer_del(update_timer);
            update_timer = nullptr;
        }
        ScreenManager::get_instance().show_screen(0);
        }, LV_EVENT_CLICKED, NULL);

    lv_obj_t* label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, "Geri");
    lv_obj_center(label_back);
    lv_obj_add_style(label_back, &style_label_white, 0);

    debug_temp_label = lv_label_create(screen);
    lv_label_set_text(debug_temp_label, "T:");
    lv_obj_add_style(debug_temp_label, &style_label_white, 0);
    lv_obj_align(debug_temp_label, LV_ALIGN_BOTTOM_RIGHT, -10, -60);

    debug_cond_label = lv_label_create(screen);
    lv_label_set_text(debug_cond_label, "C:");
    lv_obj_add_style(debug_cond_label, &style_label_white, 0);
    lv_obj_align(debug_cond_label, LV_ALIGN_BOTTOM_RIGHT, -10, -40);

    debug_pres_label = lv_label_create(screen);
    lv_label_set_text(debug_pres_label, "P:");
    lv_obj_add_style(debug_pres_label, &style_label_white, 0);
    lv_obj_align(debug_pres_label, LV_ALIGN_BOTTOM_RIGHT, -10, -20);

    update_timer = lv_timer_create(update_live_data_cb, 1000, NULL);

    ScreenManager::get_instance().register_screen(screen, [] {
        if (update_timer) {
            lv_timer_del(update_timer);
            update_timer = nullptr;
        }
        update_timer = lv_timer_create(update_live_data_cb, 1000, NULL);
        });
}
