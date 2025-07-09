#ifndef STYLE_H
#define STYLE_H

#ifdef __cplusplus
extern "C" {
#endif

    void init_styles();

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include "lvgl/lvgl.h"


extern lv_style_t style_screen_bg;
extern lv_style_t style_label_white;
extern lv_style_t style_title;
extern lv_style_t style_button;
extern lv_style_t style_label_white;
extern lv_style_t style_header_bar;
extern lv_style_t style_status_icon;
extern lv_style_t style_clock_label;

#endif

#endif 
