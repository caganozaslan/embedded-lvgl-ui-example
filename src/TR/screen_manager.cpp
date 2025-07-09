#include "screen_manager.h"

ScreenManager& ScreenManager::get_instance() {
    static ScreenManager instance;
    return instance;
}

void ScreenManager::register_screen(lv_obj_t* screen, ScreenCallback on_show) {
    screens.push_back(screen);         
    callbacks.push_back(on_show);      
    lv_obj_add_flag(screen, LV_OBJ_FLAG_HIDDEN); 
}

void ScreenManager::show_screen(int index) {
    if (index < 0 || index >= static_cast<int>(screens.size()))
        return;

    if (current_index >= 0 && current_index < static_cast<int>(screens.size()))
        lv_obj_add_flag(screens[current_index], LV_OBJ_FLAG_HIDDEN);

    lv_obj_clear_flag(screens[index], LV_OBJ_FLAG_HIDDEN);
    current_index = index;

    if (callbacks[index]) {
        callbacks[index]();  
    }
}
   

int ScreenManager::get_current_index() const {
    return current_index;
}
