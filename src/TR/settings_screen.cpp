#include "lvgl/lvgl.h"
#include "style.h"
#include "settings_screen.h"
#include "screen_manager.h"
#include "header.h"
#include "systemfunctions.h"

#ifndef _WIN32
#   include <fstream>
#   include <sstream>
#   include <unordered_map>

static const char* SETTINGS_PATH = "/etc/settings.txt";
static std::unordered_map<std::string, std::string> settings_map;

static void load_settings();
static void save_settings();
#endif





/*-------------------------------------------------
 * POPUP GÖSTERİCİLERİ (Global, Tekil Referanslar)
 *------------------------------------------------*/
static lv_obj_t* confirm_popup = nullptr;
static lv_obj_t* info_popup = nullptr;
static lv_obj_t* logs_popup = nullptr;

/*-------------------------------------------------
 *  EKRAN & BİLEŞEN GÖSTERİCİLERİ
 *------------------------------------------------*/
static lv_obj_t* settings_container = nullptr;
static lv_obj_t* tabview = nullptr;

static lv_obj_t* wifi_switch = nullptr;
static lv_obj_t* ssh_switch = nullptr;
static lv_obj_t* server_switch = nullptr;
static lv_obj_t* timeout_dropdown = nullptr;
static lv_obj_t* theme_switch = nullptr;
static lv_obj_t* brightness_slider = nullptr;
static lv_obj_t* memory_switch = nullptr;
static lv_obj_t* power_switch = nullptr;

/*–------------------------------------------------
 *  ÖN İLANLAR
 *------------------------------------------------*/
static void create_network_tab(lv_obj_t* tab);
static void create_system_tab(lv_obj_t* tab);
static void create_interface_tab(lv_obj_t* tab);

/* Popup yardımcıları */
static void close_parent_popup_cb(lv_event_t* e);
static void info_popup_ok_cb(lv_event_t* e);
static void logs_popup_close_cb(lv_event_t* e);

/* Onay callback’leri */
static void confirm_forget_wifi_cb(lv_event_t* e);
static void confirm_factory_reset_cb(lv_event_t* e);
static void confirm_clear_logs_cb(lv_event_t* e);

/* Event callback’ler */
static void wifi_auto_connect_cb(lv_event_t* e);
static void forget_wifi_cb(lv_event_t* e);
static void ssh_access_cb(lv_event_t* e);
static void server_data_share_cb(lv_event_t* e);
static void date_time_cb(lv_event_t* e);
static void sync_time_cb(lv_event_t* e);
static void check_updates_cb(lv_event_t* e);
static void factory_reset_cb(lv_event_t* e);
static void view_logs_cb(lv_event_t* e);
static void clear_logs_cb(lv_event_t* e);
static void screen_timeout_cb(lv_event_t* e);
static void theme_switch_cb(lv_event_t* e);
static void brightness_cb(lv_event_t* e);
static void memory_saver_cb(lv_event_t* e);
static void power_saver_cb(lv_event_t* e);

/* Popup oluşturucular */
static void show_confirm_popup(const char* message, lv_event_cb_t confirm_cb);
static void show_info_popup(const char* message);
static void show_logs_popup();


#ifndef _WIN32
#   include <regex>
#   include <cstdlib>

static void apply_datetime(const char* date_str, const char* time_str)
{
    // GG.AA.YYYY kontrolü
    std::regex date_regex(R"(^\d{2}\.\d{2}\.\d{4}$)");
    std::regex time_regex(R"(^\d{2}\.\d{2}$)");

    if (!std::regex_match(date_str, date_regex) || !std::regex_match(time_str, time_regex)) {
        show_info_popup("Tarih/Saat formati hatali!");
        return;
    }

    // GG.AA.YYYY → YYYY-AA-GG
    std::string d(date_str);
    std::string t(time_str);
    std::string yyyy = d.substr(6, 4);
    std::string mm = d.substr(3, 2);
    std::string dd = d.substr(0, 2);
    std::string hh = t.substr(0, 2);
    std::string mi = t.substr(3, 2);

    std::string cmd = "date -s \"" + yyyy + "-" + mm + "-" + dd + " " + hh + ":" + mi + "\"";
    int result = std::system(cmd.c_str());

    if (result != 0) {
        show_info_popup("Sistem saati ayarlanamadi!");
    }
    else {
        show_info_popup("Sistem saati basariyla ayarlandi.");
    }
}
#else
static void apply_datetime(const char* date_str, const char* time_str)
{
    std::string msg = std::string("Simulasyon: ") + date_str + " " + time_str;
    show_info_popup(msg.c_str());
}
#endif



static void show_datetime_popup()
{
    static lv_obj_t* datetime_popup = nullptr;
    if (datetime_popup && lv_obj_is_valid(datetime_popup)) {
        lv_obj_del(datetime_popup);
        datetime_popup = nullptr;
    }

    // Popup arkaplanı
    datetime_popup = lv_obj_create(lv_scr_act());
    lv_obj_set_size(datetime_popup, 400, 330);
    lv_obj_center(datetime_popup);
    lv_obj_add_style(datetime_popup, &style_screen_bg, 0);

    // Başlık
    lv_obj_t* label = lv_label_create(datetime_popup);
    lv_label_set_text(label, "Tarih ve Saat Giriniz");
    lv_obj_add_style(label, &style_label_white, 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    // Tarih alanı (GG.AA.YYYY)
    lv_obj_t* date_ta = lv_textarea_create(datetime_popup);
    lv_textarea_set_placeholder_text(date_ta, "GG.AA.YYYY");
    lv_obj_set_size(date_ta, 160, 40);
    lv_obj_align(date_ta, LV_ALIGN_TOP_LEFT, 20, 60);
    lv_textarea_set_one_line(date_ta, true);
    lv_textarea_set_max_length(date_ta, 10);
    lv_obj_add_flag(date_ta, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_add_style(date_ta, &style_button, 0);
    lv_obj_set_style_text_align(date_ta, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    // Saat alanı (SS.DD)
    lv_obj_t* time_ta = lv_textarea_create(datetime_popup);
    lv_textarea_set_placeholder_text(time_ta, "SS.DD");
    lv_obj_set_size(time_ta, 100, 40);
    lv_obj_align(time_ta, LV_ALIGN_TOP_RIGHT, -20, 60);
    lv_textarea_set_one_line(time_ta, true);
    lv_textarea_set_max_length(time_ta, 5);
    lv_obj_add_flag(time_ta, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_add_style(time_ta, &style_button, 0);
    lv_obj_set_style_text_align(time_ta, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    // Buton satırı (ortalanmış)
    lv_obj_t* btn_row = lv_obj_create(datetime_popup);
    lv_obj_set_size(btn_row, lv_pct(100), 50);
    lv_obj_align(btn_row, LV_ALIGN_CENTER, 0, 10);
    lv_obj_clear_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(btn_row, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(btn_row, LV_OPA_0, 0);
    lv_obj_set_layout(btn_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Tamam butonu
    lv_obj_t* ok_btn = lv_btn_create(btn_row);
    lv_obj_set_size(ok_btn, 120, 40);
    lv_obj_add_style(ok_btn, &style_button, 0);
    lv_obj_add_event_cb(ok_btn, [](lv_event_t* e) {
        lv_obj_t* btn = static_cast<lv_obj_t*>(lv_event_get_target(e));
        lv_obj_t* popup = lv_obj_get_parent(lv_obj_get_parent(btn));

        // Textarea'ları yakala
        lv_obj_t* date_ta = lv_obj_get_child(popup, 1);
        lv_obj_t* time_ta = lv_obj_get_child(popup, 2);

        const char* date_val = lv_textarea_get_text(date_ta);
        const char* time_val = lv_textarea_get_text(time_ta);

        apply_datetime(date_val, time_val);  // 🔧 Ayar fonksiyonu çağrısı

        if (popup && lv_obj_is_valid(popup)) lv_obj_del(popup);
        }, LV_EVENT_CLICKED, NULL);

    lv_obj_t* ok_label = lv_label_create(ok_btn);
    lv_label_set_text(ok_label, "Tamam");
    lv_obj_center(ok_label);
    lv_obj_add_style(ok_label, &style_label_white, 0);

    // Kapat butonu
    lv_obj_t* close_btn = lv_btn_create(btn_row);
    lv_obj_set_size(close_btn, 120, 40);
    lv_obj_add_style(close_btn, &style_button, 0);
    lv_obj_add_event_cb(close_btn, [](lv_event_t* e) {
        lv_obj_t* btn = static_cast<lv_obj_t*>(lv_event_get_target(e));
        lv_obj_t* popup = lv_obj_get_parent(lv_obj_get_parent(btn));
        if (popup && lv_obj_is_valid(popup)) lv_obj_del(popup);
        }, LV_EVENT_CLICKED, NULL);

    lv_obj_t* close_label = lv_label_create(close_btn);
    lv_label_set_text(close_label, "Kapat");
    lv_obj_center(close_label);
    lv_obj_add_style(close_label, &style_label_white, 0);

    // Klavye
    lv_obj_t* kb = lv_keyboard_create(datetime_popup);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_NUMBER);
    lv_obj_set_size(kb, 380, 100);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 5);
    lv_keyboard_set_textarea(kb, date_ta);

    // Focus değiştirici
    auto focus_cb = [](lv_event_t* e) {
        lv_obj_t* ta = static_cast<lv_obj_t*>(lv_event_get_target(e));
        lv_obj_t* kb = static_cast<lv_obj_t*>(lv_event_get_user_data(e));
        lv_keyboard_set_textarea(kb, ta);
        };
    lv_obj_add_event_cb(date_ta, focus_cb, LV_EVENT_FOCUSED, kb);
    lv_obj_add_event_cb(time_ta, focus_cb, LV_EVENT_FOCUSED, kb);
}



// BACKEND FUNCTIONS




#ifndef _WIN32
static void clear_all_logs()
{
    std::ofstream out("/etc/logs.txt", std::ios::trunc);
    if (!out.is_open()) {
        show_info_popup("Log dosyasi silinemedi!");
        return;
    }
}

static void delete_all_wifi_records()
{
    std::system("rm -f /etc/known_networks.txt");
}
#else   // ____ SADECE Windows derlemesi ____
static void clear_all_logs() {}
static void delete_all_wifi_records() {}
#endif  // _WIN32
// BACKEND END









/*-------------------------------------------------
 *  ANA EKRAN
 *------------------------------------------------*/
void create_settings_screen()
{
    if (!settings_container) {
        init_styles();

        settings_container = lv_obj_create(lv_scr_act());
        lv_obj_set_size(settings_container, lv_pct(100), lv_pct(100));
        lv_obj_clear_flag(settings_container, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_style(settings_container, &style_screen_bg, 0);

        create_header(settings_container);

        /* Başlık */
        lv_obj_t* title = lv_label_create(settings_container);
        lv_label_set_text(title, "Cihaz Ayarlari");
        lv_obj_add_style(title, &style_title, 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);

        /* TabView */
        tabview = lv_tabview_create(settings_container);
        lv_obj_set_size(tabview, lv_pct(90), lv_pct(70));
        lv_tabview_set_tab_bar_size(tabview, 35);
        lv_obj_align(tabview, LV_ALIGN_CENTER, 0, 15);

        /* Sekmeler */
        lv_obj_t* tab1 = lv_tabview_add_tab(tabview, "Ag/Sunucu");
        lv_obj_t* tab2 = lv_tabview_add_tab(tabview, "Sistem");
        lv_obj_t* tab3 = lv_tabview_add_tab(tabview, "Arayuz");

        create_network_tab(tab1);
        create_system_tab(tab2);
        create_interface_tab(tab3);
    }

    /* Geri butonu */
    lv_obj_t* back_btn = lv_btn_create(settings_container);
    lv_obj_set_size(back_btn, 100, 40);
    lv_obj_add_style(back_btn, &style_button, 0);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        ScreenManager::get_instance().show_screen(0);
        }, LV_EVENT_CLICKED, NULL);

    lv_obj_t* back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "Geri");
    lv_obj_center(back_lbl);
    lv_obj_add_style(back_lbl, &style_label_white, 0);

    /* Kaydet butonu */
    lv_obj_t* save_btn = lv_btn_create(settings_container);
    lv_obj_set_size(save_btn, 100, 40);
    lv_obj_align(save_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_add_event_cb(save_btn, [](lv_event_t* e) {
#ifndef _WIN32
        save_settings();
#endif
        show_info_popup("Ayarlar kaydedildi.");
        }, LV_EVENT_CLICKED, NULL);

    lv_obj_t* save_label = lv_label_create(save_btn);
    lv_label_set_text(save_label, "Kaydet");
    lv_obj_center(save_label);

#ifndef _WIN32
    load_settings();  /* Önceki ayarları dosyadan yükle */
#endif

    ScreenManager::get_instance().register_screen(settings_container);
}

/*-------------------------------------------------
 *  TAB: Ağ & Sunucu
 *------------------------------------------------*/
static void create_network_tab(lv_obj_t* tab)
{
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(tab, 10, 0);

    /* Wi-Fi Otomatik Bağlantı */
    lv_obj_t* wifi_container = lv_obj_create(tab);
    lv_obj_set_size(wifi_container, lv_pct(100), 60);
    lv_obj_clear_flag(wifi_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(wifi_container, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(wifi_container, LV_OPA_0, 0);
    lv_obj_set_style_pad_all(wifi_container, 5, 0);

    lv_obj_t* wifi_label = lv_label_create(wifi_container);
    lv_label_set_text(wifi_label, "Wi-Fi Otomatik Baglanti");
    lv_obj_align(wifi_label, LV_ALIGN_LEFT_MID, 0, 0);

    wifi_switch = lv_switch_create(wifi_container);
    lv_obj_align(wifi_switch, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(wifi_switch, wifi_auto_connect_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Wi-Fi kayıtlarını unut */
    lv_obj_t* forget_btn = lv_btn_create(tab);
    lv_obj_set_size(forget_btn, lv_pct(100), 50);
    lv_obj_add_style(forget_btn, &style_button, 0);
    lv_obj_add_event_cb(forget_btn, forget_wifi_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* forget_label = lv_label_create(forget_btn);
    lv_label_set_text(forget_label, "Tum Wi-Fi Kayitlarini Unut");
    lv_obj_center(forget_label);
    lv_obj_add_style(forget_label, &style_label_white, 0);

    /* SSH erişimi */
    lv_obj_t* ssh_container = lv_obj_create(tab);
    lv_obj_set_size(ssh_container, lv_pct(100), 60);
    lv_obj_clear_flag(ssh_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(ssh_container, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(ssh_container, LV_OPA_0, 0);
    lv_obj_set_style_pad_all(ssh_container, 5, 0);

    lv_obj_t* ssh_label = lv_label_create(ssh_container);
    lv_label_set_text(ssh_label, "SSH Erisimi");
    lv_obj_align(ssh_label, LV_ALIGN_LEFT_MID, 0, 0);

    ssh_switch = lv_switch_create(ssh_container);
    lv_obj_align(ssh_switch, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(ssh_switch, ssh_access_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Sunucuyla veri paylaşımı */
    lv_obj_t* server_container = lv_obj_create(tab);
    lv_obj_set_size(server_container, lv_pct(100), 60);
    lv_obj_clear_flag(server_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(server_container, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(server_container, LV_OPA_0, 0);
    lv_obj_set_style_pad_all(server_container, 5, 0);

    lv_obj_t* server_label = lv_label_create(server_container);
    lv_label_set_text(server_label, "Sunucuyla Veri Paylasimi");
    lv_obj_align(server_label, LV_ALIGN_LEFT_MID, 0, 0);

    server_switch = lv_switch_create(server_container);
    lv_obj_align(server_switch, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(server_switch, server_data_share_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

/*-------------------------------------------------
 *  TAB: Sistem
 *------------------------------------------------*/
static void create_system_tab(lv_obj_t* tab)
{
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(tab, 10, 0);

    /* Tarih/Saat */
    lv_obj_t* datetime_container = lv_obj_create(tab);
    lv_obj_set_size(datetime_container, lv_pct(100), 60);
    lv_obj_clear_flag(datetime_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(datetime_container, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(datetime_container, LV_OPA_0, 0);
    lv_obj_set_style_pad_all(datetime_container, 5, 0);

    lv_obj_t* datetime_btn = lv_btn_create(datetime_container);
    lv_obj_set_size(datetime_btn, 120, 40);
    lv_obj_add_style(datetime_btn, &style_button, 0);
    lv_obj_align(datetime_btn, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_event_cb(datetime_btn, date_time_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* datetime_label = lv_label_create(datetime_btn);
    lv_label_set_text(datetime_label, "Tarih/Saat");
    lv_obj_center(datetime_label);
    lv_obj_add_style(datetime_label, &style_label_white, 0);

    lv_obj_t* sync_btn = lv_btn_create(datetime_container);
    lv_obj_set_size(sync_btn, 120, 40);
    lv_obj_add_style(sync_btn, &style_button, 0);
    lv_obj_align(sync_btn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(sync_btn, sync_time_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* sync_label = lv_label_create(sync_btn);
    lv_label_set_text(sync_label, "Senkronize Et");
    lv_obj_center(sync_label);
    lv_obj_add_style(sync_label, &style_label_white, 0);

    /* Güncellemeleri kontrol et */
    lv_obj_t* update_btn = lv_btn_create(tab);
    lv_obj_set_size(update_btn, lv_pct(100), 50);
    lv_obj_add_style(update_btn, &style_button, 0);
    lv_obj_add_event_cb(update_btn, check_updates_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* update_label = lv_label_create(update_btn);
    lv_label_set_text(update_label, "Guncellemeleri Kontrol Et");
    lv_obj_center(update_label);
    lv_obj_add_style(update_label, &style_label_white, 0);

    /* Fabrika ayarlarına döndür */
    lv_obj_t* factory_btn = lv_btn_create(tab);
    lv_obj_set_size(factory_btn, lv_pct(100), 50);
    lv_obj_add_style(factory_btn, &style_button, 0);
    lv_obj_add_event_cb(factory_btn, factory_reset_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* factory_label = lv_label_create(factory_btn);
    lv_label_set_text(factory_label, "Fabrika Ayarlarina Dondur");
    lv_obj_center(factory_label);
    lv_obj_add_style(factory_label, &style_label_white, 0);

    /* Logları görüntüle */
    lv_obj_t* logs_btn = lv_btn_create(tab);
    lv_obj_set_size(logs_btn, lv_pct(100), 50);
    lv_obj_add_style(logs_btn, &style_button, 0);
    lv_obj_add_event_cb(logs_btn, view_logs_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* logs_label = lv_label_create(logs_btn);
    lv_label_set_text(logs_label, "Loglari Goruntule");
    lv_obj_center(logs_label);
    lv_obj_add_style(logs_label, &style_label_white, 0);

    /* Logları temizle */
    lv_obj_t* clear_logs_btn = lv_btn_create(tab);
    lv_obj_set_size(clear_logs_btn, lv_pct(100), 50);
    lv_obj_add_style(clear_logs_btn, &style_button, 0);
    lv_obj_add_event_cb(clear_logs_btn, clear_logs_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* clear_logs_label = lv_label_create(clear_logs_btn);
    lv_label_set_text(clear_logs_label, "Loglari Temizle");
    lv_obj_center(clear_logs_label);
    lv_obj_add_style(clear_logs_label, &style_label_white, 0);

    /* Ekran zaman aşımı */
    lv_obj_t* timeout_container = lv_obj_create(tab);
    lv_obj_set_size(timeout_container, lv_pct(100), 60);
    lv_obj_clear_flag(timeout_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(timeout_container, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(timeout_container, LV_OPA_0, 0);
    lv_obj_set_style_pad_all(timeout_container, 5, 0);

    lv_obj_t* timeout_label = lv_label_create(timeout_container);
    lv_label_set_text(timeout_label, "Ekran Zaman Asimi");
    lv_obj_align(timeout_label, LV_ALIGN_LEFT_MID, 0, 0);

    timeout_dropdown = lv_dropdown_create(timeout_container);
    lv_dropdown_set_options(timeout_dropdown, "30 saniye\n1 dakika\n5 dakika\n10 dakika\nHic");
    lv_obj_align(timeout_dropdown, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(timeout_dropdown, screen_timeout_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

/*-------------------------------------------------
 *  TAB: Arayüz
 *------------------------------------------------*/
static void create_interface_tab(lv_obj_t* tab)
{
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(tab, 10, 0);

    /* Gece/Gündüz Modu */
    lv_obj_t* theme_container = lv_obj_create(tab);
    lv_obj_set_size(theme_container, lv_pct(100), 60);
    lv_obj_clear_flag(theme_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(theme_container, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(theme_container, LV_OPA_0, 0);
    lv_obj_set_style_pad_all(theme_container, 5, 0);

    lv_obj_t* theme_label = lv_label_create(theme_container);
    lv_label_set_text(theme_label, "Gece/Gunduz Modu");
    lv_obj_align(theme_label, LV_ALIGN_LEFT_MID, 0, 0);

    theme_switch = lv_switch_create(theme_container);
    lv_obj_align(theme_switch, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(theme_switch, theme_switch_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Parlaklık */
    lv_obj_t* brightness_container = lv_obj_create(tab);
    lv_obj_set_size(brightness_container, lv_pct(100), 80);
    lv_obj_clear_flag(brightness_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(brightness_container, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(brightness_container, LV_OPA_0, 0);
    lv_obj_set_style_pad_all(brightness_container, 5, 0);

    lv_obj_t* brightness_label = lv_label_create(brightness_container);
    lv_label_set_text(brightness_label, "Ekran Parlakligi");
    lv_obj_align(brightness_label, LV_ALIGN_TOP_LEFT, 0, 0);

    brightness_slider = lv_slider_create(brightness_container);
    lv_obj_set_size(brightness_slider, lv_pct(100), 20);
    lv_obj_align(brightness_slider, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_slider_set_range(brightness_slider, 10, 100);
    lv_slider_set_value(brightness_slider, 80, LV_ANIM_OFF);
    lv_obj_add_event_cb(brightness_slider, brightness_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Bellek Tasarrufu */
    lv_obj_t* memory_container = lv_obj_create(tab);
    lv_obj_set_size(memory_container, lv_pct(100), 60);
    lv_obj_clear_flag(memory_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(memory_container, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(memory_container, LV_OPA_0, 0);
    lv_obj_set_style_pad_all(memory_container, 5, 0);

    lv_obj_t* memory_label = lv_label_create(memory_container);
    lv_label_set_text(memory_label, "Bellek Tasarruf Modu");
    lv_obj_align(memory_label, LV_ALIGN_LEFT_MID, 0, 0);

    memory_switch = lv_switch_create(memory_container);
    lv_obj_align(memory_switch, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(memory_switch, memory_saver_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Güç Tasarrufu */
    lv_obj_t* power_container = lv_obj_create(tab);
    lv_obj_set_size(power_container, lv_pct(100), 60);
    lv_obj_clear_flag(power_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(power_container, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(power_container, LV_OPA_0, 0);
    lv_obj_set_style_pad_all(power_container, 5, 0);

    lv_obj_t* power_label = lv_label_create(power_container);
    lv_label_set_text(power_label, "Dusuk Guc Tuketimi Modu");
    lv_obj_align(power_label, LV_ALIGN_LEFT_MID, 0, 0);

    power_switch = lv_switch_create(power_container);
    lv_obj_align(power_switch, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(power_switch, power_saver_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

/*-------------------------------------------------
 *  CALLBACK’LER
 *------------------------------------------------*/
static void wifi_auto_connect_cb(lv_event_t*) {/* TODO */ }
static void ssh_access_cb(lv_event_t*) {/* TODO */ }
static void server_data_share_cb(lv_event_t*) {/* TODO */ }
static void theme_switch_cb(lv_event_t*) {/* TODO */ }
static void memory_saver_cb(lv_event_t*) {/* TODO */ }
static void power_saver_cb(lv_event_t*) {/* TODO */ }

static void forget_wifi_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    show_confirm_popup("Tum Wi-Fi kayitlari silinecek. Emin misiniz?", confirm_forget_wifi_cb);
}

static void date_time_cb(lv_event_t* e)
{
    LV_UNUSED(e);
   
    show_datetime_popup();
}

static void sync_time_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    show_info_popup("Saat senkronize edildi.");
}

static void check_updates_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    show_info_popup("Guncelleme kontrol ediliyor...");
}

static void factory_reset_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    show_confirm_popup("Tum ayarlar silinecek ve cihaz yeniden baslatilacak. Emin misiniz?",
        confirm_factory_reset_cb);
}

static void view_logs_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    show_logs_popup();
}

static void clear_logs_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    show_confirm_popup("Tum loglar silinecek. Emin misiniz?", confirm_clear_logs_cb);
}

static void screen_timeout_cb(lv_event_t* e)
{
    lv_obj_t* dropdown = (lv_obj_t*)lv_event_get_target(e);
    uint16_t option = lv_dropdown_get_selected(dropdown);
    LV_UNUSED(option);
    /* TODO: Kaydet */
}

static void brightness_cb(lv_event_t* e)
{
    lv_obj_t* slider = (lv_obj_t*)lv_event_get_target(e);
    int32_t   value = lv_slider_get_value(slider);
    LV_UNUSED(value);
    /* TODO: Parlaklığı ayarla */
}

/*--------- Onay popup callback’leri --------*/
static void confirm_forget_wifi_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    if (confirm_popup && lv_obj_is_valid(confirm_popup)) {
        lv_obj_del(confirm_popup);
        confirm_popup = nullptr;
    }

    delete_all_wifi_records();
    /* TODO: Wi-Fi kayıtlarını temizle */
    show_info_popup("Wi-Fi kayitlari temizlendi.");
}

static void confirm_factory_reset_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    if (confirm_popup && lv_obj_is_valid(confirm_popup)) {
        lv_obj_del(confirm_popup);
        confirm_popup = nullptr;
    }

    /* TODO: Fabrika ayarlarına dön */
    show_info_popup("Fabrika ayarlarina donuluyor...");
}

static void confirm_clear_logs_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    if (confirm_popup && lv_obj_is_valid(confirm_popup)) {
        lv_obj_del(confirm_popup);
        confirm_popup = nullptr;
    }

    /* TODO: Log temizleme */
    show_info_popup("Loglar temizlendi.");
}

/*--------- Popup yardımcı callback’leri --------*/
static void close_parent_popup_cb(lv_event_t* e)
{
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
    lv_obj_t* popup = lv_obj_get_parent(lv_obj_get_parent(btn));

    if (popup && lv_obj_is_valid(popup)) lv_obj_del(popup);

    if (popup == confirm_popup) confirm_popup = nullptr;
    if (popup == info_popup)    info_popup = nullptr;
    if (popup == logs_popup)    logs_popup = nullptr;
}

static void info_popup_ok_cb(lv_event_t* e)
{
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
    lv_obj_t* popup = lv_obj_get_parent(btn);

    if (popup && lv_obj_is_valid(popup)) lv_obj_del(popup);
    if (popup == info_popup) info_popup = nullptr;
}

static void logs_popup_close_cb(lv_event_t* e)
{
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
    lv_obj_t* popup = lv_obj_get_parent(btn);

    if (popup && lv_obj_is_valid(popup)) lv_obj_del(popup);
    if (popup == logs_popup) logs_popup = nullptr;
}

/*-------------------------------------------------
 *  POPUP OLUŞTURUCULAR
 *------------------------------------------------*/
static void show_confirm_popup(const char* message, lv_event_cb_t confirm_cb)
{
    if (confirm_popup && lv_obj_is_valid(confirm_popup)) {
        lv_obj_del(confirm_popup);
        confirm_popup = nullptr;
    }

    /* Popup oluştur */
    confirm_popup = lv_obj_create(lv_scr_act());
    lv_obj_set_size(confirm_popup, 300, 150);
    lv_obj_center(confirm_popup);
    lv_obj_add_style(confirm_popup, &style_screen_bg, 0);

    lv_obj_set_scrollbar_mode(confirm_popup, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(confirm_popup, LV_DIR_NONE);
    lv_obj_set_style_pad_all(confirm_popup, 0, 0);
    lv_obj_set_style_border_width(confirm_popup, 0, 0);

    /* Mesaj */
    static lv_style_t txt_style;
    static bool style_inited = false;
    if (!style_inited) {
        lv_style_init(&txt_style);
        lv_style_set_text_color(&txt_style, lv_color_white());
        style_inited = true;
    }

    lv_obj_t* label = lv_label_create(confirm_popup);
    lv_label_set_text(label, message);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, 280);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_add_style(label, &txt_style, LV_PART_MAIN);

    /* Buton kapsayıcısı */
    lv_obj_t* btn_container = lv_obj_create(confirm_popup);
    lv_obj_set_size(btn_container, 260, 50);
    lv_obj_align(btn_container, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_scrollbar_mode(btn_container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(btn_container, LV_DIR_NONE);
    lv_obj_set_style_bg_opa(btn_container, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(btn_container, LV_OPA_0, 0);
    lv_obj_set_style_pad_all(btn_container, 0, 0);

    /* İptal butonu */
    lv_obj_t* cancel_btn = lv_btn_create(btn_container);
    lv_obj_set_size(cancel_btn, 120, 40);
    lv_obj_align(cancel_btn, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_event_cb(cancel_btn, close_parent_popup_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, "Iptal");
    lv_obj_center(cancel_label);
    lv_obj_add_style(cancel_label, &txt_style, LV_PART_MAIN);

    /* Onayla butonu */
    lv_obj_t* confirm_btn = lv_btn_create(btn_container);
    lv_obj_set_size(confirm_btn, 120, 40);
    lv_obj_align(confirm_btn, LV_ALIGN_RIGHT_MID, 0, 0);
    if (confirm_cb) lv_obj_add_event_cb(confirm_btn, confirm_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* confirm_label = lv_label_create(confirm_btn);
    lv_label_set_text(confirm_label, "Onayla");
    lv_obj_center(confirm_label);
    lv_obj_add_style(confirm_label, &txt_style, LV_PART_MAIN);
}

static void show_info_popup(const char* message)
{
    if (info_popup && lv_obj_is_valid(info_popup)) {
        lv_obj_del(info_popup);
        info_popup = nullptr;
    }

    info_popup = lv_obj_create(lv_scr_act());
    lv_obj_set_size(info_popup, 300, 120);
    lv_obj_center(info_popup);
    lv_obj_add_style(info_popup, &style_screen_bg, 0);

    lv_obj_set_scrollbar_mode(info_popup, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(info_popup, LV_DIR_NONE);
    lv_obj_set_style_pad_all(info_popup, 0, 0);
    lv_obj_set_style_border_width(info_popup, 0, 0);

    static lv_style_t txt_style;
    static bool style_inited = false;
    if (!style_inited) {
        lv_style_init(&txt_style);
        lv_style_set_text_color(&txt_style, lv_color_white());
        style_inited = true;
    }

    lv_obj_t* label = lv_label_create(info_popup);
    lv_label_set_text(label, message);
    lv_obj_add_style(label, &txt_style, LV_PART_MAIN);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(label, 280);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t* ok_btn = lv_btn_create(info_popup);
    lv_obj_set_size(ok_btn, 100, 40);
    lv_obj_align(ok_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(ok_btn, info_popup_ok_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* ok_label = lv_label_create(ok_btn);
    lv_label_set_text(ok_label, "Tamam");
    lv_obj_center(ok_label);
    lv_obj_add_style(ok_label, &txt_style, LV_PART_MAIN);
}

static void show_logs_popup()
{
    if (logs_popup && lv_obj_is_valid(logs_popup)) {
        lv_obj_del(logs_popup);
        logs_popup = nullptr;
    }

    logs_popup = lv_obj_create(lv_scr_act());
    lv_obj_set_size(logs_popup, lv_pct(90), lv_pct(80));
    lv_obj_center(logs_popup);
    lv_obj_add_style(logs_popup, &style_screen_bg, 0);

    lv_obj_t* title = lv_label_create(logs_popup);
    lv_label_set_text(title, "Sistem Loglari");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t* list = lv_list_create(logs_popup);
    lv_obj_set_size(list, lv_pct(95), lv_pct(75));
    lv_obj_align(list, LV_ALIGN_CENTER, 0, -10);

#ifndef _WIN32
    // ✅ Linux: /etc/logs.txt dosyasını oku (son loglar en üste)
    std::ifstream infile("/etc/logs.txt");
    std::string line;
    std::vector<std::string> log_lines;

    if (infile.is_open()) {
        while (std::getline(infile, line)) {
            if (!line.empty()) log_lines.push_back(line);
        }
        infile.close();
    }

    if (!log_lines.empty()) {
        for (auto it = log_lines.rbegin(); it != log_lines.rend(); ++it) {
            lv_obj_t* btn = lv_list_add_button(list, LV_SYMBOL_FILE, it->c_str());
            lv_obj_add_style(btn, &style_button, 0);
        }
    }
    else {
        lv_obj_t* empty = lv_label_create(list);
        lv_label_set_text(empty, "Gosterilecek log bulunamadi.");
        lv_obj_add_style(empty, &style_label_white, 0);
        lv_obj_align(empty, LV_ALIGN_CENTER, 0, 0);
    }
#else
    // 💻 Windows: Simülasyon logları (sabit ve ters sırada gösterilir)
    const char* sample_logs[] = {
        "[Sim] Veri senkronize edildi",
        "[Sim] Sensor okuma basarili",
        "[Sim] Sensor okuma hatasi – yeniden deneniyor",
        "[Sim] Sistem durumu: Normal",
        "[Sim] Otomatik yedekleme tamamlandi"
    };

    for (int i = sizeof(sample_logs) / sizeof(sample_logs[0]) - 1; i >= 0; --i) {
        lv_obj_t* btn = lv_list_add_button(list, LV_SYMBOL_FILE, sample_logs[i]);
        lv_obj_add_style(btn, &style_button, 0);
    }
#endif

    lv_obj_t* close_btn = lv_btn_create(logs_popup);
    lv_obj_set_size(close_btn, 100, 40);
    lv_obj_align(close_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(close_btn, logs_popup_close_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* close_label = lv_label_create(close_btn);
    lv_label_set_text(close_label, "Kapat");
    lv_obj_center(close_label);
}

#ifndef _WIN32
/*-------------------------------------------------
 *  AYAR DOSYASI YÜKLE / KAYDET
 *------------------------------------------------*/
static void load_settings()
{
    settings_map.clear();
    std::ifstream in(SETTINGS_PATH);
    if (in.is_open()) {
        std::string line;
        while (std::getline(in, line)) {
            auto pos = line.find('=');
            if (pos != std::string::npos)
                settings_map[line.substr(0, pos)] = line.substr(pos + 1);
        }
        in.close();
    }

    auto get = [&](const std::string& key, const std::string& def) {
        return settings_map.count(key) ? settings_map[key] : def;
        };

    if (wifi_switch)       lv_obj_add_state(wifi_switch, get("WiFiAuto", "0") == "1" ? LV_STATE_CHECKED : 0);
    if (ssh_switch)        lv_obj_add_state(ssh_switch, get("SSH", "0") == "1" ? LV_STATE_CHECKED : 0);
    if (server_switch)     lv_obj_add_state(server_switch, get("SrvShare", "0") == "1" ? LV_STATE_CHECKED : 0);
    if (timeout_dropdown)  lv_dropdown_set_selected(timeout_dropdown, std::stoi(get("ScreenTimeout", "1")));
    if (theme_switch)      lv_obj_add_state(theme_switch, get("DarkMode", "0") == "1" ? LV_STATE_CHECKED : 0);
    if (brightness_slider) lv_slider_set_value(brightness_slider, std::stoi(get("Brightness", "80")), LV_ANIM_OFF);
    if (memory_switch)     lv_obj_add_state(memory_switch, get("MemSaver", "0") == "1" ? LV_STATE_CHECKED : 0);
    if (power_switch)      lv_obj_add_state(power_switch, get("PwrSaver", "0") == "1" ? LV_STATE_CHECKED : 0);
}

static void save_settings()
{
    settings_map = {
        {"WiFiAuto",      wifi_switch && lv_obj_has_state(wifi_switch,  LV_STATE_CHECKED) ? "1" : "0"},
        {"SSH",           ssh_switch && lv_obj_has_state(ssh_switch,   LV_STATE_CHECKED) ? "1" : "0"},
        {"SrvShare",      server_switch && lv_obj_has_state(server_switch,LV_STATE_CHECKED) ? "1" : "0"},
        {"ScreenTimeout", timeout_dropdown ? std::to_string(lv_dropdown_get_selected(timeout_dropdown)) : "1"},
        {"DarkMode",      theme_switch && lv_obj_has_state(theme_switch, LV_STATE_CHECKED) ? "1" : "0"},
        {"Brightness",    brightness_slider ? std::to_string(lv_slider_get_value(brightness_slider)) : "80"},
        {"MemSaver",      memory_switch && lv_obj_has_state(memory_switch,LV_STATE_CHECKED) ? "1" : "0"},
        {"PwrSaver",      power_switch && lv_obj_has_state(power_switch, LV_STATE_CHECKED) ? "1" : "0"}
    };

    std::ofstream out(SETTINGS_PATH, std::ios::trunc);
    if (!out.is_open()) {
        show_info_popup("/etc/settings.txt yazilamadi!");
        return;
    }

    for (const auto& kv : settings_map) out << kv.first << '=' << kv.second << '\n';
    out.close();
}
#endif
