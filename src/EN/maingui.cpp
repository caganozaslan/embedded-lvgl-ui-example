#include "lvgl/lvgl.h"
#include "style.h"
#include "maingui.h"
#include "screen_manager.h"
#include "header.h"
#include "systemfunctions.h"
#include "wifi_settings.h"  
#include <thread>
#include "systeminfo.h"
#include "sensor_settings.h"
#include "live_data.h"

static lv_obj_t* main_container = nullptr;

void create_main_screen() {
    if (!main_container) {
        init_styles();

        std::thread([] {
            std::this_thread::sleep_for(std::chrono::seconds(10));
            try_auto_connect_background();
            }).detach();

        main_container = lv_obj_create(lv_scr_act());
        lv_obj_set_size(main_container, lv_pct(100), lv_pct(100));
        lv_obj_clear_flag(main_container, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_style(main_container, &style_screen_bg, 0);

        create_header(main_container);

        lv_obj_t* title = lv_label_create(main_container);
        lv_label_set_text(title, "Sensor Reader");
        lv_obj_add_style(title, &style_title, 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);

        const char* left_labels[] = { "Live Data", "Average Data", "System Info" };
        for (int i = 0; i < 3; ++i) {
            lv_obj_t* btn = lv_btn_create(main_container);
            lv_obj_set_size(btn, 140, 50);
            lv_obj_add_style(btn, &style_button, 0);
            lv_obj_align(btn, LV_ALIGN_CENTER, -150, -60 + i * 70);

            if (i == 0) {
                lv_obj_add_event_cb(btn, [](lv_event_t* e) {
                    ScreenManager::get_instance().show_screen(4);
                    }, LV_EVENT_CLICKED, NULL);
            }
            if (i == 1) {
                lv_obj_add_event_cb(btn, [](lv_event_t* e) {
                    ScreenManager::get_instance().show_screen(5);
                    }, LV_EVENT_CLICKED, NULL);
            }

            if (i == 2) {
                lv_obj_add_event_cb(btn, [](lv_event_t* e) {
                    ScreenManager::get_instance().show_screen(2);
                    }, LV_EVENT_CLICKED, NULL);
            }

            lv_obj_t* lbl = lv_label_create(btn);
            lv_label_set_text(lbl, left_labels[i]);
            lv_obj_center(lbl);
            lv_obj_add_style(lbl, &style_label_white, 0);
        }

        const char* right_labels[] = { "Wi-Fi Settings", "Sensor Settings", "Device Settings" };
        for (int i = 0; i < 3; ++i) {
            lv_obj_t* btn = lv_btn_create(main_container);
            lv_obj_set_size(btn, 140, 50);
            lv_obj_add_style(btn, &style_button, 0);
            lv_obj_align(btn, LV_ALIGN_CENTER, 150, -60 + i * 70);

            if (i == 0) {
                lv_obj_add_event_cb(btn, [](lv_event_t* e) {
                    ScreenManager::get_instance().show_screen(1);
                    }, LV_EVENT_CLICKED, NULL);
            }
            if (i == 1) {
                lv_obj_add_event_cb(btn, [](lv_event_t* e) {
                    ScreenManager::get_instance().show_screen(3);
                    }, LV_EVENT_CLICKED, NULL);
            }
            if (i == 2) {
                lv_obj_add_event_cb(btn, [](lv_event_t* e) {
                    ScreenManager::get_instance().show_screen(6);
                    }, LV_EVENT_CLICKED, NULL);
            }

            lv_obj_t* lbl = lv_label_create(btn);
            lv_label_set_text(lbl, right_labels[i]);
            lv_obj_center(lbl);
            lv_obj_add_style(lbl, &style_label_white, 0);
        }

        static lv_style_t style_shutdown_button;
        lv_style_init(&style_shutdown_button);
        lv_style_set_radius(&style_shutdown_button, 10);
        lv_style_set_bg_color(&style_shutdown_button, lv_palette_main(LV_PALETTE_RED));
        lv_style_set_bg_opa(&style_shutdown_button, LV_OPA_COVER);

        lv_obj_t* shutdown_btn = lv_btn_create(main_container);
        lv_obj_set_size(shutdown_btn, 160, 50);
        lv_obj_add_style(shutdown_btn, &style_shutdown_button, 0);
        lv_obj_align(shutdown_btn, LV_ALIGN_BOTTOM_MID, 0, -20);

        lv_obj_add_event_cb(shutdown_btn, [](lv_event_t* e) {
            shutdown_device();
            }, LV_EVENT_CLICKED, NULL);

        lv_obj_t* lbl = lv_label_create(shutdown_btn);
        lv_label_set_text(lbl, "Shutdown Device");
        lv_obj_center(lbl);
        lv_obj_add_style(lbl, &style_label_white, 0);
    }

    ScreenManager::get_instance().register_screen(main_container);
}
