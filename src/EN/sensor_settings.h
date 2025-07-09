#pragma once

#include "lvgl/lvgl.h"

//–– Uygulama içinde bir-kere tanımlanacak değişkenlerin bildirimleri ––//
extern bool simulation_enabled;
extern int  polling_interval_seconds;
extern bool show_temperature;
extern bool show_conductivity;
extern bool show_pressure;

// Ekranı oluşturan API
void create_sensor_settings_screen();
