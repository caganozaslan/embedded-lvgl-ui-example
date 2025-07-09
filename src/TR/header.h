#ifndef HEADER_H
#define HEADER_H

#ifdef __cplusplus
#include "lvgl/lvgl.h"      
#include <string>
#include <vector>
#include <functional>

void create_header(lv_obj_t* parent);

void header_show_wifi_status(const std::string& text, lv_color_t color);
void header_clear_wifi_status();

#else


#endif 

#endif 
