#pragma once

#ifdef __cplusplus
extern "C" {
#endif

    void create_system_info_screen();

    
    void update_sysinfo_timer(lv_timer_t* timer);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
// Parametresiz kullanabilmek için SİLME!
void update_sysinfo();
#endif
