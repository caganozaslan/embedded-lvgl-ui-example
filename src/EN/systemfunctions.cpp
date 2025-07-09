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
 *  Windows Simulator Section
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
        on_done("Connected: IP = 192.168.1.100");
        }).detach();
}

/*--------------------------------------------------------------------
 *  Linux Device Section
 *------------------------------------------------------------------*/
#else   // ! _WIN32

 /*-----------------  Logging Section  -----------------------------*/
namespace {
    const char* LOG_PATH = "/etc/logs.txt";
    std::mutex   log_mutex;

    std::string get_timestamp()
    {
        std::time_t now = std::time(nullptr);
        char buf[32]{};
        std::strftime(buf, sizeof(buf), "[%Y-%m-%d %H:%M:%S] ",
            std::localtime(&now));
        return buf;
    }

    void log_message(const std::string& msg)
    {
        std::lock_guard<std::mutex> lock(log_mutex);
        std::ofstream out(LOG_PATH, std::ios::app);
        if (!out.is_open()) {
            std::cerr << "Failed to open log file: " << LOG_PATH << '\n';
            return;
        }
        out << get_timestamp() << msg << '\n';
    }
}

static std::atomic<bool> wifi_connected_cached(false);
static std::atomic<bool> wifi_connecting(false);

/*------------------  TIME SYNC (Not stable, consider switching to NTP)  --------------------------------*/
bool sync_time_from_api(const std::string& timezone)
{
    log_message("Time synchronization started. Zone=" + timezone);

    std::string cmd =
        "datetime=$(curl -s --max-time 5 'http://worldtimeapi.org/api/timezone/"
        + timezone +
        "' | grep -o '\"datetime\":\"[^\"]*' | cut -d'\"' -f4 | "
        "cut -d'.' -f1 | tr 'T' ' '); "
        "if [ -z \"$datetime\" ]; then exit 1; fi; "
        "date -s \"$datetime\"";

    int ret = std::system(cmd.c_str());

    if (ret != 0) {
        log_message("Time sync FAILED");
        return false;
    }

    log_message("Time sync SUCCESSFUL!");
    return true;
}

/*------------------  Wi-Fi Connection Handling  ----------------------*/
void disconnect_wifi()
{
    std::system("killall wpa_supplicant; ifconfig wlan0 down");
    wifi_connected_cached.store(false);
    log_message("Wi-Fi connection DISCONNECTED");
}

bool is_wifi_connected_cached() { return wifi_connected_cached.load(); }

static bool prev_connected = true;
static void wifi_status_monitor_cb(lv_timer_t*)
{
    bool now_connected = is_wifi_connected();
    wifi_connected_cached.store(now_connected);

    if (!now_connected && prev_connected) {
        log_message("Wi-Fi DISCONNECTED – attempting auto-reconnect");
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

/*------------------  System Shutdown  --------------------------------*/
void shutdown_device()
{
    log_message("Device SHUTTING DOWN (shutdown -h now)");
    std::system("sync");
    std::system("shutdown -h now");
}

/*------------------  IP Address  --------------------------*/
std::string get_ip_address()
{
    std::string ip;
    std::system("ip -4 addr show wlan0 | awk '/inet / {print $2}' "
        "| cut -d/ -f1 > /tmp/ip_addr.txt 2>/dev/null");
    std::ifstream in("/tmp/ip_addr.txt");
    if (in.is_open()) std::getline(in, ip);
    return ip;
}

/*------------------  Auto Connect  -----------------------------*/
void try_auto_connect_background()
{
    std::thread([] {
        log_message("Auto-connect started in background");

        std::system("ip link set wlan0 up > /dev/null 2>&1");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if (is_wifi_connected()) return;

        const char* path = "/etc/known_networks.txt";
        std::ifstream file(path);
        if (!file.is_open()) { log_message("known_networks.txt not found"); return; }

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
            header_show_wifi_status("Connecting", lv_palette_main(LV_PALETTE_YELLOW));
            log_message("Auto-connect attempt SSID=" + ssid);

            connect_to_wifi(ssid, pass, [ssid, pass, path](std::string result) {
                if (result.find("Connected:") == std::string::npos) {
                    log_message("Auto-connect FAILED SSID=" + ssid);

                    std::ifstream fin(path);
                    std::ofstream fout("/etc/known_networks.tmp");
                    std::string line;
                    while (std::getline(fin, line))
                        if (line.find(ssid + ":") != 0) fout << line << '\n';
                    fin.close(); fout.close();
                    std::remove(path);
                    std::rename("/etc/known_networks.tmp", path);

                    header_show_wifi_status("ERROR", lv_palette_main(LV_PALETTE_RED));
                }
                else {
                    log_message("Auto-connect SUCCESS SSID=" + ssid);
                    header_show_wifi_status("✓", lv_palette_main(LV_PALETTE_GREEN));
                    std::this_thread::sleep_for(std::chrono::seconds(3));
                    header_clear_wifi_status();
                }
                });
            break;
        }
        }).detach();
}

/*------------------  Manual Connection  --------------------------------*/
void connect_to_wifi(const std::string& ssid, const std::string& password,
    std::function<void(std::string)> on_done)
{
    std::thread([=] {
        log_message("Wi-Fi connection started SSID=" + ssid);

        if (password.length() < 8 || password.length() > 63) {
            log_message("Wi-Fi password INVALID");
            on_done("Password must be 8–63 characters");
            return;
        }

        if (std::system("ip link show wlan0 > /dev/null 2>&1") != 0) {
            log_message("wlan0 interface not found");
            on_done("wlan0 not found!");
            return;
        }

        std::system("ip link set wlan0 up > /dev/null 2>&1");

        const char* cfg = "/tmp/wpa_dynamic.conf";
        {
            std::ofstream tmp(cfg);
            if (!tmp.is_open()) {
                log_message("Temporary wpa config write failed");
                on_done("Failed to write temporary file!");
                return;
            }
            tmp << "ctrl_interface=/var/run/wpa_supplicant\n"
                << "update_config=1\ncountry=TR\nnetwork={\n"
                << "    ssid=\"" << ssid << "\"\n"
                << "    psk=\"" << password << "\"\n"
                << "    key_mgmt=WPA-PSK\n}\n";
        }

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
            log_message("Wi-Fi connection FAILED SSID=" + ssid);
            on_done("Connection error (wpa_supplicant)");
            return;
        }

        std::system("udhcpc -i wlan0 > /dev/null 2>&1");
        std::string ip = get_ip_address();
        if (ip.empty()) {
            log_message("DHCP failed to get IP SSID=" + ssid);
            on_done("Failed to obtain IP!");
            return;
        }

        // Save network
        const char* known = "/etc/known_networks.txt";
        {
            std::ifstream in(known); bool found = false; std::string L;
            while (std::getline(in, L)) if (L.find(ssid + ":") == 0) { found = true; break; }
            if (!found) {
                std::ofstream out(known, std::ios::app);
                if (out.is_open()) out << ssid << ":" << password << '\n';
            }
        }

        log_message("Wi-Fi connection OK SSID=" + ssid + " IP=" + ip);
        wifi_connected_cached.store(true);
        on_done("Connected: IP = " + ip);
        }).detach();
}

/*------------------  Network Scanning  --------------------------------------*/
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

#endif
