#include "systemfunctions.h"
#include "lvgl/lvgl.h"
#include "header.h"

#include <map>
#include <fstream>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <cstdio>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <ctime>

/*--------------------------------------------------------------------
 *  PLATFORM: WINDOWS (simülasyon)
 *------------------------------------------------------------------*/
#if defined(_WIN32)

void try_auto_connect_background() {}
void shutdown_device() {}

bool sync_time_from_api(const std::string&) { return true; }

std::vector<std::string> scan_wifi_networks() {
    return { "TestNet", "OfficeWiFi", "GuestNetwork" };
}
void disconnect_wifi() {}
bool is_wifi_connected_cached() { return true; }
bool is_wifi_connected() { return true; }

void connect_to_wifi(const std::string&, const std::string&,
    std::function<void(std::string)> on_done)
{
    std::thread([=]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        on_done("Baglandi: IP = 192.168.1.100");
        }).detach();
}

/*--------------------------------------------------------------------
 *  PLATFORM: LINUX / GERÇEK CİHAZ
 *------------------------------------------------------------------*/
#else   // ! _WIN32

 /*-----------------  LOGGING ALT YAPISI  -----------------------------*/
namespace {
    const char* LOG_PATH = "/etc/logs.txt";
    std::mutex   log_mutex;

    /* Zaman damgası üretir: [2025-07-07 14:25:36] */
    std::string get_timestamp()
    {
        std::time_t now = std::time(nullptr);
        char buf[32]{};
        std::strftime(buf, sizeof(buf), "[%Y-%m-%d %H:%M:%S] ",
            std::localtime(&now));
        return buf;
    }

    /* Mesajı /etc/logs.txt dosyasına (varsa oluşturarak) ekler */
    void log_message(const std::string& msg)
    {
        std::lock_guard<std::mutex> lock(log_mutex);
        std::ofstream out(LOG_PATH, std::ios::app);
        if (!out.is_open()) {
            std::cerr << "LOG dosyasi acilamadi: " << LOG_PATH << '\n';
            return;
        }
        out << get_timestamp() << msg << '\n';
    }
} // namespace (anon)

static std::atomic<bool> wifi_connected_cached(false);
static std::atomic<bool> wifi_connecting(false);

/*------------------  SAAT EŞİTLEME  --------------------------------*/
bool sync_time_from_api(const std::string& timezone)
{
    log_message("Zaman senkronizasyonu basladi. Zone=" + timezone);

    std::string cmd =
        "datetime=$(curl -s --max-time 5 'http://worldtimeapi.org/api/timezone/"
        + timezone +
        "' | grep -o '\"datetime\":\"[^\"]*' | cut -d'\"' -f4 | "
        "cut -d'.' -f1 | tr 'T' ' '); "
        "if [ -z \"$datetime\" ]; then exit 1; fi; "
        "date -s \"$datetime\"";

    int ret = std::system(cmd.c_str());

    if (ret != 0) {
        log_message("Zaman senkronizasyonu HATA");
        return false;
    }

    log_message("Zaman senkronizasyonu OK");
    return true;
}

/*------------------  Wi-Fi Bağlantı İşlemleri  ----------------------*/
void disconnect_wifi()
{
    std::system("killall wpa_supplicant; ifconfig wlan0 down");
    wifi_connected_cached.store(false);
    log_message("Wi-Fi baglanti KAPATILDI");
}

bool is_wifi_connected_cached() { return wifi_connected_cached.load(); }

static bool prev_connected = true;
static void wifi_status_monitor_cb(lv_timer_t*)
{
    bool now_connected = is_wifi_connected();
    wifi_connected_cached.store(now_connected);

    if (!now_connected && prev_connected) {
        log_message("Wi-Fi baglanti KOPTU – auto-reconnect deneniyor");
        try_auto_connect_background();
    }
    prev_connected = now_connected;
}

void start_wifi_monitoring() {
    lv_timer_create(wifi_status_monitor_cb, 15000, nullptr);
}

bool is_wifi_connected()
{
    FILE* f = popen("wpa_cli status | grep wpa_state", "r");
    if (!f) return false;
    char buf[128];
    bool ok = false;
    while (fgets(buf, sizeof(buf), f)) {
        if (strstr(buf, "COMPLETED")) { ok = true; break; }
    }
    pclose(f);
    return ok;
}

/*------------------  Sistemi Kapat  --------------------------------*/
void shutdown_device()
{
    log_message("Cihaz KAPATILIYOR (shutdown -h now)");
    std::system("sync");
    std::system("shutdown -h now");
}

/*------------------  IP Adresi Yardımcısı  --------------------------*/
std::string get_ip_address()
{
    std::string ip;
    std::system("ip -4 addr show wlan0 | awk '/inet / {print $2}' "
        "| cut -d/ -f1 > /tmp/ip_addr.txt 2>/dev/null");
    std::ifstream in("/tmp/ip_addr.txt");
    if (in.is_open()) std::getline(in, ip);
    return ip;
}

/*------------------  Otomatik Bağlanma  -----------------------------*/
void try_auto_connect_background()
{
    std::thread([] {
        log_message("Auto-connect arka planda basladi");

        std::system("ip link set wlan0 up > /dev/null 2>&1");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if (is_wifi_connected()) return;

        const char* path = "/etc/known_networks.txt";
        std::ifstream file(path);
        if (!file.is_open()) { log_message("known_networks.txt bulunamadi"); return; }

        std::map<std::string, std::string> known;
        std::string line;
        while (std::getline(file, line)) {
            size_t sep = line.find(':');
            if (sep != std::string::npos)
                known[line.substr(0, sep)] = line.substr(sep + 1);
        }
        file.close();

        std::vector<std::string> current = scan_wifi_networks();
        for (const auto& ssid : current) {
            if (!known.count(ssid)) continue;

            std::string pass = known[ssid];
            header_show_wifi_status("Baglaniyor", lv_palette_main(LV_PALETTE_YELLOW));
            log_message("Auto-connect denemesi SSID=" + ssid);

            connect_to_wifi(ssid, pass, [ssid, pass, path](std::string result) {
                if (result.find("Baglandi:") == std::string::npos) {
                    log_message("Auto-connect HATA SSID=" + ssid);

                    /* Başarısız SSID'yi listeden çıkar */
                    std::ifstream fin(path);
                    std::ofstream fout("/etc/known_networks.tmp");
                    std::string line;
                    while (std::getline(fin, line))
                        if (line.find(ssid + ":") != 0) fout << line << '\n';
                    fin.close(); fout.close();
                    std::remove(path);
                    std::rename("/etc/known_networks.tmp", path);

                    header_show_wifi_status("HATA", lv_palette_main(LV_PALETTE_RED));
                }
                else {
                    log_message("Auto-connect OK SSID=" + ssid);
                    header_show_wifi_status("✓", lv_palette_main(LV_PALETTE_GREEN));
                    std::this_thread::sleep_for(std::chrono::seconds(3));
                    header_clear_wifi_status();
                }
                });
            break;
        }
        }).detach();
}

/*------------------  Manuel Bağlan  --------------------------------*/
void connect_to_wifi(const std::string& ssid, const std::string& password,
    std::function<void(std::string)> on_done)
{
    std::thread([=] {
        log_message("Wi-Fi baglanti basladi SSID=" + ssid);

        if (password.length() < 8 || password.length() > 63) {
            log_message("Wi-Fi sifre GECERSIZ");
            on_done("Sifre 8-63 karakter olmalidir");
            return;
        }

        if (std::system("ip link show wlan0 > /dev/null 2>&1") != 0) {
            log_message("wlan0 arayuzu bulunamadi");
            on_done("wlan0 bulunamadi!");
            return;
        }

        std::system("ip link set wlan0 up > /dev/null 2>&1");

        /* Geçici wpa_supplicant dosyası oluştur */
        const char* cfg = "/tmp/wpa_dynamic.conf";
        {
            std::ofstream tmp(cfg);
            if (!tmp.is_open()) {
                log_message("Gecici wpa dosyasi yazilamadi");
                on_done("Gecici dosya yazilamadi!");
                return;
            }
            tmp << "ctrl_interface=/var/run/wpa_supplicant\n"
                << "update_config=1\ncountry=TR\nnetwork={\n"
                << "    ssid=\"" << ssid << "\"\n"
                << "    psk=\"" << password << "\"\n"
                << "    key_mgmt=WPA-PSK\n}\n";
        }

        /* wpa_supplicant başlat */
        std::system("killall wpa_supplicant > /dev/null 2>&1");
        std::system("rm -f /var/run/wpa_supplicant/wlan0 > /dev/null 2>&1");
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));

        bool ok = false;
        for (int i = 0; i < 3; ++i) {
            if (std::system("wpa_supplicant -B -i wlan0 "
                "-c /tmp/wpa_dynamic.conf > /tmp/wpa_log.txt 2>&1") == 0)
            {
                ok = true; break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        std::remove(cfg);

        if (!ok) {
            log_message("Wi-Fi baglantisi HATA SSID=" + ssid);
            on_done("Baglanti hatasi (wpa_supplicant)");
            return;
        }

        std::system("udhcpc -i wlan0 > /dev/null 2>&1");
        std::string ip = get_ip_address();
        if (ip.empty()) {
            log_message("DHCP IP alinamadi SSID=" + ssid);
            on_done("IP alinamadi!");
            return;
        }

        /* Bilinen ağlar listesine ekle */
        const char* known = "/etc/known_networks.txt";
        {
            std::ifstream in(known); bool found = false; std::string L;
            while (std::getline(in, L)) if (L.find(ssid + ":") == 0) { found = true; break; }
            if (!found) {
                std::ofstream out(known, std::ios::app);
                if (out.is_open()) out << ssid << ":" << password << '\n';
            }
        }

        log_message("Wi-Fi baglantisi OK SSID=" + ssid + " IP=" + ip);
        wifi_connected_cached.store(true);
        on_done("Baglandi: IP = " + ip);
        }).detach();
}

/*------------------  Ağ Tara  --------------------------------------*/
std::vector<std::string> scan_wifi_networks()
{
    std::vector<std::string> list;

    if (std::system("ip link show wlan0 | grep 'state UP' > /dev/null 2>&1") != 0) {
        std::system("ip link set wlan0 up > /dev/null 2>&1");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    FILE* fp = popen("iw dev wlan0 scan | grep SSID", "r");
    if (!fp) return list;

    char buf[256];
    while (fgets(buf, sizeof(buf), fp)) {
        std::string L(buf);
        size_t p = L.find("SSID: ");
        if (p != std::string::npos) {
            std::string ssid = L.substr(p + 6);
            ssid.erase(std::remove(ssid.begin(), ssid.end(), '\n'), ssid.end());
            if (!ssid.empty()) list.push_back(ssid);
        }
    }
    pclose(fp);
    return list;
}

#endif  /* ! _WIN32 */
