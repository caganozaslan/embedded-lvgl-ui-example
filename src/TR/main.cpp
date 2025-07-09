#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "wifi_settings.h"
#include "systemfunctions.h"
#include "lvgl/src/drivers/evdev/lv_evdev.h"
#include <stdio.h>
#include "screen_manager.h"
#include "systeminfo.h"
#include "sensor_settings.h"
#include "live_data.h"
#include "average_data.h"
#include "settings_screen.h"
extern "C" {
    #include "maingui.h"
}

int main(void)
{
    lv_init();

    /*Linux frame buffer device init*/
    lv_display_t * disp = lv_linux_fbdev_create();
    lv_linux_fbdev_set_file(disp, "/dev/fb0");

    // Doğru input ayarı
    lv_indev_t * indev = lv_evdev_create(LV_INDEV_TYPE_POINTER, "/dev/input/touchscreen0");

    if(indev == NULL) {
        printf("Dokunmatik input oluşturulamadı!\n");
    }

    /*Çağırılacak Fonksiyon Yeri*/
	start_wifi_monitoring();
    create_main_screen();
    create_wifi_screen();
    create_system_info_screen();
    create_sensor_settings_screen();
    create_live_data_screen();
    create_average_data_screen();
    create_settings_screen();
    ScreenManager::get_instance().show_screen(0);

    /*Handle LVGL tasks*/
    while(1) {
        lv_timer_handler();
        usleep(5000);
    }

    return 0;
}
