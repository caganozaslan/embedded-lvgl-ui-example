# Kullanım Kılavuzu

## Giriş

Bu belge, LVGL tabanlı gömülü arayüz projesinin nasıl derleneceğini, çalıştırılacağını ve kullanılacağını açıklamaktadır. Sistem hem gerçek Linux cihazlar üzerinde hem de Windows simülatörü ile çalışacak şekilde tasarlanmıştır. Arayüz, modüler bir yapıya sahip olup çeşitli sayfalardan oluşur ve her biri sistemsel işlevleri veya veri görüntüleme görevlerini yerine getirir.

## Gereksinimler

### Linux Tabanlı Gömülü Cihazlar İçin

Projenin tüm testleri Yocto tabanlı özel Linux dağıtımı ile yapılmıştır. Proje tüm Linux cihazlarda kullanılabilir yapıdadır ancak tüm gereksinimlere sahip olduğundan emin olmanız gerekir.

- Gerekli paketler: iproute2, iw, wpa-supplicant, psmisc, net-tools, coreutils, grep, gawk, nano.
- LVGL 9.1
- CMake >= 3.13
- GCC / G++ (C++17 desteği olan)
- Linux framebuffer desteği (örneğin: /dev/fb0)
- Gerekli izinler: /etc dizinine yazma


### Windows Simulator İçin

- Visual Studio IDE (Test edilen 2022 versiyonu)
  - Desktop development with C++
  - Windows 10 SDK veya Windows 11 SDK
- 8 GB RAM
- LVGL 9.1

## Derleme ve Çalıştırma

### Linux Ortamı İçin

Örnek projeyi indirmek ve derlemek için aşağıdaki adımları uygulayabilirsiniz.

```bash
git clone --recurse-submodules https://github.com/caganozaslan/embedded-lvgl-ui-example/src
cd PROJECT-FOLDER
mkdir build
cmake -B build -S .
make -C build -j
```

Derlenen çıktı bin klasörü altında **main** adında bulunacaktır.


### Windows Ortamı İçin

Windows simülatörü kuracağınız klasöre gidin ve ardından: 

```bash
git clone --recurse-submodules https://github.com/lvgl/lv_port_pc_visual_studio.git
```

İlgili repodaki Windows simülatör kurulum adımlarını takip ederek simülatör ortamının hazırlandığından emin olun.

**LvglWindowssimulator.cpp** dosyasını açın ve aşağıdaki satırları bulup yorum satırı haline çevirin.

```bash
lv_demo_widgets();
lv_demo_benchmark();
```

Ardından projedeki gerekli header dosyaları (bu projede screen içeren tüm dosyalar buraya dahildir) en üste `#include live_data.h` formatında ekleyin. Bu aşamadan itibaren yorum satırı yaptığınız demo bölümüne kendi fonksiyonunuzu ekleyebilirsiniz. İlgili dosyanın tam hali aşağıdaki gibidir:

```cpp
#include <Windows.h>

#include <LvglWindowsIconResource.h>
#include "screen_manager.h"
#include "my_app.h"
#include "maingui.h"
#include "lvgl/lvgl.h"
#include "lvgl/examples/lv_examples.h"
#include "lvgl/demos/lv_demos.h"
#include "systeminfo.h"
#include "sensor_settings.h"
#include "live_data.h"
#include "average_data.h"
#include "settings_screen.h"


extern "C" {
    #include "maingui.h"
}
int main()
{
    lv_init();

    /*
     * Optional workaround for users who wants UTF-8 console output.
     * If you don't want that behavior can comment them out.
     *
     * Suggested by jinsc123654.
     */
#if LV_TXT_ENC == LV_TXT_ENC_UTF8
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

    int32_t zoom_level = 100;
    bool allow_dpi_override = false;
    bool simulator_mode = true;
    lv_display_t* display = lv_windows_create_display(
        L"LVGL Windows Simulator Display 1",
        800,
        480,
        zoom_level,
        allow_dpi_override,
        simulator_mode);
    if (!display)
    {
        return -1;
    }

    HWND window_handle = lv_windows_get_display_window_handle(display);
    if (!window_handle)
    {
        return -1;
    }

    HICON icon_handle = LoadIconW(
        GetModuleHandleW(NULL),
        MAKEINTRESOURCE(IDI_LVGL_WINDOWS));
    if (icon_handle)
    {
        SendMessageW(
            window_handle,
            WM_SETICON,
            TRUE,
            (LPARAM)icon_handle);
        SendMessageW(
            window_handle,
            WM_SETICON,
            FALSE,
            (LPARAM)icon_handle);
    }

    lv_indev_t* pointer_indev = lv_windows_acquire_pointer_indev(display);
    if (!pointer_indev)
    {
        return -1;
    }

    lv_indev_t* keypad_indev = lv_windows_acquire_keypad_indev(display);
    if (!keypad_indev)
    {
        return -1;
    }

    lv_indev_t* encoder_indev = lv_windows_acquire_encoder_indev(display);
    if (!encoder_indev)
    {
        return -1;
    }

    //lv_demo_widgets();
    //lv_demo_benchmark();
    
	
    create_main_screen();
    create_wifi_screen();
    create_system_info_screen();
    create_sensor_settings_screen(); 
    create_live_data_screen();
    create_average_data_screen();
    create_settings_screen();
    ScreenManager::get_instance().show_screen(0); 
    while (1)
    {
        uint32_t time_till_next = lv_timer_handler();
        lv_delay_ms(time_till_next);
    }

    return 0;
}
```

Bu aşamadan itibaren tüm proje dosyalarını simülatörün bulunduğu klasöre koyun ve Visual Studio IDE üzerinden DEBUG başlatın. Simülasyon ekranı karşınıza gelecektir.

## Önemli Uyarılar

- Örnek uygulama Framebuffer görüntü altyapısına göre yapılandırılmıştır. Bu nedenle aktif bir masaüstü ortamı çalışmaması gerekir. Eğer masaüstü ortamı çalıştırmak istiyorsanız uygulamanın görüntü altyapısını SDL olarak değiştirmelisiniz.
- Framebuffer tabanlı uygulamalarda konsol logları ve GUI aynı ekrana görüntü yazarlar. Bu nedenle uygulamayı tam görebilmek için konsol devredışı bırakılmalıdır.
- Uygulamaya yeni bir dosya ekliyor veya mevcut bir dosyayı siliyorsanız **CMakeLists.txt** dosyasında derlenecek dosyaları güncellemeyi unutmayın.
- Bu uygulama Raspberry Pi 4 cihazı üzerinde test edilmiştir. Daha düşük donanımlarda test edilmemiştir.
- Bu uygulamada bazı backend işlemleri için C kütüphaneleri kullanılmıştır. Uygulama yalnızca LVGL kütüphanesine bağımlı değildir.