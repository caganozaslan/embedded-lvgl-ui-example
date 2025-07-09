#include "screen_manager.h"

// Singleton access to the ScreenManager instance
ScreenManager& ScreenManager::get_instance() {
    static ScreenManager instance;
    return instance;
}

// Register a screen with an optional callback to run when it's shown
void ScreenManager::register_screen(lv_obj_t* screen, ScreenCallback on_show) {
    screens.push_back(screen);              // Store the screen
    callbacks.push_back(on_show);           // Store the associated callback
    lv_obj_add_flag(screen, LV_OBJ_FLAG_HIDDEN);  // Hide the screen initially
}

// Show the screen at the given index
void ScreenManager::show_screen(int index) {
    if (index < 0 || index >= static_cast<int>(screens.size()))
        return;

    // Hide current screen if one is active
    if (current_index >= 0 && current_index < static_cast<int>(screens.size()))
        lv_obj_add_flag(screens[current_index], LV_OBJ_FLAG_HIDDEN);

    // Show the new screen
    lv_obj_clear_flag(screens[index], LV_OBJ_FLAG_HIDDEN);
    current_index = index;

    // Call the associated callback if available
    if (callbacks[index]) {
        callbacks[index]();
    }
}

// Get the index of the currently active screen
int ScreenManager::get_current_index() const {
    return current_index;
}
