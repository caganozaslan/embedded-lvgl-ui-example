#include "lvgl/lvgl.h"
#include "screen_manager.h"
#include "style.h"
#include "header.h"
#include "average_data.h"
#include <thread>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>

#ifdef _WIN32
#define IS_SIMULATOR 1
#else
#define IS_SIMULATOR 0
#include <fstream>
#endif

#ifdef min
#undef min
#endif

// ====================== GUI Bileşenleri ======================
static lv_obj_t* keyboard = nullptr;
static lv_obj_t* cb_temp = nullptr;
static lv_obj_t* cb_cond = nullptr;
static lv_obj_t* cb_pres = nullptr;
static lv_obj_t* screen = nullptr;
static lv_obj_t* chart = nullptr;
static lv_obj_t* dropdown_chart_type = nullptr;
static lv_obj_t* ta_last_x = nullptr;
static lv_obj_t* label_avg_x = nullptr;
static lv_obj_t* ta_last_min = nullptr;
static lv_obj_t* label_avg_min = nullptr;
static lv_obj_t* btn_update_left = nullptr;
static lv_obj_t* btn_update_right = nullptr;
static lv_obj_t* btn_update_chart = nullptr;

static std::vector<lv_chart_series_t*> active_series;

struct SensorData {
    std::tm timestamp{};
    float   temp = 0;
    float   cond = 0;
    float   pres = 0;
};

static const char* simulated_data_text = R"(
2025-06-24 16:33:19, Temp: 19.559000, Cond: 30.724001, Pres: -0.000002
2025-06-24 16:33:22, Temp: 19.009001, Cond: 30.848000, Pres: -0.000005
2025-06-24 16:33:25, Temp: 19.257999, Cond: 30.965000, Pres: -0.000019
2025-06-24 16:33:28, Temp: 19.420000, Cond: 30.920000, Pres: -0.000020
2025-06-24 16:33:31, Temp: 19.390000, Cond: 30.905000, Pres: -0.000017
2025-06-24 16:33:34, Temp: 19.360000, Cond: 30.890000, Pres: -0.000014
2025-06-24 16:33:37, Temp: 19.350000, Cond: 30.870000, Pres: -0.000010
2025-06-24 16:33:40, Temp: 19.340000, Cond: 30.860000, Pres: -0.000007
2025-06-24 16:33:43, Temp: 19.330000, Cond: 30.850000, Pres: -0.000005
2025-06-24 16:33:46, Temp: 19.320000, Cond: 30.840000, Pres: -0.000002
2025-06-24 16:33:47, Temp: 21.320000, Cond: 33.840000, Pres: -0.000002
2025-06-24 16:33:48, Temp: 17.320000, Cond: 30.840000, Pres: -0.000002
2025-06-24 16:33:49, Temp: 19.320000, Cond: 31.840000, Pres: -0.000002
2025-06-24 16:33:52, Temp: 17.320000, Cond: 30.840000, Pres: -0.000002
2025-06-24 16:33:55, Temp: 14.320000, Cond: 25.840000, Pres: -0.000002
)";

// --- Ortak ayrıştırıcı ---
static std::vector<SensorData> parse_lines(std::istream& stream)
{
    std::vector<SensorData> data;
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty()) continue;
        SensorData sd;
        char date[11], time[9];
        if (sscanf(line.c_str(), "%10s %8s, Temp: %f, Cond: %f, Pres: %f",
            date, time, &sd.temp, &sd.cond, &sd.pres) == 5) {
            std::string dt = std::string(date) + " " + std::string(time);
            std::istringstream ss(dt);
            ss >> std::get_time(&sd.timestamp, "%Y-%m-%d %H:%M:%S");
            if (!ss.fail()) data.push_back(sd);
        }
    }
    return data;
}

#if IS_SIMULATOR
static std::vector<SensorData> load_data()
{
    std::istringstream s(simulated_data_text);
    return parse_lines(s);
}
#else
static std::vector<SensorData> load_data()
{
    std::ifstream f("/etc/sensor_data.txt");
    if (!f) {
        std::istringstream s(simulated_data_text);
        return parse_lines(s);
    }
    return parse_lines(f);
}
#endif

static int textarea_get_int(lv_obj_t* ta, int fallback)
{
    const char* txt = lv_textarea_get_text(ta);
    return (txt && txt[0] != '\0') ? std::atoi(txt) : fallback;
}

static void update_average_by_count(int count)
{
    auto data = load_data();
    int total = std::min(count, static_cast<int>(data.size()));
    if (total == 0) return;

    float sumT = 0, sumC = 0, sumP = 0;
    for (int i = data.size() - total; i < data.size(); ++i) {
        sumT += data[i].temp;
        sumC += data[i].cond;
        sumP += data[i].pres;
    }

    char buf[128];
    snprintf(buf, sizeof(buf),
        "Ortalama:\nT=%.2f\nC=%.2f\nP=%.5f",
        sumT / total, sumC / total, sumP / total);
    lv_label_set_text(label_avg_x, buf);
}

static void update_average_by_minute(int minutes)
{
    auto data = load_data();
    if (data.empty()) return;

#if IS_SIMULATOR
    std::tm now_tm{};
    std::istringstream ss("2025-06-24 17:00:00");
    ss >> std::get_time(&now_tm, "%Y-%m-%d %H:%M:%S");
    time_t now = mktime(&now_tm);
#else
    time_t now = std::time(nullptr);
#endif

    time_t threshold = now - minutes * 60;
    float sumT = 0, sumC = 0, sumP = 0;
    int cnt = 0;

    for (auto& d : data) {
        time_t t = mktime(const_cast<std::tm*>(&d.timestamp));
        if (t >= threshold) {
            sumT += d.temp;
            sumC += d.cond;
            sumP += d.pres;
            ++cnt;
        }
    }
    if (cnt == 0) return;

    char buf[128];
    snprintf(buf, sizeof(buf),
        "Ortalama:\nT=%.2f\nC=%.2f\nP=%.5f",
        sumT / cnt, sumC / cnt, sumP / cnt);
    lv_label_set_text(label_avg_min, buf);
}

static void rebuild_chart()
{
    // Thread içinde çalışacak işlemler
    std::thread([] {
        auto data = load_data();
        const int max_points = 20;
        int point_count = std::min(static_cast<int>(data.size()), max_points);
        if (point_count == 0) return;

        // Seri değerlerini önceden oluştur
        std::vector<float> temps, conds, press;
        for (int i = data.size() - point_count; i < static_cast<int>(data.size()); ++i) {
            temps.push_back(data[i].temp);
            conds.push_back(data[i].cond);
            press.push_back(data[i].pres);
        }

        // GUI işlemleri ana thread'e atılır
        lv_async_call([](void* param) {
            auto* d = static_cast<std::tuple<std::vector<float>, std::vector<float>, std::vector<float>>*>(param);
            const auto& temps = std::get<0>(*d);
            const auto& conds = std::get<1>(*d);
            const auto& press = std::get<2>(*d);

            for (auto* s : active_series) lv_chart_remove_series(chart, s);
            active_series.clear();

            lv_chart_set_point_count(chart, temps.size());

            uint16_t sel = lv_dropdown_get_selected(dropdown_chart_type);
            lv_chart_set_type(chart,
                sel == 0 ? LV_CHART_TYPE_LINE :
                sel == 1 ? LV_CHART_TYPE_BAR :
                LV_CHART_TYPE_SCATTER);

            bool showT = lv_obj_has_state(cb_temp, LV_STATE_CHECKED);
            bool showC = lv_obj_has_state(cb_cond, LV_STATE_CHECKED);
            bool showP = lv_obj_has_state(cb_pres, LV_STATE_CHECKED);

            lv_chart_series_t* sT = nullptr;
            lv_chart_series_t* sC = nullptr;
            lv_chart_series_t* sP = nullptr;

            if (showT) { sT = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y); active_series.push_back(sT); }
            if (showC) { sC = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y); active_series.push_back(sC); }
            if (showP) { sP = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y); active_series.push_back(sP); }

            for (size_t i = 0; i < temps.size(); ++i) {
                if (sT) lv_chart_set_next_value(chart, sT, temps[i]);
                if (sC) lv_chart_set_next_value(chart, sC, conds[i]);
                if (sP) lv_chart_set_next_value(chart, sP, press[i]);
            }

            lv_chart_refresh(chart);
            delete d;
            }, new std::tuple<std::vector<float>, std::vector<float>, std::vector<float>>(temps, conds, press));
        }).detach();  // thread'i arka planda başlat
}


static void show_keyboard(lv_obj_t* target)
{
    if (keyboard) lv_obj_del(keyboard);
    keyboard = lv_keyboard_create(screen);
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_NUMBER);
    lv_keyboard_set_textarea(keyboard, target);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
}

static void hide_keyboard()
{
    if (keyboard) { lv_obj_del(keyboard); keyboard = nullptr; }
}


// GUI Alanı
void create_average_data_screen()
{
    if (screen) return;
    init_styles();

    // Ana ekran
    screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(screen, lv_pct(100), lv_pct(100));
    lv_obj_add_style(screen, &style_screen_bg, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    create_header(screen);

    // Başlık
    lv_obj_t* title = lv_label_create(screen);
    lv_label_set_text(title, "Ortalama Veriler");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);

    const int panel_h = 300;

    // -------- SOL PANEL --------
    lv_obj_t* left_panel = lv_obj_create(screen);
    lv_obj_set_size(left_panel, 160, panel_h);
    lv_obj_add_style(left_panel, &style_screen_bg, 0);
    lv_obj_set_style_pad_all(left_panel, 8, 0);
    lv_obj_align(left_panel, LV_ALIGN_LEFT_MID, 25, 0);

    lv_obj_t* lbl_x = lv_label_create(left_panel);
    lv_label_set_text(lbl_x, "Son X Veri");
    lv_obj_add_style(lbl_x, &style_label_white, 0);
    lv_obj_align(lbl_x, LV_ALIGN_TOP_MID, 0, 0);

    ta_last_x = lv_textarea_create(left_panel);
    lv_textarea_set_one_line(ta_last_x, true);     // ← ekle
    lv_obj_clear_flag(ta_last_x, LV_OBJ_FLAG_SCROLLABLE);  // ← ekle
    lv_obj_set_size(ta_last_x, 100, 30);
    lv_textarea_set_placeholder_text(ta_last_x, "10");
    lv_obj_align_to(ta_last_x, lbl_x, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_add_event_cb(ta_last_x, [](lv_event_t*){ show_keyboard(ta_last_x); }, LV_EVENT_FOCUSED, nullptr);

    btn_update_left = lv_btn_create(left_panel);
    lv_obj_set_size(btn_update_left, 100, 30);
    lv_obj_add_style(btn_update_left, &style_button, 0);
    lv_obj_align_to(btn_update_left, ta_last_x, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_add_event_cb(btn_update_left, [](lv_event_t*){
        hide_keyboard();
        update_average_by_count(textarea_get_int(ta_last_x, 10));
    }, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* lbl_btn_left = lv_label_create(btn_update_left);
    lv_label_set_text(lbl_btn_left, "Guncelle");
    lv_obj_center(lbl_btn_left);
    lv_obj_add_style(lbl_btn_left, &style_label_white, 0);

    label_avg_x = lv_label_create(left_panel);
    lv_label_set_text(label_avg_x, "Ortalama: -");
    lv_obj_add_style(label_avg_x, &style_label_white, 0);
    lv_obj_align_to(label_avg_x, btn_update_left, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);

    // -------- SAĞ PANEL --------
    lv_obj_t* right_panel = lv_obj_create(screen);
    lv_obj_set_size(right_panel, 160, panel_h);
    lv_obj_add_style(right_panel, &style_screen_bg, 0);
    lv_obj_set_style_pad_all(right_panel, 8, 0);
    lv_obj_align(right_panel, LV_ALIGN_RIGHT_MID, -25, 0);

    lv_obj_t* lbl_min = lv_label_create(right_panel);
    lv_label_set_text(lbl_min, "Son X Dakika");
    lv_obj_add_style(lbl_min, &style_label_white, 0);
    lv_obj_align(lbl_min, LV_ALIGN_TOP_MID, 0, 0);

    ta_last_min = lv_textarea_create(right_panel);
    lv_textarea_set_one_line(ta_last_min, true);   // ← ekle
    lv_obj_clear_flag(ta_last_min, LV_OBJ_FLAG_SCROLLABLE); // ← ekle
    lv_obj_set_size(ta_last_min, 100, 30);
    lv_textarea_set_placeholder_text(ta_last_min, "10");
    lv_obj_align_to(ta_last_min, lbl_min, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_add_event_cb(ta_last_min, [](lv_event_t*){ show_keyboard(ta_last_min); }, LV_EVENT_FOCUSED, nullptr);

    btn_update_right = lv_btn_create(right_panel);
    lv_obj_set_size(btn_update_right, 100, 30);
    lv_obj_add_style(btn_update_right, &style_button, 0);
    lv_obj_align_to(btn_update_right, ta_last_min, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_add_event_cb(btn_update_right, [](lv_event_t*){
        hide_keyboard();
        update_average_by_minute(textarea_get_int(ta_last_min, 10));
    }, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* lbl_btn_right = lv_label_create(btn_update_right);
    lv_label_set_text(lbl_btn_right, "Guncelle");
    lv_obj_center(lbl_btn_right);
    lv_obj_add_style(lbl_btn_right, &style_label_white, 0);

    label_avg_min = lv_label_create(right_panel);
    lv_label_set_text(label_avg_min, "Ortalama: -");
    lv_obj_add_style(label_avg_min, &style_label_white, 0);
    lv_obj_align_to(label_avg_min, btn_update_right, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);


    // -------- ORTA PANEL (Grafik) --------
    lv_obj_t* center_panel = lv_obj_create(screen);
    lv_obj_set_size(center_panel, 440, panel_h);
    lv_obj_add_style(center_panel, &style_screen_bg, 0);
    lv_obj_set_style_pad_all(center_panel, 10, 0);
    lv_obj_align(center_panel, LV_ALIGN_CENTER, 0, 0);

    dropdown_chart_type = lv_dropdown_create(center_panel);
    lv_dropdown_set_options(dropdown_chart_type, "Cizgi\nBar\nScatter");
    lv_obj_set_width(dropdown_chart_type, 150);
    lv_obj_align(dropdown_chart_type, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_event_cb(dropdown_chart_type, [](lv_event_t*){ rebuild_chart(); }, LV_EVENT_VALUE_CHANGED, nullptr);

    cb_temp = lv_checkbox_create(center_panel);
    lv_checkbox_set_text(cb_temp, "Sicaklik");
    lv_obj_add_style(cb_temp, &style_label_white, 0);

    lv_obj_align(cb_temp, LV_ALIGN_TOP_MID, -110, 55);
    lv_obj_add_event_cb(cb_temp, [](lv_event_t*){ rebuild_chart(); }, LV_EVENT_VALUE_CHANGED, nullptr);

    cb_cond = lv_checkbox_create(center_panel);
    lv_checkbox_set_text(cb_cond, "Iletkenlik");
    lv_obj_add_style(cb_cond, &style_label_white, 0);

    lv_obj_align(cb_cond, LV_ALIGN_TOP_MID, 0, 55);
    lv_obj_add_event_cb(cb_cond, [](lv_event_t*){ rebuild_chart(); }, LV_EVENT_VALUE_CHANGED, nullptr);

    cb_pres = lv_checkbox_create(center_panel);
    lv_checkbox_set_text(cb_pres, "Basinc");
    lv_obj_add_style(cb_pres, &style_label_white, 0);

    lv_obj_align(cb_pres, LV_ALIGN_TOP_MID, 110, 55);
    lv_obj_add_event_cb(cb_pres, [](lv_event_t*){ rebuild_chart(); }, LV_EVENT_VALUE_CHANGED, nullptr);

    chart = lv_chart_create(center_panel);
    lv_obj_set_size(chart, 400, 140);
    lv_obj_align(chart, LV_ALIGN_CENTER, 0, 40);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);

    rebuild_chart();            // İlk çizim

    // -------- Grafik “Güncelle” --------
    btn_update_chart = lv_btn_create(screen);
    lv_obj_set_size(btn_update_chart, 120, 35);
    lv_obj_align(btn_update_chart, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(btn_update_chart, lv_color_hex(0x2ecc71), 0);
    lv_obj_set_style_bg_opa(btn_update_chart, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn_update_chart, 6, 0);
    lv_obj_add_event_cb(btn_update_chart, [](lv_event_t*){ hide_keyboard(); rebuild_chart(); }, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* lbl_chart_btn = lv_label_create(btn_update_chart);
    lv_label_set_text(lbl_chart_btn, "Guncelle");
    lv_obj_center(lbl_chart_btn);
    lv_obj_add_style(lbl_chart_btn, &style_label_white, 0);

    // -------- Geri Butonu --------
    lv_obj_t* btn_back = lv_btn_create(screen);
    lv_obj_set_size(btn_back, 100, 40);
    lv_obj_add_style(btn_back, &style_button, 0);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_add_event_cb(btn_back, [](lv_event_t*){ hide_keyboard(); ScreenManager::get_instance().show_screen(0); }, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, "Geri");
    lv_obj_center(lbl_back);
    lv_obj_add_style(lbl_back, &style_label_white, 0);

    // Ekrana tıklayınca klavyeyi kapat
    lv_obj_add_event_cb(screen, [](lv_event_t* e){
        if (lv_event_get_code(e) == LV_EVENT_CLICKED) hide_keyboard();
    }, LV_EVENT_CLICKED, nullptr);

    ScreenManager::get_instance().register_screen(screen, []{/* tekrar gösterildiğinde yapılacaklar */});
}
