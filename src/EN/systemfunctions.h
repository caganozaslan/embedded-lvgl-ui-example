#ifndef SYSTEMFUNCTIONS_H
#define SYSTEMFUNCTIONS_H






// C C++ ayar için detaylar var bu dosyaya dokunma
#ifdef __cplusplus
extern "C" {
#endif

    void connect_to_wifi(const char* ssid, const char* password);
    bool is_wifi_connected();
    bool is_wifi_connected_cached();     
    void start_wifi_monitoring();        
    void disconnect_wifi();

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus
#include <string>
#include <vector>
#include <functional>

bool sync_time_from_api(const std::string& timezone);
std::vector<std::string> scan_wifi_networks();
std::string get_ip_address();

void connect_to_wifi(const std::string& ssid,
    const std::string& password,
    std::function<void(std::string)> on_done);

void try_auto_connect_background();
void shutdown_device();
#endif 

#endif 
