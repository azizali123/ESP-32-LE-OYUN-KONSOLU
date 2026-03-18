# ESP-32-LE-OYUN-KONSOLU
ESP32 tabanlı Türkçe oyun konsolu 🎮 | NRF24L01, OLED SH1106, WiFi, Bluetooth destekli | Açık kaynak, geliştirilebilir oyunlar ve uygulamalar
ESP32-based Turkish Game Console 🎮 | NRF24L01, OLED SH1106, WiFi, Bluetooth support | Open-source, customizable games and apps

# ESP32 Türkçe Game Console 🎮

ESP32 tabanlı, NRF24L01, OLED (SH1106) ve çeşitli butonlarla geliştirilmiş
çok yönlü bir **Türkçe oyun konsolu** projesi. Hem oyun oynanabilir hem de
notepad, hesap makinesi gibi uygulamalar çalıştırılabilir.

## 🚀 Özellikler
- ESP32 mikrodenetleyici
- NRF24L01 RF modülü
- SH1106 OLED ekran (128x64)
- 4 büyük buton + 2 toggle buton
- Buzzer ile ses desteği
- WiFi, Bluetooth ve RF24 ile iletişim
- Türkçe arayüz ve menü sistemi
- Açık kaynak kod, geliştirilebilir yapı

## 🎮 Oyunlar
- Galaga, Tetris, Pacman
- Snake, Pong, XOX
- 2048, Flappy, TRex, Lander
- Arkanoid, Minesweeper, Maze

## 📱 Uygulamalar
- Notepad (şifre/not saklama)
- Hesap Makinesi (Calc)
- Pomodoro zamanlayıcı
- Kahve Falı, Zar atma

## 🔧 Kurulum
1. Arduino IDE veya PlatformIO kullanın.
2. ESP32 board paketini yükleyin.
3. Gerekli kütüphaneleri ekleyin:
   - `Adafruit_GFX`
   - `Adafruit_SH110X`
   - `BleGamepad`
   - `RF24`
   - `WiFi`
   - `SPI`, `Preferences`, `math`, `time`
4. `ESP32_TURKCE_GAME_CONSOLE.ino` dosyasını açın ve yükleyin.
5. # ESP32 Pin Bağlantı Tablosu

Bu tablo, ESP32 tabanlı Smart Console & Gamepad projesindeki tüm donanım bileşenlerinin GPIO bağlantılarını özetler.

| **Bileşen**              | **Pin Adı (Kod)** | **ESP32 GPIO** | **İşlev / Açıklama**             |
|--------------------------|-------------------|----------------|----------------------------------|
| **OLED SH1106 (SPI)**    | OLED_SCK_PIN      | GPIO 18        | SPI Clock (SCK)                  |
|                          | OLED_MOSI_PIN     | GPIO 23        | SPI MOSI (Data)                  |
|                          | OLED_RES_PIN      | GPIO 17        | Reset                            |
|                          | OLED_DC_PIN       | GPIO 16        | Data/Command seçimi              |
|                          | OLED_CS_PIN       | GPIO 5         | Chip Select                      |
| **NRF24L01 (SPI)**       | NRF_CE_PIN        | GPIO 4         | Chip Enable                      |
|                          | NRF_CSN_PIN       | GPIO 15        | Chip Select                      |
|                          | NRF_MISO_PIN      | GPIO 19        | SPI MISO                         |
|                          | *SCK (ortak SPI)* | GPIO 18        | SPI Clock (OLED ile ortak)       |
| **Joystick – Sol**       | SOL_JOY_X_PIN     | GPIO 34 (ADC)  | X ekseni analog giriş            |
|                          | SOL_JOY_Y_PIN     | GPIO 35 (ADC)  | Y ekseni analog giriş            |
|                          | SOL_JOY_BTN       | GPIO 25        | Buton (dijital giriş)            |
| **Joystick – Sağ**       | SAG_JOY_X_PIN     | GPIO 32 (ADC)  | X ekseni analog giriş            |
|                          | SAG_JOY_Y_PIN     | GPIO 33 (ADC)  | Y ekseni analog giriş            |
|                          | SAG_JOY_BTN       | GPIO 26        | Buton (dijital giriş)            |
| **Renkli Butonlar**      | BTN_KIRMIZI       | GPIO 13        | Kırmızı buton                    |
|                          | BTN_YESIL         | GPIO 22        | Yeşil buton                      |
|                          | BTN_MAVI          | GPIO 14        | Mavi buton                       |
|                          | BTN_SARI          | GPIO 27        | Sarı buton                       |
| **Toggle Switchler**     | TOGGLE1_PIN       | GPIO 36 (ADC)  | Toggle 1                         |
|                          | TOGGLE2_PIN       | GPIO 39 (ADC)  | Toggle 2                         |
| **Ses Sistemi (Buzzer)** | BUZZER_PIN        | GPIO 21        | Piezo buzzer çıkışı              |

> ℹ️ Not: GPIO 34, 35, 36, 39 sadece analog giriş olarak kullanılabilir. Dijital çıkış desteklemez.

6.# 📡 ESP32 WiFi Ayarları Rehberi

Bu projede WiFi bağlantısı için gerekli olan **kullanıcı adı (SSID)** ve **şifre**, doğrudan kod içerisinden tanımlanmaktadır. Aşağıdaki adımları takip ederek kolayca ayarlayabilirsin.

---

## 🔧 WiFi Bilgileri Nereden Değiştirilir?

Kod içerisinde aşağıdaki satırları bulman gerekiyor:

```cpp
char wifiSSID[32] = "MODEM KULLANICI ADINIZI BURAYA YAZIN";
char wifiSifre[64] = "MODEM KULLANICI ŞİFRENİZİ BURAYA YAZIN";
```

---

## ✍️ WiFi Bilgileri Nasıl Girilir?

* `"MODEM KULLANICI ADINIZI BURAYA YAZIN"` kısmını kendi **WiFi adın (SSID)** ile değiştir
* `"MODEM KULLANICI ŞİFRENİZİ BURAYA YAZIN"` kısmını kendi **WiFi şifren** ile değiştir

### ✅ Örnek Kullanım:

```cpp
char wifiSSID[32] = "testWifi";
char wifiSifre[64] = "12345678";
```

---

## ⚠️ Önemli Not

Eğer kod içerisinde WiFi bilgilerini eklerken problem yaşarsan:

* ESP32’ye kodu yükle
* Cihaz açıldıktan sonra menüye gir
* **WiFi Ayarları** bölümünü aç
* **WiFi adı (SSID)** kısmına kullanıcı adını gir
* **Şifre** kısmına WiFi şifreni gir

👉 Bu şekilde de bağlantı sağlayabilirsin.

---

## 💡 İpuçları

* WiFi adı ve şifre **büyük/küçük harfe duyarlıdır**
* ESP32 cihazları genellikle **2.4GHz WiFi** ile daha stabil çalışır
* Şifrede boşluk veya özel karakter kullanıyorsan doğru yazdığına emin ol
* Bağlantı sorunlarında modemine yakın olmayı dene

---

## 🚀 Sonuç

WiFi bilgilerini doğru şekilde girdikten sonra cihaz otomatik olarak ağa bağlanacaktır ve sistemin WiFi modunda sorunsuz çalışacaktır.

---


## 🎯 Kontroller
- Joystick yönleri: yukarı, aşağı, sola, sağ
- Menüde smooth scroll desteği
- Butonlar ve toggle anahtarları ile ek fonksiyonlar

## 📜 Lisans
Bu proje **açık kaynak** olarak paylaşılmıştır. İstediğiniz gibi
geliştirebilir ve uyarlayabilirsiniz.
