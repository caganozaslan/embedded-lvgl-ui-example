#include "lvgl/lvgl.h"
#include "style.h"

lv_style_t style_screen_bg;
lv_style_t style_label_white;
lv_style_t style_title;
lv_style_t style_button;
lv_style_t style_header_bar;
lv_style_t style_status_icon;
lv_style_t style_clock_label;

void init_styles() {
    // background color
    lv_style_init(&style_screen_bg);
    lv_style_set_bg_color(&style_screen_bg, lv_color_hex(0x1e1e1e));
    lv_style_set_bg_opa(&style_screen_bg, LV_OPA_COVER);

    // white text color (added for adjustable brightness if needed)
    lv_style_init(&style_label_white);
    lv_style_set_text_color(&style_label_white, lv_color_white());

    // Title font style
    lv_style_init(&style_title);
    lv_style_set_text_color(&style_title, lv_color_white());
    lv_style_set_text_font(&style_title, &lv_font_montserrat_22);

    // Standard style for all buttons
    lv_style_init(&style_button);
    lv_style_set_radius(&style_button, 10);
    lv_style_set_bg_color(&style_button, lv_color_hex(0x0055aa));
    lv_style_set_bg_grad_color(&style_button, lv_color_hex(0x003377));
    lv_style_set_bg_grad_dir(&style_button, LV_GRAD_DIR_VER);
    lv_style_set_text_color(&style_button, lv_color_white());

    // Header-related styles below

    lv_style_init(&style_header_bar);
    lv_style_set_bg_color(&style_header_bar, lv_color_hex(0x222222));
    lv_style_set_bg_opa(&style_header_bar, LV_OPA_COVER);
    lv_style_set_pad_all(&style_header_bar, 6);

    lv_style_init(&style_status_icon);
    lv_style_set_text_color(&style_status_icon, lv_color_white());
    lv_style_set_text_font(&style_status_icon, &lv_font_montserrat_20);

    lv_style_init(&style_clock_label);
    lv_style_set_text_color(&style_clock_label, lv_color_white());
    lv_style_set_text_font(&style_clock_label, &lv_font_montserrat_20);
}
