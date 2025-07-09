#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include "lvgl/lvgl.h"
#include <vector>


typedef void (*ScreenCallback)();

class ScreenManager {
public:
    
    static ScreenManager& get_instance();

   
    void register_screen(lv_obj_t* screen, ScreenCallback on_show = nullptr);
    void show_screen(int index);

    int get_current_index() const;

private:
    
    std::vector<lv_obj_t*> screens;
    std::vector<ScreenCallback> callbacks;

    int current_index = -1;

    ScreenManager() = default;
    ScreenManager(const ScreenManager&) = delete;
    ScreenManager& operator=(const ScreenManager&) = delete;
};

#endif 
