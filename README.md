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

## 🎯 Kontroller
- Joystick yönleri: yukarı, aşağı, sola, sağ
- Menüde smooth scroll desteği
- Butonlar ve toggle anahtarları ile ek fonksiyonlar

## 📜 Lisans
Bu proje **açık kaynak** olarak paylaşılmıştır. İstediğiniz gibi
geliştirebilir ve uyarlayabilirsiniz.