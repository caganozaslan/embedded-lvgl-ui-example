# Yapılacaklar Listesi (`todo.md`)

Bu belge, projenin geliştirme sürecindeki görevleri, bilinen hataları ve gelecek planlarını takip etmek amacıyla hazırlanmıştır.

---

## Son Tamamlanan Gelişmeler

- [x] Tüm ana sayfa butonları ve sayfa geçiş sistemi tamamlandı (`ScreenManager`).
- [x] RS-485 üzerinden sıcaklık, iletkenlik ve basınç verisi başarıyla okundu.
- [x] Simülasyon ortamında (Windows) GUI başarıyla başlatıldı.
- [x] Wi-Fi bağlantısı için ağ tarama, parola girişi ve bağlantı işlevleri eklendi.
- [x] Ayarların `/etc/settings.txt` dosyasına yazılıp okunması başarıyla uygulandı.
- [x] `sensor_recorder.cpp` arka plan thread yapısı geliştirildi.
- [x] Ortalama veriler sayfasında grafikler dinamik olarak oluşturulabiliyor.

---

## Devam Eden Geliştirmeler

- [ ] SSH bağlantısı açma/kapama özelliği geliştiriliyor.
- [ ] Otomatik Wi-Fi bağlantısı kurma özelliği geliştiriliyor.
- [ ] Gece/gündüz modu geliştiriliyor.


---

## Bilinen Hatalar

- [ ] Wi-Fi bağlantısı elle kesildiğinde hızlı bir şekilde Wi-Fi ayarları sayfasına çık-gir yapıldığında Wi-Fi servisi takılıyor.


---

## Planlanan Özellikler

- [ ] Veri paylaşımı için bir sunucu yapılandırılması ve cihazdan sunucuya veri aktarımı sağlanması.
- [ ] Parlaklık ayarının doğrudan GUI'den LCD ekrana uygulanması.
- [ ] Bellek tasarruf özelliği.
- [ ] Güç tasarufu özelliği.
- [ ] Grafikler için yakınlaştırma/pan (zoom/pan) fonksiyonu 
- [ ] Ekran zaman aşımı özelliği (Ekran kararacak, uygulama çalışmaya devam edecek).

---



Bu dosya sürekli olarak güncellenmektedir. Geliştirme sırasında yeni hatalar, fikirler veya görevler buraya eklenecektirgf.
