# Uygulama Mimarisi

## Projenin Amacı

Bu örnek eğitim projesi, LVGL (Light and Versatile Graphics Library) kullanılarak gömülü Linux cihazlar için tam ekranlı, dokunmatik destekli bir kullanıcı arayüzü oluşturmayı amaçlamaktadır. Proje; Wi-Fi yapılandırması, sensör verilerinin izlenmesi, sistem bilgileri, stil yönetimi, ayarlar ve veri kaydı gibi bileşenleri kapsar.

## Genel Sistem Yapısı

Proje, üç ana modülden oluşur:

1. **Kullanıcı Arayüzü (GUI):**
   - LVGL ile yazılmış sayfalar
   - `ScreenManager` üzerinden yönetilen ekran geçişleri
   - `style.cpp` ile merkezi stil yönetimi

2. **Arka Plan Sistem İşlevleri:**
   - Wi-Fi bağlantısı ve ağı tarama (`systemfunctions.cpp`)
   - Sistem saati eşitleme, IP alma, shutdown işlemleri
   - Otomatik bağlantı mekanizması ve loglama

3. **Sensör Alt Sistemi:**
   - RS-485 üzerinden veri okuma (`serial_sensor.cpp`)
   - Simülasyon modu desteği
   - Anlık ve geçmiş verilerin dosyaya yazılması (`sensor_recorder.cpp`)
   - Sensör ayarları (`sensor_settings.cpp`)

## Dosya ve Katman Yapısı

| Dosya                     | Açıklama |
|--------------------------|----------|
| `main.cpp`               | Uygulama girişi, ilk ekran yükleme |
| `maingui.cpp`            | İlk açılan ana sayfa ekranı |
| `screen_manager.cpp`     | Sayfa geçişlerini yöneten singleton sınıf |
| `style.cpp`              | LVGL stil tanımları ve merkezi renk/font kontrolü |
| `header.cpp`             | Her sayfada üstte görünen başlık çubuğu |
| `wifi_settings.cpp`      | Wi-Fi tarama, bağlantı, parola girişi arayüzü |
| `sensor_settings.cpp`    | Sensör konfigürasyon sayfası (simülasyon, periyot, veri türleri) |
| `sensor_recorder.cpp`    | Gerçek/simüle sensör verisini okuyan ve kaydeden thread |
| `serial_sensor.cpp`      | RS-485 üzerinden veri alma işlevleri |
| `live_data.cpp`          | Anlık sensör verilerini gösteren ekran |
| `average_data.cpp`       | Ortalama veriler, kullanıcı tanımlı istatistik ve grafik |
| `systeminfo.cpp`         | Cihaz hakkında bilgiler (CPU, Wi-Fi, saat, kernel vb.) |
| `settings_screen.cpp`    | Uygulama genel ayarlarının tutulduğu merkezi sayfa |
| `systemfunctions.cpp`    | Wi-Fi yönetimi, zaman senkronizasyonu, cihaz kapatma, loglama |

## Arayüz Mimarisi

- Tüm sayfalar tam ekran `lv_obj_create()` ile oluşturulur.
- `create_header()` her sayfada üst bilgi çubuğunu ekler.
- `ScreenManager::register_screen()` ile ekranlar kayıt edilir.
- `style.cpp` içinde her nesne tipi için önceden tanımlı stiller (`style_button`, `style_title`, vb.) vardır.
- Simülasyon ve gerçek donanım farkı `#ifdef _WIN32` blokları ile ayrılmıştır.

## Wi-Fi Yönetimi

- `wifi_settings.cpp` ekranı, mevcut ağları tarayıp listeler.
- Kullanıcı, SSID ve parola girerek bağlanır.
- Bilinen ağlar `/etc/known_networks.txt` içinde saklanır.
- Bağlantı durumu `systemfunctions.cpp` içindeki `wifi_status_monitor_cb()` ile her 15 saniyede izlenir.
- Otomatik yeniden bağlanma desteklenir.

## Sensör Sistemi

- Gerçek cihazda RS-485 ile `/dev/ttyUSB0` üzerinden veri okunur.
- Simülasyon modu Windows’ta veya test amaçlı Linux'ta kullanılabilir.
- Veriler `/etc/sensor_data.txt` dosyasına yazılır.
- Veri türleri: sıcaklık, iletkenlik, basınç.
- Arka planda çalışan thread, `sensor_recorder.cpp` içinde yönetilir.

## Veri Görselleştirme

### Anlık Veriler (`live_data.cpp`)
- En son okunan değerleri büyük fontla gösterir.
- Arka planda çalışan thread'den alınan veri görüntülenir.

### Ortalama Veriler (`average_data.cpp`)
- Son X veri ya da son X dakika için ortalama hesaplar.
- Kullanıcı hangi verilerin (sıcaklık, basınç...) dahil edileceğini seçebilir.
- LVGL chart widget kullanılarak grafik gösterimi yapılır.

## Sistem Bilgisi

- `systeminfo.cpp`, sistemdeki temel bilgileri listeler.
- Örnek bilgiler: cihaz modeli, CPU çekirdeği, bağlı Wi-Fi, saat, IP, kernel sürümü.
- Linux komutları çalıştırılarak veriler alınır (`popen` + `grep/awk`).

## Bellek ve Güç Ayarları

- `settings_screen.cpp` ekranında kullanıcı; Wi-Fi otomatik bağlantı, SSH erişimi, ekran zaman aşımı, gece modu gibi ayarları değiştirebilir.
- Tüm ayarlar `/etc/settings.txt` dosyasında saklanır.

## Akış Diyagramı

1. `main.cpp` → `ScreenManager` başlatılır → `maingui.cpp` içindeki ana ekran gösterilir
2. Kullanıcı butonlar aracılığıyla ekranlar arasında geçiş yapar
3. Sayfa içindeki butonlar ayarları değiştirir veya sistem fonksiyonları çağırır
4. Arka planda:
   - Timer ile güncellenen bilgiler (Wi-Fi durumu, sistem bilgisi)
   - Thread ile okunan ve dosyaya yazılan sensör verileri

## Çoklu Görev ve Eşzamanlılık

- Sensör veri toplama işlemi `std::thread` ile bağımsız çalışır
- Wi-Fi bağlantısı arka planda başlatılır, tamamlandığında `on_done` fonksiyonu çağrılır
- LVGL'de GUI işlemleri asenkron güncellenir (`lv_async_call()`)

## Platform Uyumluluğu

- `#ifdef _WIN32` kullanımıyla Windows simülasyonu desteklenir
- Simülasyon ortamı için örnek veriler üretilir
- Gerçek cihazda `/dev`, `ifconfig`, `wpa_supplicant` gibi Linux araçları kullanılır

## Dosya İletişimi

| Dosya Yolu              | İçerik                        |
|-------------------------|-------------------------------|
| `/etc/settings.txt`     | Tüm kullanıcı ayarları        |
| `/etc/sensor_data.txt`  | Tarih/zaman damgalı sensor logu |
| `/etc/logs.txt`         | Sistem logları ve uyarılar    |
| `/etc/known_networks.txt` | Bilinen SSID:Şifre kayıtları  |

## Geliştirme Notları

- LVGL sürümü: 9.1.0
- Uygulama geliştirme aşamasındadır
- Fontlar: `lv_font_montserrat_20` ve `22`
- Uygulama CMake ile derlenir, hem gerçek cihaz hem de simülasyon desteklenir

---

