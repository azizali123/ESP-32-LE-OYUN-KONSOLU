// ==========================================================================================
//  AZİZ ALİ ESP32 SMART CONSOLE & GAMEPAD VE ANA İŞLETİM SİSTEMİ (Sürüm 5.0 -MEGA)        //
// ==========================================================================================
//                                                                                         //
// Merhaba Ben Aziz Ali Artuç.                                                             //
// Bu proje; 3 modüllü (NRF24L01, ESP32, SH10X OLED ekran),                                //
// 4 adet büyük buton, 2 adet toggle buton ve bir buzzer içeren                            //
// çok yönlü bir sistemdir.                                                                //
//                                                                                         //
// Özellikler:                                                                             //
// - Hem Wi-Fi, hem RF24 radyo, hem de Bluetooth üzerinden iletişim sağlar.                //
// - Sistem kontrolcüsü istenildiğinde oyun kolu gibi kullanılabilir.                      //
// - İçerik ve kullanım dili tamamen Türkçedir.                                            //
// - İçinde nostalji yaratan sürpriz dolu birkaç oyun bulunur.                             //
// - Notepad uygulaması ile kısa not veya şifre saklanabilir.                              //
// - Çok yönlü olup geliştirmeye açıktır.                                                  //
//                                                                                         //
// Kullanılan Kütüphaneler:                                                                //
// Adafruit_GFX.h, Adafruit_SH110X.h, Arduino.h, BleGamepad.h,                             //
// Preferences.h, RF24.h, SPI.h, WiFi.h, WiFiUdp.h, math.h, time.h                         //
//                                                                                         //
// Bu sistem tamamen açık kaynak kodları kullanılarak oluşturulmuştur.                     //
// Geliştirilebilir ve kişiselleştirilebilir.                                              //
//===========================================================================================
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Arduino.h>
#include <BleGamepad.h>
#include <Preferences.h>
#include <RF24.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <math.h>
#include <time.h>

// ===================== PİN TANIMLARI =====================
#define OLED_SCK_PIN 18
#define OLED_MOSI_PIN 23
#define OLED_RES_PIN 17
#define OLED_DC_PIN 16
#define OLED_CS_PIN 5
#define NRF_CE_PIN 4
#define NRF_CSN_PIN 15
#define NRF_MISO_PIN 19
#define SOL_JOY_X_PIN 34
#define SOL_JOY_Y_PIN 35
#define SOL_JOY_BTN 25
#define SAG_JOY_X_PIN 32
#define SAG_JOY_Y_PIN 33
#define SAG_JOY_BTN 26
#define BTN_KIRMIZI 13
#define BTN_YESIL 22
#define BTN_MAVI 14
#define BTN_SARI 27
#define TOGGLE1_PIN 36
#define TOGGLE2_PIN 39
#define BUZZER_PIN 21

// ===================== SABİTLER =====================
#define MOD_WIFI 0
#define MOD_BLUETOOTH 1
#define MOD_NRF_KOPRU 2
#define APP_OYUN 0
#define APP_NOTEPAD 1
#define GONDERIM_MS 20
#define EKRAN_GUNCELLEME 50
#define DEBOUNCE_MS 50
#define ACILIS_MS 3000
#define WIFI_UDP_PORT 4210
#define MAX_NOT_SAYISI 10
#define NOT_MAX_KARAKTER 108

// ===================== EMA FİLTRE PARAMETRELERİ =====================
#define ALPHA_MOVE 0.20f
#define ALPHA_CAM 0.30f
#define DEADZONE 0.04f

// ===================== ENUM =====================
enum EkranSayfasi {
  EKRAN_ACILIS = 0,
  EKRAN_ANA_MENU,
  EKRAN_OYUN_MODU,
  EKRAN_NOTEPAD_LISTE,
  EKRAN_NOTEPAD_DUZENLE,
  EKRAN_AYARLAR,
  EKRAN_BAGLANTI_AYAR,
  EKRAN_NRF_AYAR,
  EKRAN_HAKKINDA,
  EKRAN_TARIH_AYAR,
  EKRAN_SAAT_AYAR,
  EKRAN_SISTEM_GUNCELLEME,
  EKRAN_HASSASIYET_AYAR,
  EKRAN_WIFI_AYAR,
  // Oyun & Uygulama
  EKRAN_GAME_GALAGA,
  EKRAN_GAME_LANDER,
  EKRAN_GAME_FLAPPY,
  EKRAN_GAME_TREX,
  EKRAN_GAME_ARKANOID,
  EKRAN_GAME_2048,
  EKRAN_GAME_MAZE,
  EKRAN_GAME_PACMAN,
  EKRAN_GAME_SNAKE,
  EKRAN_GAME_PONG,
  EKRAN_GAME_XOX,
  EKRAN_GAME_MINESWEEPER,
  EKRAN_GAME_TETRIS,
  EKRAN_APP_FAL,
  EKRAN_APP_POMODORO,
  EKRAN_GAME_DICE,
  EKRAN_APP_CALC,
  EKRAN_SES_AYAR,
  EKRAN_OYUNLAR_MENU,
  EKRAN_UYG_MENU,
  EKRAN_WIFI_TARA,
  EKRAN_WIFI_TARA_LISTE,
  EKRAN_SNAKE_SECIM,
  EKRAN_PONG_SECIM
};

struct Not {
  char baslik[20];
  char icerik[NOT_MAX_KARAKTER];
};
struct VeriPaketi {
  int16_t solJoyX;
  int16_t solJoyY;
  bool solJoyButon;
  int16_t sagJoyX;
  int16_t sagJoyY;
  bool sagJoyButon;
  bool buton1;
  bool buton2;
  bool buton3;
  bool buton4;
  bool toggle1;
  bool toggle2;
  uint8_t baglantiModu;
  uint8_t aktiveMod;
  uint8_t hassasiyet;
};

// ===================== GLOBAL NESNELER =====================
Adafruit_SH1106G ekran(128, 64, &SPI, OLED_DC_PIN, OLED_RES_PIN, OLED_CS_PIN);
BleGamepad bleGamepad;
BleGamepadConfiguration bleAyarlari;
WiFiUDP udp;
RF24 nrfRadyo(NRF_CE_PIN, NRF_CSN_PIN);
Preferences tercihler;
SemaphoreHandle_t spiMutex;
TaskHandle_t girisGorevilHandle = NULL;
TaskHandle_t ekranGorevilHandle = NULL;

// ===================== SİSTEM DEĞİŞKENLERİ =====================
uint8_t baglantiModu = MOD_BLUETOOTH;
uint8_t aktiveMod = APP_OYUN;
EkranSayfasi aktifEkran = EKRAN_ACILIS;
int16_t solJoyMerkezX = 2048, solJoyMerkezY = 2048, sagJoyMerkezX = 2048,
        sagJoyMerkezY = 2048;

// Optimize EMA filtre deÄŸerleri (dual-alpha)
float emSolX = 0.5f, emSolY = 0.5f, emSagX = 0.5f, emSagY = 0.5f;
float globalHassasiyet = 1.6f;

char wifiSSID[32] = "SUPERONLINE_Wi-Fi_DE0F";
char wifiSifre[64] = "GmKbvaqs5D";
char pcIP[20] = "192.168.1.7";
bool wifiBagliMi = false, nrfAktif = false, bleBasladiMi = false;
uint8_t nrfKanal = 75;
uint8_t saatSaat = 5, saatDakika = 31, saatSaniye = 0, tarihGun = 13,
        tarihAy = 3;
uint16_t tarihYil = 2026;
unsigned long sonSaatGuncelleme = 0;
uint8_t buzzerVolume = 5; // 0-10 arasi varsayilan 5
int bulunanAgSayisi = 0;
String agIsimleri[15];
int wifiTaramaSecim = 0;

unsigned long btnZamani[16] = {0};
bool btnOnceki[16] = {false};
VeriPaketi guncelPaket;
Not notlar[MAX_NOT_SAYISI];
int aktifNotIndeks = 0, duzenlemeImlecu = 0, karSecimiX = 0, karSecimiY = 0;
int aktifKarSeti = 0, anaMenuSecim = 0, ayarlarSecim = 0, baglantiSecim = 0,
    notListeSecim = 0, gameMenuSecim = 0, uygMenuSecim = 0;
int hakkindaScrollY = 0, tarihSeciliAlan = 0, saatSeciliAlan = 0;
uint8_t sinyalKare = 0;
unsigned long sinyalAnimZaman = 0;
char duzenlemeBuf[NOT_MAX_KARAKTER + 1];
// WiFi ayar paneli
int wifiAyarSecim = 0; // 0=SSID, 1=Sifre, 2=IP
bool wifiDuzenleModu = false;
int wifiDuzenleHedef = 0;
char wifiDuzenlemeBuf[64];

// ===================== SES SİSTEMİ =====================
void bipTek() { tone(BUZZER_PIN, 1200, 80); }
void bipCift() {
  tone(BUZZER_PIN, 800, 80);
  delay(120);
  tone(BUZZER_PIN, 800, 80);
}
void bipUclu() {
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 600, 60);
    delay(100);
  }
}
void bipBasari() {
  tone(BUZZER_PIN, 1000, 60);
  delay(80);
  tone(BUZZER_PIN, 1400, 100);
}
void bipHata() {
  tone(BUZZER_PIN, 400, 150);
  delay(180);
  tone(BUZZER_PIN, 300, 200);
}
void bipMenuSec() { tone(BUZZER_PIN, 1800, 30); }
void bipOyunBitti() {                                                                       
  tone(BUZZER_PIN, 800, 100);
  delay(120);
  tone(BUZZER_PIN, 600, 100);
  delay(120);
  tone(BUZZER_PIN, 400, 200);
}
void bipSeviyeAtla() {
  tone(BUZZER_PIN, 800, 60);
  delay(80);
  tone(BUZZER_PIN, 1200, 60);
  delay(80);
  tone(BUZZER_PIN, 1600, 100);
}
void bipCanKaybi() {
  tone(BUZZER_PIN, 500, 80);
  delay(100);
  tone(BUZZER_PIN, 350, 120);
}
// Galaga sesleri
void bipAtes() { tone(BUZZER_PIN, 2500, 20); }
void bipPatlama() {
  tone(BUZZER_PIN, 200, 40);
  delay(50);
  tone(BUZZER_PIN, 100, 60);
}
// Lander sesleri
void bipItki() { tone(BUZZER_PIN, 150 + random(0, 50), 15); }
void bipInis() {
  tone(BUZZER_PIN, 600, 50);
  delay(60);
  tone(BUZZER_PIN, 900, 50);
  delay(60);
  tone(BUZZER_PIN, 1200, 80);
}
// Zar casino sesleri
void bipZarTik(int hiz) {
  tone(BUZZER_PIN, 1000 + random(0, 500), max(10, 30 - hiz));                                                                                   
}
void bipZarSon() {
  tone(BUZZER_PIN, 800, 60);
  delay(70);
  tone(BUZZER_PIN, 1200, 60);
  delay(70);
  tone(BUZZER_PIN, 1600, 120);
}
// T-Rex
void bipZipla() { tone(BUZZER_PIN, 1500, 30); }
// Snake yem
void bipYem() {
  tone(BUZZER_PIN, 1800, 25);
  delay(30);
  tone(BUZZER_PIN, 2200, 35);
}

// ===================== OYUN DEĞİŞKENLERİ (Optimize) =====================
// Ortak oyun durumu
uint8_t oyunCan = 5;
uint8_t oyunSeviye = 1;
bool oyunAktif = false, oyunBitti = false;

// Snake
int snk_x[100], snk_y[100], snk_length, snk_dir, snk_foodX, snk_foodY,
    snk_score;
bool snk_active = false, snk_gameOver = false;
unsigned long snk_lastMoveTime = 0;
uint8_t snk_mode = 0; // 0=kolay(duvar geçiş), 1=zor(Nokia3310)
uint8_t snk_can = 5, snk_speed = 100;

// Pong
int png_pL_Y, png_pR_Y, png_bX, png_bY, png_bDX, png_bDY, png_sL, png_sR;
bool png_active = false, png_gameOver = false;
unsigned long png_lastMoveTime = 0;
uint8_t png_mode = 0; // 0=insan-AI, 1=insan-insan
uint8_t png_difficulty = 0;

// XOX
int xox_board[3][3], xox_curX, xox_curY, xox_turn, xox_win;
bool xox_active = false;
uint8_t xox_seviye = 1, xox_skor_x = 0, xox_skor_o = 0;

// Minesweeper - 12x6 grid (100x50px alan)
#define MINE_COLS 12
#define MINE_ROWS 6
int mine_grid[MINE_ROWS][MINE_COLS], mine_cx, mine_cy;
bool mine_rev[MINE_ROWS][MINE_COLS], mine_flag[MINE_ROWS][MINE_COLS];
bool mine_act = false, mine_go = false;

// Tetris
int tet_board[15][10], tet_x, tet_y, tet_score, tet_type, tet_rot, tet_next;
unsigned long tet_lastDrop;
bool tet_act = false, tet_go = false;
const uint16_t tet_shapes[7][4] = {
    {0x0F00, 0x2222, 0x00F0, 0x4444}, {0x44C0, 0x8E00, 0x6440, 0x0E20},
    {0x4460, 0x0E80, 0xC440, 0x2E00}, {0xCC00, 0xCC00, 0xCC00, 0xCC00},
    {0x06C0, 0x8C40, 0x6C00, 0x4620}, {0x0E40, 0x4C40, 0x4E00, 0x4640},
    {0x0C60, 0x4C80, 0xC600, 0x2640}};

// Dice
int dice_val1 = 6, dice_val2 = 6;
bool dice_rolling = false;
unsigned long dice_lastFrameTime = 0;
int dice_roll_count = 0;

// T-Rex
int rex_y = 40, rex_vy = 0, rex_obsX = 128, rex_obsW = 8, rex_score = 0;
bool rex_act = false, rex_go = false;
uint8_t rex_can = 5, rex_speed = 2;

// Calculator
int calc_cx = 0, calc_cy = 0;
float calc_num1 = 0, calc_num2 = 0; 
char calc_op = ' ';
String calc_disp = "";
int calc_karSet = 0; // 0=numpad, 1=bilimsel
char calc_keys[4][4] = {{'7', '8', '9', '/'},
                        {'4', '5', '6', '*'},
                        {'1', '2', '3', '-'},
                        {'C', '0', '=', '+'}};
const char calc_sci_keys[4][4] = {{'s', 'c', 't', 'l'},
                                  {'S', 'C', 'T', 'L'},
                                  {'P', 'e', '!', '^'},
                                  {'(', ')', '.', 'R'}};
// s=sin c=cos t=tan l=log S=asin C=acos T=atan L=ln P=pi e=euler !=fact ^=pow
// R=sqrt

// Arkanoid
int ark_px, ark_bx, ark_by, ark_bvx, ark_bvy, ark_score;
bool ark_act = false, ark_go = false;
int ark_br[3][8];
uint8_t ark_can = 5, ark_level = 1;

// Fal
bool fal_act = false, fal_reading = false;
unsigned long fal_time = 0;
int fal_msg = -1;

// Pomodoro
bool pom_act = false, pom_running = false;
unsigned long pom_startMillis = 0, pom_elapsed = 0;

// 2048
int g2048_board[4][4], g2048_score = 0;
bool g2048_act = false, g2048_go = false;

// Maze - Prosedural 100 seviye
int maze_px, maze_py, maze_level = 0;
bool maze_act = false, maze_win = false;
uint8_t maze_can = 3;
unsigned long maze_startTime = 0;
uint8_t maze_grid[8][21]; // dinamik uretilen labirent
void mazeGenerate(int level) {
  // Duvarlarla doldur
  for (int r = 0; r < 8; r++)
    for (int c = 0; c < 21; c++)
      maze_grid[r][c] = 1;
  // Seed-based random ile yol ac
  randomSeed(level * 137 + 42);
  // Recursive backtracker benzeri basit algoritma
  // Satir 1,3,5 ve sutun 0-20 arasi yollar ac
  for (int r = 1; r < 7; r += 2) {
    maze_grid[r][0] = 0; // giris
    int c = 0;
    while (c < 20) {
      maze_grid[r][c] = 0;
      int adim = 1 + random(0, 3);
      for (int s = 0; s < adim && c < 20; s++) {
        c++;
        maze_grid[r][c] = 0;
      }
      // Dikey baglanti
      if (r < 6 && random(0, 3) > 0) {
        maze_grid[r + 1][c] = 0;
      }
      if (r > 1 && random(0, 3) > 0) {
        maze_grid[r - 1][c] = 0;
      }
    }
    maze_grid[r][20] = 0;
  }
  // Ek dikey baglantilar
  for (int c = 2; c < 20; c += 3) {
    int r = 2 + random(0, 4) * 2;
    if (r < 7)
      maze_grid[r][c] = 0;
  }
  // Giris ve cikis garantisi
  maze_grid[1][0] = 0;
  maze_grid[6][20] = 0;
  maze_grid[1][1] = 0;
  maze_grid[6][19] = 0;
  // Seviye arttikca daha fazla duvar ekle
  int extraWalls = min(level / 5, 15);
  for (int i = 0; i < extraWalls; i++) {
    int wr = 1 + random(0, 6), wc = 1 + random(0, 19);
    if (maze_grid[wr][wc] == 0) {
      // Yolu tamamen kapatma kontrolu - basit
      int komsu = 0;
      if (wr > 0 && maze_grid[wr - 1][wc] == 0)
        komsu++;
      if (wr < 7 && maze_grid[wr + 1][wc] == 0)
        komsu++;
      if (wc > 0 && maze_grid[wr][wc - 1] == 0)
        komsu++;
      if (wc < 20 && maze_grid[wr][wc + 1] == 0)
        komsu++;
      if (komsu >= 3)
        maze_grid[wr][wc] = 1; // sadece 3+ komsulu acik hucrelere duvar koy
    }
  }
  // Kenar duvarlari
  for (int c = 0; c < 21; c++) {
    maze_grid[0][c] = 1;
    maze_grid[7][c] = 1;
  }
  randomSeed(analogRead(34)); // seed'i geri yukle
}

// Pacman
byte pac_map[8][21];
int pac_x, pac_y, pac_dir, pac_nextDir;
int gh_x, gh_y, gh_dir, pac_score, pac_dots;
bool pac_act = false, pac_go = false, pac_win = false;
unsigned long pac_lastMove = 0;
uint8_t pac_can = 5;
const uint8_t pac_layout[8][21] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1},
    {1, 2, 1, 1, 2, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 2, 1, 1, 2, 1},
    {1, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 1},
    {1, 2, 2, 2, 2, 1, 1, 2, 1, 0, 0, 1, 2, 1, 1, 2, 2, 2, 2, 2, 1},
    {1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 2, 1},
    {1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};

// Galaga
int gal_px, gal_bx, gal_by, gal_score;
bool gal_act = false, gal_fire = false, gal_go = false;
int gal_ex[5], gal_ey[5];
bool gal_eAlive[5];
int gal_edir = 1, gal_wave = 1;
uint8_t gal_can = 5;
struct Star {
  float x, y, speed;
};
Star gal_stars[15];
const uint8_t PROGMEM gal_player_sprite[] = {0x18, 0x3C, 0x3C, 0x7E,
                                             0x7E, 0xFF, 0xFF, 0x66};
const uint8_t PROGMEM gal_enemy1_sprite[] = {0x66, 0xFF, 0xFF, 0xDB,
                                             0xDB, 0xFF, 0x7E, 0x3C};

// Lander
float lnd_x, lnd_y, lnd_vy, lnd_vx, lnd_fuel;
bool lnd_act = false, lnd_go = false, lnd_win = false, lnd_isThrusting = false;
struct Particle {
  float x, y, vx, vy;
  int life;
};
Particle lnd_particles[10];
const uint8_t PROGMEM lnd_lander_sprite[] = {
    0x18, 0x00, 0x3C, 0x00, 0x3C, 0x00, 0x7E, 0x00, 0xFF, 0xC0,
    0xFF, 0xC0, 0x66, 0x00, 0x66, 0x00, 0xFF, 0xC0, 0x81, 0x00};

// Flappy
float flp_y, flp_vy;
int flp_px, flp_gapY, flp_score;
bool flp_act = false, flp_go = false;
uint8_t flp_can = 5;
struct FlpParticle {
  float x, y, vx, vy;
  int life;
};
FlpParticle flp_particles[5];
const uint8_t PROGMEM flp_ufo_sprite[] = {0x07, 0xE0, 0x08, 0x10, 0x1F, 0xF8,
                                          0x3F, 0xFC, 0x7E, 0x7E, 0xFF, 0xFF,
                                          0x3F, 0xFC, 0x0E, 0x70};

// ===================== KAHVE FALI VERİLERİ (PROGMEM) =====================
const char *const fal_subjects[] PROGMEM = {
    "Bir kedi",     "Bir kopek",      "Bir yilan",  "Bir balik",   "Bir kus",
    "Bir guvercin", "Bir kartal",     "Bir baykus", "Bir kaplan",  "Bir aslan",
    "Bir fil",      "Bir kaplumbaga", "Bir tavsan", "Bir kelebek", "Bir ari",
    "Bir orumcek",  "Bir kurt",       "Bir tilki",  "Bir ayi",     "Bir geyik",
    "Bir at",       "Bir esek",       "Bir deve",   "Bir horoz",   "Bir tavuk",
    "Bir anahtar",  "Bir kilit",      "Bir kapi",   "Bir pencere", "Bir kopru",
    "Bir ev",       "Bir araba",      "Bir gemi",   "Bir ucak",    "Bir mektup",
    "Bir saat",     "Bir yuzuk",      "Bir kolye",  "Bir tac",     "Bir kilic",
    "Bir fener",    "Bir mum",        "Bir kitap",  "Bir kalem",   "Bir para",
    "Bir semsiye",  "Bir ayna",       "Bir vazo",   "Bir bardak",  "Bir fincan",
    "Bir bayrak",   "Bir balon",      "Bir top",    "Bir yildiz",  "Bir gunes",
    "Bir ay",       "Bir bulut",      "Bir yaprak", "Bir tas",     "Bir dag",
    "Bir agac",     "Bir gul",        "Bir deniz",  "Bir nehir",   "Bir gol",
    "Bir kadin",    "Bir erkek",      "Bir cocuk",  "Bir aile",    "Bir melek"};
#define FAL_SUBJECT_COUNT 70

const char *const fal_positives[] PROGMEM = {
    "guzel bir haber getirecek",      "kismet kapilarini aciyor",
    "yeni bir baslangicin habercisi", "kalbini ferahlatacak",
    "sansinin acildigini soyluyor",   "huzurlu bir donem basliyor",
    "guzel bir yolculuk fisilidyor",  "bereketli gunler yaklastiriyor",
    "ask enerjini guclendiriyor",     "icsel bir ferahlik yaklasti",
    "dileklerin gerceklesecek",       "yeni firsat kapisi acildi",
    "guclu bir destek alacaksin",     "yolun acildi engeller kalkti",
    "guzel bir surpriz bekliyor",     "sicak enerji geliyor",
    "bekledigin haber cok yakin",     "mutlu donem basliyor",
    "ruhsal olarak gucleniyorsun",    "kalbin istedigini bulacak"};
#define FAL_POSITIVE_COUNT 20

const char *const fal_negatives[] PROGMEM = {
    "dikkat etmen gereken biri var", "kucuk bir sikinti olabilir",
    "kiskanc bir goz seni izliyor",  "kararsizlik enerjisi tasiyor",
    "ufak bir engel cikabilir",      "gizli bir niyet var",
    "sabirli olman gereken sure",    "yanlis anlasilma olabilir",
    "enerjin biraz dusuk",           "acele etmemelisin",
    "bir firsati kacirabilirsin",    "hayal kirikligi olabilir",
    "samimi olmayan biri var",       "planin ertelenebilir",
    "beklenmedik engel cikabilir",   "dikkatli adim at",
    "kararsiz kalabilirsin",         "mesafeli biri var",
    "gercekleri tam goremiyorsun",   "plan yavas ilerleyebilir"};
#define FAL_NEGATIVE_COUNT 20

const char *const fal_connectors[] PROGMEM = {
    "ama sonunda yoluna girecek",      "sabirli ol",
    "ic sesin dogru yolu gosterecek",  "bu surec guclendiriyor",
    "bekledigin aydinlik yakin",       "kalbin dogru yolu biliyor",
    "sonunda yuzun gulecek",           "dogru zamanda netlesecek",
    "sabir ile istedigin olacak",      "evren guzel plan hazirliyor",
    "kalbin sesi yaniltmayacak",       "haber dusundugunden yakin",
    "her seyin sebebini anlayacaksin", "icindeki guc ayakta tutuyor",
    "dogru zaman gelince hizlanacak",  "evren seni koruyor",
    "sabirla odulunu alacaksin",       "ic huzurunu bulacaksin",
    "taslar yerine oturacak",          "dogru insanlar an meselesi"};
#define FAL_CONNECTOR_COUNT 20

// ===================== MENU VERİLERİ (KATEGORİLERİ) =====================
#define TOTAL_MENU_ITEMS 6
const char *anaMenuOgeleri[TOTAL_MENU_ITEMS] = {
    "Konsol Test Mode", "Not Defteri", "Sistem Ayarlari",
    "Oyunlar", "Uygulamalar", "Hakkinda"};

#define TOTAL_GAME_ITEMS 13
const char *oyunMenuOgeleri[TOTAL_GAME_ITEMS] = {
    "Galaga V2", "Ay'a Inis", "Flappy UFO", "T-Rex", "Tugla Kir", 
    "2048", "Labirent", "Pacman", "Yilan", "Pong", "XOX", "Mayin Tarlasi", "Tetris"};

#define TOTAL_APP_ITEMS 4
const char *uygMenuOgeleri[TOTAL_APP_ITEMS] = {
    "Kahve Fali", "Kronometre", "Zar At", "Hesap Makinesi"};

const char *ayarOgeleri[] = {"Tarih Ayari", "Saat Ayari", "Baglanti Ayari",
                             "NRF Kopru Ayari", "Guncelleme", "Hassasiyet",
                             "WiFi Giris", "Ses Ayari"};
const char *bagModAdi[] = {"WiFi", "Bluetooth", "NRF Kopru"};
const char klavyeSetleri[4][27] = {
    "qwertyuiopasdfghjklzxcvbnm", "QWERTYUIOPASDFGHJKLZXCVBNM",
    "0123456789@#/*+-=()[]{}_$&", "cgiosuCGIOSU<>\\|~^`'\"     "};
const char *klavyeAdlari[] = {"[abc]", "[ABC]", "[123]", "[TR] "};
const int HAKKINDA_SATIR_SAYISI = 22;
const char hakkindaMetni[HAKKINDA_SATIR_SAYISI][19] PROGMEM = {
    "Bu konsol Aziz Ali",
    "Artuc tarafindan",
    "ESP32 NRF24 OLED",
    "4 buyuk buton",
    "2 toggle buzzer",
    "WiFi BT RF24",
    "cok yonlu iletisim",
    "Oyun kolu gibi",
    "Turkce arayuz",
    "Notepad pswr sakla",
    "Acik kaynak kod",
    "Gelistirilebilir",
    "Oyunlar Galaga",
    "Pacman Snake Pong",
    "2048 Flappy TRex",
    "Arkanoid Mineswp",
    "Maze oyunu var",
    "Uyg Notepad Calc",
    "Pomodoro Kahve",
    "Kutuphane Adafruit",
    "BleGamepad WiFi",
    "SPI Pref math"
};

// ===================== FONKSİYON PROTOTİPLERİ =====================
void girisBakim(void *param);
void ekranBakim(void *param);
int16_t joyOrt(int pin);
bool btnKenarBul(int butonPini, int indeks);
void joyOku();
void butonlariOku();
void togglelariOku();
void veriPaketiDoldur();
void oyunModuGonder();
void notepadModuIsle();
void wifiBaslat();
void wifiGonder();
void bleGamepadBaslat();
void nrfBaslat();
void nrfGonder(const VeriPaketi &paket);
void saatiGuncelle();
void notlariYukle();
void notuKaydet(int indeks);
void ayarlariKaydet();
void joyKalibrasyon();
void modGecisIslemi(int yeniMod);
bool sagTetikGeri();
bool sagTetikIleri();
bool solTetikSola();
bool solTetikSaga();
bool solTetikYukari();
bool solTetikAsagi();
bool menuGeriBasti();
bool menuIleriBasti();
void drawGameUI(String title, String scoreStr);
void popupGoster(const char *baslik, const char *mesaj);
bool popupYenidenBaslat();
void canCiz(uint8_t can, uint8_t     maxCan);
String falCumleUret();
int minimaxXOX(int b[3][3], int depth, bool isMax);
int enIyiHamleXOX(int b[3][3]);

// Ekran fonksiyonlari
void ekranCiz();
void acilisEkrani();
void durumCubugu(const char *baslik = nullptr);
void altCubukGeri();
void anaMenuCiz();
void oyunModuCiz();
void notepadListeCiz();
void notepadDuzenlemeCiz();
void ayarlarCiz();
void baglantiAyarCiz();
void nrfAyarCiz();
void hakkindaCiz();
void tarihAyarCiz();
void saatAyarCiz();
void guncellemeAyarCiz();
void hassasiyetAyarCiz();
void wifiAyarCiz();
void runGalaga();
void runLander();
void runFlappy();
void runTRexGame();
void runArkanoid();
void run2048();
void runMaze();
void runPacman();
void runSnakeGame();
void runPongGame();
void runXOXGame();
void runMinesweeperGame();
void runTetrisGame();
void runDiceGame();
void runCalculatorApp();
void runFal();
void runPomodoro();
void snakeSecimCiz();
void pongSecimCiz();
// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(28));
  spiMutex = xSemaphoreCreateMutex();
  configASSERT(spiMutex != NULL);
  pinMode(SOL_JOY_BTN, INPUT_PULLUP);
  pinMode(SAG_JOY_BTN, INPUT_PULLUP);
  pinMode(BTN_KIRMIZI, INPUT);
  pinMode(BTN_YESIL, INPUT);
  pinMode(BTN_MAVI, INPUT);
  pinMode(BTN_SARI, INPUT);
  pinMode(TOGGLE1_PIN, INPUT);
  pinMode(TOGGLE2_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(NRF_CSN_PIN, OUTPUT);
  digitalWrite(NRF_CSN_PIN, HIGH);
  pinMode(OLED_CS_PIN, OUTPUT);
  digitalWrite(OLED_CS_PIN, HIGH);
  SPI.begin(OLED_SCK_PIN, NRF_MISO_PIN, OLED_MOSI_PIN, -1);
  if (ekran.begin(0, true)) {
    ekran.setContrast(200);
    ekran.clearDisplay();
    ekran.display();
  }
  tercihler.begin("azizali", true);
  baglantiModu = tercihler.getUChar("bagMod", MOD_BLUETOOTH);
  globalHassasiyet = tercihler.getFloat("hasiy", 1.6f);
  if (digitalRead(BTN_KIRMIZI) == LOW) {
    baglantiModu = MOD_BLUETOOTH;
    tercihler.putUChar("bagMod", MOD_BLUETOOTH);
  }
  saatSaat = tercihler.getUChar("saat", 5);
  saatDakika = tercihler.getUChar("dakika", 31);
  tarihGun = tercihler.getUChar("gun", 13);
  tarihAy = tercihler.getUChar("ay", 3);
  tarihYil = tercihler.getUShort("yil", 2026);
  solJoyMerkezX = tercihler.getShort("solMX", 2048);
  solJoyMerkezY = tercihler.getShort("solMY", 2048);
  sagJoyMerkezX = tercihler.getShort("sagMX", 2048);
  sagJoyMerkezY = tercihler.getShort("sagMY", 2048);
  tercihler.getString("ssid", wifiSSID, 32);
  tercihler.getString("sifre", wifiSifre, 64);
  tercihler.getString("pcip", pcIP, 20);
  buzzerVolume = tercihler.getUChar("vol", 5);
  tercihler.end();
  notlariYukle();
  joyKalibrasyon();
  acilisEkrani();
  ekran.clearDisplay();
  ekran.drawRoundRect(4, 15, 120, 34, 4, SH110X_WHITE);
  ekran.fillRect(4, 15, 120, 12, SH110X_WHITE);
  ekran.setTextColor(SH110X_BLACK);
  ekran.setCursor(18, 17);
  ekran.print("SISTEM BASLATMA");
  ekran.setTextColor(SH110X_WHITE);
  ekran.setCursor(6, 33);
  ekran.print("Donanim Hazirlaniyor");
  ekran.display();
  delay(2000);
  bipBasari();
  ekran.clearDisplay();
  ekran.drawRoundRect(4, 15, 120, 34, 4, SH110X_WHITE);
  ekran.fillRect(4, 15, 120, 12, SH110X_WHITE);
  ekran.setTextColor(SH110X_BLACK);
  ekran.setCursor(24, 17);
  ekran.print("AG BAGLANTISI");
  ekran.setTextColor(SH110X_WHITE);
  switch (baglantiModu) {
  case MOD_BLUETOOTH:
    ekran.setCursor(12, 33);
    ekran.print("BT Baslatiliyor");
    ekran.display();
    bleGamepadBaslat();
    bleBasladiMi = true;
    bipTek();
    break;
  case MOD_WIFI:
    ekran.setCursor(10, 33);
    ekran.print("WiFi Baslatiliyor");
    ekran.display();
    wifiBaslat();
    break;
  case MOD_NRF_KOPRU:
    ekran.setCursor(10, 33);
    ekran.print("NRF Baslatiliyor");
    ekran.display();
    nrfBaslat();
    break;
  }
  delay(1000);
  aktifEkran = EKRAN_ANA_MENU;
  sonSaatGuncelleme = millis();
  xTaskCreatePinnedToCore(girisBakim, "GirisGorevi", 8192, NULL, 2,
                          &girisGorevilHandle, 1);
  xTaskCreatePinnedToCore(ekranBakim, "EkranGorevi", 8192, NULL, 1,
                          &ekranGorevilHandle, 0);
}

void loop() { vTaskDelay(portMAX_DELAY); }

// ===================== RTOS GÖREVLERİ =====================
void girisBakim(void *param) {
  TickType_t sonUyanma = xTaskGetTickCount();
  for (;;) {
    joyOku();
    butonlariOku();
    togglelariOku();
    veriPaketiDoldur();
    if (aktiveMod == APP_OYUN)
      oyunModuGonder();
    
    // Klavye islemleri (Notepad ve WiFi Duzenleme ekranlarinda aktif)
    if (aktifEkran == EKRAN_NOTEPAD_DUZENLE || (aktifEkran == EKRAN_WIFI_AYAR && wifiDuzenleModu))
      notepadModuIsle();
    if (baglantiModu == MOD_WIFI && wifiBagliMi)
      wifiGonder();
    if (baglantiModu == MOD_NRF_KOPRU && nrfAktif)
      nrfGonder(guncelPaket);
    vTaskDelayUntil(&sonUyanma, pdMS_TO_TICKS(GONDERIM_MS));
  }
}

void ekranBakim(void *param) {
  TickType_t sonUyanma = xTaskGetTickCount();
  for (;;) {
    saatiGuncelle();
    if (millis() - sinyalAnimZaman > 400) {
      sinyalKare = (sinyalKare + 1) % 4;
      sinyalAnimZaman = millis();
    }
    if (xSemaphoreTake(spiMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
      ekranCiz();
      xSemaphoreGive(spiMutex);
    }
    if (baglantiModu == MOD_WIFI) {
      if (WiFi.status() != WL_CONNECTED) {
        wifiBagliMi = false;
        WiFi.reconnect();
      } else
        wifiBagliMi = true;
    }
    vTaskDelayUntil(&sonUyanma, pdMS_TO_TICKS(EKRAN_GUNCELLEME));
  }
}

// ===================== OPTİMİZE EMA KONTROL & SENSÖR =====================
int16_t joyOrt(int pin) {
  int32_t top = 0;
  for (int i = 0; i < 16; i++) {
    top += analogRead(pin);
    delayMicroseconds(10);
  }
  return top >> 4; // /16 yerine bit shift
}

void joyKalibrasyon() {
  long tSolX = 0, tSolY = 0, tSagX = 0, tSagY = 0;
  for (int i = 0; i < 32; i++) {
    tSolX += analogRead(SOL_JOY_X_PIN);
    tSolY += analogRead(SOL_JOY_Y_PIN);
    tSagX += analogRead(SAG_JOY_X_PIN);
    tSagY += analogRead(SAG_JOY_Y_PIN);
    delay(5);
  }
  solJoyMerkezX = tSolX >> 5;
  solJoyMerkezY = tSolY >> 5;
  sagJoyMerkezX = tSagX >> 5;
  sagJoyMerkezY = tSagY >> 5;
  emSolX = solJoyMerkezX / 4095.0f;
  emSolY = solJoyMerkezY / 4095.0f;
  emSagX = sagJoyMerkezX / 4095.0f;
  emSagY = sagJoyMerkezY / 4095.0f;
}

void joyOku() {
  float rawSolX = joyOrt(SOL_JOY_X_PIN) / 4095.0f;
  float rawSolY = joyOrt(SOL_JOY_Y_PIN) / 4095.0f;
  float rawSagX = joyOrt(SAG_JOY_X_PIN) / 4095.0f;
  float rawSagY = joyOrt(SAG_JOY_Y_PIN) / 4095.0f;
  // EMA dual-alpha filtre
  emSolX = ALPHA_MOVE * rawSolX + (1.0f - ALPHA_MOVE) * emSolX;
  emSolY = ALPHA_MOVE * rawSolY + (1.0f - ALPHA_MOVE) * emSolY;
  emSagX = ALPHA_CAM * rawSagX + (1.0f - ALPHA_CAM) * emSagX;
  emSagY = ALPHA_CAM * rawSagY + (1.0f - ALPHA_CAM) * emSagY;
  // Deadzone
  float dSolX = emSolX, dSolY = emSolY, dSagX = emSagX, dSagY = emSagY;
  float cX = solJoyMerkezX / 4095.0f, cY = solJoyMerkezY / 4095.0f;
  float cRX = sagJoyMerkezX / 4095.0f, cRY = sagJoyMerkezY / 4095.0f;
  if (abs(dSolX - cX) < DEADZONE)
    dSolX = cX;
  if (abs(dSolY - cY) < DEADZONE)
    dSolY = cY;
  if (abs(dSagX - cRX) < DEADZONE)
    dSagX = cRX;
  if (abs(dSagY - cRY) < DEADZONE)
    dSagY = cRY;
  guncelPaket.solJoyX = (int16_t)(dSolX * 4095.0f);
  guncelPaket.solJoyY = (int16_t)(dSolY * 4095.0f);
  guncelPaket.sagJoyX = (int16_t)(dSagX * 4095.0f);
  guncelPaket.sagJoyY = (int16_t)(dSagY * 4095.0f);
  guncelPaket.solJoyButon = !digitalRead(SOL_JOY_BTN);
  guncelPaket.sagJoyButon = !digitalRead(SAG_JOY_BTN);
}

void butonlariOku() {
  guncelPaket.buton1 = !digitalRead(BTN_KIRMIZI);
  guncelPaket.buton2 = !digitalRead(BTN_YESIL) || !digitalRead(SAG_JOY_BTN);
  guncelPaket.buton3 = !digitalRead(BTN_MAVI);
  guncelPaket.buton4 = !digitalRead(BTN_SARI);
}

void togglelariOku() {
  guncelPaket.toggle1 = !digitalRead(TOGGLE1_PIN);
  guncelPaket.toggle2 = !digitalRead(TOGGLE2_PIN);
}

void veriPaketiDoldur() {
  guncelPaket.baglantiModu = baglantiModu;
  guncelPaket.aktiveMod = aktiveMod;
  guncelPaket.hassasiyet = 1;
}

bool btnKenarBul(int pin, int indeks) {
  bool g = !digitalRead(pin);
  if (pin == BTN_YESIL)
    g = (!digitalRead(BTN_YESIL) || !digitalRead(SAG_JOY_BTN));
  if (g && !btnOnceki[indeks] && (millis() - btnZamani[indeks] > DEBOUNCE_MS)) {
    btnZamani[indeks] = millis();
    btnOnceki[indeks] = true;
    return true;
  }
  if (!g)
    btnOnceki[indeks] = false;
  return false;
}

// ===================== KONTROL ŞEMASI =====================
// Joystick yönleri ve butonlar
bool solTetikSola() { 
  return (guncelPaket.sagJoyX < 1000); 
}
bool solTetikSaga() { 
  return (guncelPaket.sagJoyX > 3000); 
}
bool solTetikYukari() { 
  return (guncelPaket.sagJoyY < 1000 || guncelPaket.buton3); 
}
bool solTetikAsagi() { 
  return (guncelPaket.sagJoyY > 3000 || guncelPaket.buton4); 
}
bool sagTetikGeri() { 
  return (guncelPaket.solJoyX < 1000); 
}
bool sagTetikIleri() { 
  return (guncelPaket.solJoyX > 3000); 
}
// ===================== SMOOTH SCROLL =====================
// Global değişkenler
int scrollDelay = 700;   // başlangıç gecikmesi (ms)
int minDelay = 200;      // en hızlı kayma
int maxDelay = 1200;     // en yavaş kayma

// Menüde aşağı kaydırma
bool menuIleriBasti() {
  static unsigned long t = 0;
  if (solTetikAsagi()) {
    if (millis() - t > scrollDelay) {
      t = millis();
      // Basılı tutunca hızlanır (delay azalır)
      if (scrollDelay > minDelay) scrollDelay -= 50;
      return true;
    }
  } else {
    // Joystick bırakıldığında yavaşlayarak durur
    if (scrollDelay < maxDelay) scrollDelay += 30;
    t = 0;
  }
  return false;
}
// Menüde yukarı kaydırma
bool menuGeriBasti() {
  static unsigned long t = 0;
  if (solTetikYukari()) {
    if (millis() - t > scrollDelay) {
      t = millis();
      // Basılı tutunca hızlanır
      if (scrollDelay > minDelay) scrollDelay -= 50;
      return true;
    }
  } else {
    // Joystick bırakıldığında yavaşlayarak durur
    if (scrollDelay < maxDelay) scrollDelay += 30;
    t = 0;
  }
  return false;
}
// ===================== BLE / WiFi / NRF =====================
void bleGamepadBaslat() {
  bleAyarlari.setControllerType(CONTROLLER_TYPE_GAMEPAD);
  bleAyarlari.setButtonCount(14);
  bleAyarlari.setAxesMin(-32767);
  bleAyarlari.setAxesMax(32767);
  bleGamepad.begin(&bleAyarlari);
}
void oyunModuGonder() {
  if (baglantiModu != MOD_BLUETOOTH || !bleGamepad.isConnected())
    return;
  auto mapAxis = [](int16_t v, int16_t m) -> int16_t {
    int32_t s = v - m;
    if (abs(s) < 80)
      return 0;
    float x = (float)s / (s > 0 ? (4095 - m) : m);
    // Optimize S-Curve: cubic + linear blend
    float curve = (0.3f * x) + (0.7f * x * x * x);
    return (int16_t)constrain((int32_t)(curve * 32767 * globalHassasiyet),
                              -32767, 32767);
  };
  bleGamepad.setLeftThumb(mapAxis(guncelPaket.solJoyX, solJoyMerkezX),
                          mapAxis(guncelPaket.solJoyY, solJoyMerkezY));
  bleGamepad.setRightThumb(mapAxis(guncelPaket.sagJoyX, sagJoyMerkezX),
                           mapAxis(guncelPaket.sagJoyY, sagJoyMerkezY));
  guncelPaket.buton1 ? bleGamepad.press(BUTTON_1)
                     : bleGamepad.release(BUTTON_1);
  guncelPaket.buton2 ? bleGamepad.press(BUTTON_2)
                     : bleGamepad.release(BUTTON_2);
  guncelPaket.buton3 ? bleGamepad.press(BUTTON_3)
                     : bleGamepad.release(BUTTON_3);
  guncelPaket.buton4 ? bleGamepad.press(BUTTON_4)
                     : bleGamepad.release(BUTTON_4);
  guncelPaket.solJoyButon ? bleGamepad.press(BUTTON_11)
                          : bleGamepad.release(BUTTON_11);
  guncelPaket.sagJoyButon ? bleGamepad.press(BUTTON_8)
                          : bleGamepad.release(BUTTON_8);
  guncelPaket.toggle1 ? bleGamepad.press(BUTTON_5)
                      : bleGamepad.release(BUTTON_5);
  guncelPaket.toggle2 ? bleGamepad.press(BUTTON_6)
                      : bleGamepad.release(BUTTON_6);
  bleGamepad.sendReport();
}

void wifiBaslat() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID, wifiSifre);
}
void wifiGonder() {
  udp.beginPacket(pcIP, WIFI_UDP_PORT);
  udp.write((uint8_t *)&guncelPaket, sizeof(VeriPaketi));
  udp.endPacket();
}

void nrfBaslat() {
  digitalWrite(OLED_CS_PIN, HIGH);
  vTaskDelay(pdMS_TO_TICKS(10));
  if (!nrfRadyo.begin()) {
    nrfAktif = false;
    return;
  }
  nrfRadyo.setChannel(nrfKanal);
  nrfRadyo.setDataRate(RF24_250KBPS);
  nrfRadyo.setPALevel(RF24_PA_MIN);
  nrfRadyo.openWritingPipe((const uint8_t *)"AZGBY");
  nrfRadyo.stopListening();
  nrfAktif = true;
}

void nrfGonder(const VeriPaketi &p) {
  if (nrfAktif && xSemaphoreTake(spiMutex, 5) == pdTRUE) {
    nrfRadyo.write(&p, sizeof(VeriPaketi));
    xSemaphoreGive(spiMutex);
  }
}

void saatiGuncelle() {
  if (millis() - sonSaatGuncelleme >= 1000) {
    sonSaatGuncelleme += 1000;
    saatSaniye++;
    if (saatSaniye >= 60) {
      saatSaniye = 0;
      saatDakika++;
      if (saatDakika >= 60) {
        saatDakika = 0;
        saatSaat++;
        if (saatSaat >= 24)
          saatSaat = 0;
      }
    }
  }
}

void notlariYukle() {
  tercihler.begin("notlar", true);
  for (int i = 0; i < MAX_NOT_SAYISI; i++) {
    String a = "not_" + String(i);
    if (tercihler.isKey(a.c_str()))
      tercihler.getBytes(a.c_str(), &notlar[i], sizeof(Not));
    else {
      memset(&notlar[i], 0, sizeof(Not));
      snprintf(notlar[i].baslik, 20, "Bos Not %d", i + 1);
    }
  }
  tercihler.end();
}

void notuKaydet(int i) {
  tercihler.begin("notlar", false);
  String a = "not_" + String(i);
  tercihler.putBytes(a.c_str(), &notlar[i], sizeof(Not));
  tercihler.end();
}

void ayarlariKaydet() {
  tercihler.begin("azizali", false);
  tercihler.putUChar("bagMod", baglantiModu);
  tercihler.putUChar("saat", saatSaat);
  tercihler.putUChar("dakika", saatDakika);
  tercihler.putUChar("gun", tarihGun);
  tercihler.putString("ssid", wifiSSID);
  tercihler.putString("sifre", wifiSifre);
  tercihler.putString("pcip", pcIP);
  tercihler.putUChar("vol", buzzerVolume);
  tercihler.end();
}

// ===================== UI ORTAK FONKSİYONLAR =====================
void drawGameUI(String title, String scoreStr) {
  ekran.drawRect(0, 0, 128, 64, SH110X_WHITE);
  ekran.drawLine(0, 12, 128, 12, SH110X_WHITE);
  ekran.setTextSize(1);
  ekran.setCursor(3, 3);
  ekran.print(scoreStr);
  int xPos = 127 - (title.length() * 6);
  ekran.setCursor(xPos, 3);
  ekran.print(title);
}

void popupGoster(const char *baslik, const char *mesaj) {
  ekran.fillRect(10, 18, 108, 30, SH110X_BLACK);
  ekran.drawRoundRect(10, 18, 108, 30, 4, SH110X_WHITE);
  ekran.fillRect(10, 18, 108, 12, SH110X_WHITE);
  ekran.setTextColor(SH110X_BLACK);
  ekran.setCursor(14, 20);
  ekran.print(baslik);
  ekran.setTextColor(SH110X_WHITE);
  ekran.setCursor(14, 34);
  ekran.print(mesaj);
}

bool popupYenidenBaslat() {
  return (btnKenarBul(SOL_JOY_BTN, 4) || btnKenarBul(SAG_JOY_BTN, 5) ||
          btnKenarBul(BTN_YESIL, 1));
}

void canCiz(uint8_t can, uint8_t maxCan) {
  for (uint8_t i = 0; i < maxCan; i++) {
    int hx = 3 + i * 8;
    if (i < can) {
      // Dolu kalp
      ekran.fillCircle(hx + 2, 5, 2, SH110X_WHITE);
      ekran.fillCircle(hx + 5, 5, 2, SH110X_WHITE);
      ekran.fillTriangle(hx, 6, hx + 7, 6, hx + 3, 10, SH110X_WHITE);
    } else {
      ekran.drawCircle(hx + 2, 5, 2, SH110X_WHITE);
      ekran.drawCircle(hx + 5, 5, 2, SH110X_WHITE);
      ekran.drawTriangle(hx, 6, hx + 7, 6, hx + 3, 10, SH110X_WHITE);
    }
  }
}

String falCumleUret() {
  int si = random(0, FAL_SUBJECT_COUNT);
  String s = fal_subjects[si];
  String result = s + " ";
  if (random(0, 2) == 0) {
    result += fal_positives[random(0, FAL_POSITIVE_COUNT)];
  } else {
    result += fal_negatives[random(0, FAL_NEGATIVE_COUNT)];
  }
  result += ", ";
  result += fal_connectors[random(0, FAL_CONNECTOR_COUNT)];
  result += ".";
  return result;
}

// =====================MİNİMAX AI (XOX) =====================
int minimaxXOX(int b[3][3], int depth, bool isMax) {
  // Kazanma kontrolu
  for (int i = 0; i < 3; i++) {
    if (b[i][0] != 0 && b[i][0] == b[i][1] && b[i][1] == b[i][2])
      return b[i][0] == 2 ? 10 - depth : -10 + depth;
    if (b[0][i] != 0 && b[0][i] == b[1][i] && b[1][i] == b[2][i])
      return b[0][i] == 2 ? 10 - depth : -10 + depth;
  }
  if (b[0][0] != 0 && b[0][0] == b[1][1] && b[1][1] == b[2][2])
    return b[0][0] == 2 ? 10 - depth : -10 + depth;
  if (b[0][2] != 0 && b[0][2] == b[1][1] && b[1][1] == b[2][0])
    return b[0][2] == 2 ? 10 - depth : -10 + depth;
  // Berabere
  bool full = true;
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      if (b[i][j] == 0)
        full = false;
  if (full)
    return 0;
  if (depth >= 6)
    return 0;

  int best = isMax ? -100 : 100;
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) {
      if (b[i][j] == 0) {
        b[i][j] = isMax ? 2 : 1;
        int val = minimaxXOX(b, depth + 1, !isMax);
        b[i][j] = 0;
        if (isMax) {
          if (val > best)
            best = val;
        } else {
          if (val < best)
            best = val;
        }
      }
    }
  return best;
}

int enIyiHamleXOX(int b[3][3]) {
  int bestVal = -100, bestMove = -1;
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) {
      if (b[i][j] == 0) {
        b[i][j] = 2;
        int val = minimaxXOX(b, 0, false);
        b[i][j] = 0;
        if (val > bestVal) {
          bestVal = val;
          bestMove = i * 3 + j;
        }
      }
    }
  return bestMove;
}

void durumCubugu(const char *baslik) {
  ekran.fillRect(0, 0, 128, 11, SH110X_BLACK);
  ekran.setTextSize(1);
  ekran.setTextColor(SH110X_WHITE);
  if (sinyalKare % 2 == 0)
    ekran.fillRect(1, 2, 2, 6, SH110X_WHITE);
  ekran.fillRect(4, 2, 2, 6, SH110X_WHITE);
  ekran.setCursor(9, 1);
  if (baglantiModu == MOD_WIFI)
    ekran.print("[W]");
  else if (baglantiModu == MOD_BLUETOOTH)
    ekran.print("[B]");
  else
    ekran.print("[N]");
  ekran.setCursor(28, 1);
  if (baslik)
    ekran.print(baslik);
  else
    ekran.print(aktiveMod == APP_OYUN ? "[OYUN]" : "[NOT]");
  char st[15];
  snprintf(st, 15, "%02d:%02d", saatSaat, saatDakika);
  ekran.setCursor(98, 1);
  ekran.print(st);
  ekran.drawFastHLine(0, 11, 128, SH110X_WHITE);
}

void altCubukGeri() {
  ekran.drawLine(0, 54, 128, 54, SH110X_WHITE);
  ekran.setCursor(10, 56);
  ekran.print("[B] Sec    [A] Geri");
}

void ekranCiz() {
  ekran.clearDisplay();
  switch (aktifEkran) {
  case EKRAN_ANA_MENU:
    anaMenuCiz();
    break;
  case EKRAN_OYUN_MODU:
    oyunModuCiz();
    break;
  case EKRAN_NOTEPAD_LISTE:
    notepadListeCiz();
    break;
  case EKRAN_NOTEPAD_DUZENLE:
    notepadDuzenlemeCiz();
    break;
  case EKRAN_AYARLAR:
    ayarlarCiz();
    break;
  case EKRAN_BAGLANTI_AYAR:
    baglantiAyarCiz();
    break;
  case EKRAN_NRF_AYAR:
    nrfAyarCiz();
    break;
  case EKRAN_HAKKINDA:
    hakkindaCiz();
    break;
  case EKRAN_TARIH_AYAR:
    tarihAyarCiz();
    break;
  case EKRAN_SAAT_AYAR:
    saatAyarCiz();
    break;
  case EKRAN_SISTEM_GUNCELLEME: 
    guncellemeAyarCiz();
    break;
  case EKRAN_HASSASIYET_AYAR:
    hassasiyetAyarCiz();
    break;
  case EKRAN_WIFI_AYAR:
    wifiAyarCiz();
    break;
  case EKRAN_GAME_GALAGA:
    runGalaga();
    break;
  case EKRAN_GAME_LANDER:
    runLander();
    break;
  case EKRAN_GAME_FLAPPY:
    runFlappy();
    break;
  case EKRAN_GAME_TREX:
    runTRexGame();
    break;
  case EKRAN_GAME_ARKANOID:
    runArkanoid();
    break;
  case EKRAN_GAME_2048:
    run2048();
    break;
  case EKRAN_GAME_MAZE:
    runMaze();
    break;
  case EKRAN_GAME_PACMAN:
    runPacman();
    break;
  case EKRAN_GAME_SNAKE:
    runSnakeGame();
    break;
  case EKRAN_GAME_PONG:
    runPongGame();
    break;
  case EKRAN_GAME_XOX:
    runXOXGame();
    break;
  case EKRAN_GAME_MINESWEEPER:
    runMinesweeperGame();
    break;
  case EKRAN_GAME_TETRIS:
    runTetrisGame();
    break;
  case EKRAN_APP_FAL:
    runFal();
    break;
  case EKRAN_APP_POMODORO:
    runPomodoro();
    break;
  case EKRAN_GAME_DICE:
    runDiceGame();
    break;
  case EKRAN_APP_CALC:
    runCalculatorApp();
    break;
  case EKRAN_SES_AYAR:
    sesAyarCiz();
    break;
  case EKRAN_OYUNLAR_MENU:
    oyunlarMenuCiz();
    break;
  case EKRAN_UYG_MENU:
    uygMenuCiz();
    break;
  case EKRAN_WIFI_TARA:
    wifiTaramaCiz();
    break;
  case EKRAN_SNAKE_SECIM:
    snakeSecimCiz();
    break;
  case EKRAN_PONG_SECIM:
    pongSecimCiz();
    break;
  default:
    break;
  }
  ekran.drawRect(0, 0, 128, 64, SH110X_WHITE);
  ekran.display();
}

void acilisEkrani() {
  unsigned long bas = millis();
  while (millis() - bas < ACILIS_MS) {
    ekran.clearDisplay();
    int yuzde = (int)((millis() - bas) * 100 / ACILIS_MS);
    if (yuzde > 100)
      yuzde = 100;
    ekran.drawRoundRect(2, 2, 124, 60, 5, SH110X_WHITE);
    ekran.fillRect(2, 2, 124, 14, SH110X_WHITE);
    ekran.setTextColor(SH110X_BLACK);
    ekran.setTextSize(1);
    ekran.setCursor(18, 5);
    ekran.print("AZIZ ALI GAMEBOY");
    ekran.setTextColor(SH110X_WHITE);
    ekran.setCursor(34, 24);
    ekran.print("Sistem v5.0");
    ekran.drawRect(14, 40, 100, 8, SH110X_WHITE);
    ekran.fillRect(14, 40, yuzde, 8, SH110X_WHITE);
    ekran.setCursor(55, 50);
    ekran.printf("%d%%", yuzde);
    ekran.display();
    // Acilis melodisi
    if (yuzde == 25)
      tone(BUZZER_PIN, 800, 50);
    if (yuzde == 50)
      tone(BUZZER_PIN, 1000, 50);
    if (yuzde == 75)
      tone(BUZZER_PIN, 1200, 50);
    if (yuzde == 99)
      tone(BUZZER_PIN, 1600, 100);
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// ===================== ANA MENÜ =====================
void anaMenuCiz() {
  durumCubugu("Ana Menu");
  if (menuGeriBasti()) {
    anaMenuSecim = max(anaMenuSecim - 1, 0);
    bipMenuSec();
  }
  if (menuIleriBasti()) {
    anaMenuSecim = min(anaMenuSecim + 1, TOTAL_MENU_ITEMS - 1);
    bipMenuSec();
  }
  if (btnKenarBul(BTN_YESIL, 1) || btnKenarBul(SOL_JOY_BTN, 4) ||
      btnKenarBul(SAG_JOY_BTN, 5)) {
    bipTek();
    switch (anaMenuSecim) {
    case 0: aktifEkran = EKRAN_OYUN_MODU; break;
    case 1: 
      aktifEkran = EKRAN_NOTEPAD_LISTE; 
      aktiveMod = APP_NOTEPAD; 
      break;
    case 2: aktifEkran = EKRAN_AYARLAR; break;
    case 3: aktifEkran = EKRAN_OYUNLAR_MENU; gameMenuSecim=0; break;
    case 4: aktifEkran = EKRAN_UYG_MENU; uygMenuSecim=0; break;
    case 5: aktifEkran = EKRAN_HAKKINDA; hakkindaScrollY=0; break;
    }
  }
  int baslangic = (anaMenuSecim > 3) ? anaMenuSecim - 3 : 0;
  for (int i = 0; i < 4; i++) {
    int gi = baslangic + i;
    if (gi >= TOTAL_MENU_ITEMS)
      break;
    int y = 16 + i * 10;
    if (gi == anaMenuSecim)
      ekran.fillTriangle(4, y, 4, y + 6, 8, y + 3, SH110X_WHITE);
    ekran.setCursor(12, y);
    ekran.printf("%s", anaMenuOgeleri[gi]);
  }
  ekran.drawLine(124, 16, 124, 50, SH110X_WHITE);
  int sY = map(anaMenuSecim, 0, TOTAL_MENU_ITEMS - 1, 16, 46);
  ekran.fillRect(123, sY, 3, 4, SH110X_WHITE);
}

void oyunModuCiz() {
  durumCubugu("Oyun Test");
  ekran.drawRoundRect(2, 14, 60, 48, 4, SH110X_WHITE);
  ekran.fillRect(2, 14, 60, 11, SH110X_WHITE);
  ekran.setTextColor(SH110X_BLACK);
  ekran.setCursor(14, 16);
  ekran.print("TUSLAR");
  ekran.setTextColor(SH110X_WHITE);
  auto tusYaz = [](int x, int y, const char *isim, bool durum) {
    ekran.setCursor(x, y);
    ekran.print(isim);
    int yg = strlen(isim) * 6;
    if (durum)
      ekran.fillRect(x + yg + 2, y, 6, 7, SH110X_WHITE);
    else
      ekran.drawRect(x + yg + 2, y, 6, 7, SH110X_WHITE);
  };
  tusYaz(5, 27, "A", guncelPaket.buton1);
  tusYaz(33, 27, "B", guncelPaket.buton2);
  tusYaz(5, 36, "X", guncelPaket.buton3);
  tusYaz(33, 36, "Y", guncelPaket.buton4);
  tusYaz(5, 45, "L1", guncelPaket.toggle1);
  tusYaz(33, 45, "R1", guncelPaket.toggle2);
  tusYaz(5, 54, "L3", guncelPaket.solJoyButon);
  tusYaz(33, 54, "R3", guncelPaket.sagJoyButon);
  ekran.drawRoundRect(65, 14, 61, 48, 4, SH110X_WHITE);
  ekran.fillRect(65, 14, 61, 11, SH110X_WHITE);
  ekran.setTextColor(SH110X_BLACK);
  ekran.setCursor(77, 16);
  ekran.print("ANALOG");
  ekran.setTextColor(SH110X_WHITE);
  ekran.setCursor(72, 27);
  ekran.printf("SX:%04d", guncelPaket.solJoyX);
  ekran.setCursor(72, 36);
  ekran.printf("SY:%04d", guncelPaket.solJoyY);
  ekran.setCursor(72, 45);
  ekran.printf("RX:%04d", guncelPaket.sagJoyX);
  ekran.setCursor(72, 54);
  ekran.printf("RY:%04d", guncelPaket.sagJoyY);
  if (btnKenarBul(BTN_KIRMIZI, 0))
    aktifEkran = EKRAN_ANA_MENU;
}

// ===================== NOT DEFTERİ KONTROLLER =====================
bool harfSecSol() {
  static unsigned long t = 0;
  if (guncelPaket.sagJoyX < 1000) {
    if (millis() - t > 150) {
      t = millis();
      return true;
    }
  } else {
    t = 0;
  }
  return false;
}
bool harfSecSag() {
  static unsigned long t = 0;
  if (guncelPaket.sagJoyX > 3000) {
    if (millis() - t > 150) {
      t = millis();
      return true;
    }
  } else {
    t = 0;
  }
  return false;
}
bool harfSecYukari() {
  static unsigned long t = 0;
  if (guncelPaket.sagJoyY < 1000) {
    if (millis() - t > 150) {
      t = millis();
      return true;
    }
  } else {
    t = 0;
  }
  return false;
}
bool harfSecAsagi() {
  static unsigned long t = 0;
  if (guncelPaket.sagJoyY > 3000) {
    if (millis() - t > 150) {
      t = millis();
      return true;
    }
  } else {
    t = 0;
  }
  return false;
}
bool harfYazildi() { return btnKenarBul(BTN_YESIL, 10); }
bool editSil() {
  static unsigned long t = 0;
  if (guncelPaket.solJoyX < 1000) {
    if (millis() - t > 150) {
      t = millis();
      return true;
    }
  } else {
    t = 0;
  }
  return false;
}
bool editBosluk() {
  static unsigned long t = 0;
  if (guncelPaket.solJoyX > 3000) {
    if (millis() - t > 150) {
      t = millis();
      return true;
    }
  } else {
    t = 0;
  }
  return false;
}
bool editBuyukHarf() {
  static unsigned long t = 0;
  if (guncelPaket.solJoyY < 1000) {
    if (millis() - t > 150) {
      t = millis();
      return true;
    }
  } else {
    t = 0;
  }
  return false;
}
bool editKarakterMod() {
  static unsigned long t = 0;
  if (guncelPaket.solJoyY > 3000) {
    if (millis() - t > 150) {
      t = millis();
      return true;
    }
  } else {
    t = 0;
  }
  return false;
}
bool editEnter() { return btnKenarBul(SOL_JOY_BTN, 11); }

void notepadModuIsle() {
  if (aktifEkran != EKRAN_NOTEPAD_DUZENLE && aktifEkran != EKRAN_WIFI_AYAR)
    return;
  if (aktifEkran == EKRAN_WIFI_AYAR && !wifiDuzenleModu)
    return;
  char *buf = (aktifEkran == EKRAN_WIFI_AYAR) ? wifiDuzenlemeBuf : duzenlemeBuf;
  int maxLen = (aktifEkran == EKRAN_WIFI_AYAR) ? 63 : NOT_MAX_KARAKTER - 1;

  if (harfSecSol())
    karSecimiX = max(karSecimiX - 1, 0);
  if (harfSecSag())
    karSecimiX = min(karSecimiX + 1, 12);
  if (harfSecYukari())
    karSecimiY = max(karSecimiY - 1, 0);
  if (harfSecAsagi())
    karSecimiY = min(karSecimiY + 1, 1);
  if (editSil()) {
    if (duzenlemeImlecu > 0) {
      duzenlemeImlecu--;
      memmove(&buf[duzenlemeImlecu], &buf[duzenlemeImlecu + 1],
              strlen(buf) - duzenlemeImlecu);
    }
  }
  if (editBosluk()) {
    if (strlen(buf) < maxLen) {
      memmove(&buf[duzenlemeImlecu + 1], &buf[duzenlemeImlecu],
              strlen(buf) - duzenlemeImlecu + 1);
      buf[duzenlemeImlecu] = ' ';
      duzenlemeImlecu++;
    }
  }
  if (editBuyukHarf())
    aktifKarSeti = (aktifKarSeti == 0) ? 1 : 0;
  if (editKarakterMod())
    aktifKarSeti = (aktifKarSeti == 2) ? 0 : 2;
  if (harfYazildi()) {
    if (strlen(buf) < maxLen) {
      memmove(&buf[duzenlemeImlecu + 1], &buf[duzenlemeImlecu],
              strlen(buf) - duzenlemeImlecu + 1);
      buf[duzenlemeImlecu] =
          klavyeSetleri[aktifKarSeti][karSecimiY * 13 + karSecimiX];
      duzenlemeImlecu++;
    }
  }
  if (aktifEkran == EKRAN_NOTEPAD_DUZENLE) {
    if (editEnter()) {
      strncpy(notlar[aktifNotIndeks].icerik, duzenlemeBuf, NOT_MAX_KARAKTER);
      strncpy(notlar[aktifNotIndeks].baslik, duzenlemeBuf, 18);
      notlar[aktifNotIndeks].baslik[18] = '\0';
      if (strlen(duzenlemeBuf) == 0)
        strcpy(notlar[aktifNotIndeks].baslik, "Bos Not");
      notuKaydet(aktifNotIndeks);
      aktifEkran = EKRAN_NOTEPAD_LISTE;
      bipBasari();
    }
    if (btnKenarBul(BTN_KIRMIZI, 0))
      aktifEkran = EKRAN_NOTEPAD_LISTE;
  }
  if (aktifEkran == EKRAN_WIFI_AYAR) {
    if (editEnter()) {
        if (wifiDuzenleHedef == 0) strncpy(wifiSSID, wifiDuzenlemeBuf, 31);
        else if (wifiDuzenleHedef == 1) strncpy(wifiSifre, wifiDuzenlemeBuf, 63);
        wifiDuzenleModu = false;
        ayarlariKaydet();
        
        // Baglanti Testi Popup
        popupGoster("BAGLANIYOR", "Lutfen Bekleyin...");
        ekran.display();
        WiFi.begin(wifiSSID, wifiSifre);
        int sayac = 0;
        while (WiFi.status() != WL_CONNECTED && sayac < 10) {
            vTaskDelay(pdMS_TO_TICKS(500));
            sayac++;
        }
        if (WiFi.status() == WL_CONNECTED) {
            wifiBagliMi = true;
            popupGoster("BASARILI", "WiFi Baglandi!");
            bipBasari();
        } else {
            wifiBagliMi = false;
            popupGoster("HATA", "Baglanamadi!");
            bipHata();
        }
        ekran.display();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    if (btnKenarBul(BTN_KIRMIZI, 0))
      wifiDuzenleModu = false;
  }
}

void notepadListeCiz() {
  durumCubugu("Not Defteri");
  if (menuGeriBasti())
    notListeSecim = max(notListeSecim - 1, 0);
  if (menuIleriBasti())
    notListeSecim = min(notListeSecim + 1, MAX_NOT_SAYISI - 1);
  if (btnKenarBul(BTN_YESIL, 1)) {
    aktifNotIndeks = notListeSecim;
    strncpy(duzenlemeBuf, notlar[aktifNotIndeks].icerik, NOT_MAX_KARAKTER);
    duzenlemeBuf[NOT_MAX_KARAKTER] = '\0';
    duzenlemeImlecu = strlen(duzenlemeBuf);
    karSecimiX = 0;
    karSecimiY = 0;
    aktifKarSeti = 0;
    aktifEkran = EKRAN_NOTEPAD_DUZENLE;
    bipMenuSec();
  }
  if (btnKenarBul(TOGGLE2_PIN, 3)) {
    memset(&notlar[notListeSecim], 0, sizeof(Not));
    snprintf(notlar[notListeSecim].baslik, 20, "Bos Not %d", notListeSecim + 1);
    notuKaydet(notListeSecim);
    bipHata();
  }
  if (btnKenarBul(BTN_KIRMIZI, 0)) {
    aktifEkran = EKRAN_ANA_MENU;
    aktiveMod = APP_OYUN;
  }
  ekran.setTextSize(1);
  int baslangic = (notListeSecim > 4) ? notListeSecim - 4 : 0;
  for (int i = 0; i < 5 && (i + baslangic) < MAX_NOT_SAYISI; i++) {
    int gi = i + baslangic;
    int y = 14 + i * 8;
    if (gi == notListeSecim) {
      ekran.fillRect(1, y - 1, 126, 9, SH110X_WHITE);
      ekran.setTextColor(SH110X_BLACK);
    } else
      ekran.setTextColor(SH110X_WHITE);
    char satir[25];
    snprintf(satir, 25, "%d: %s", gi + 1, notlar[gi].baslik);
    ekran.setCursor(3, y);
    ekran.print(satir);
  }
  ekran.setTextColor(SH110X_WHITE);
  ekran.drawLine(0, 54, 128, 54, SH110X_WHITE);
  ekran.setCursor(4, 56);
  ekran.print("[B]Ac [A]Geri [R1]Sil");
}

void notepadDuzenlemeCiz() {
  durumCubugu("Duzenle");
  ekran.drawRect(2, 13, 124, 14, SH110X_WHITE);
  int baslangic = 0;
  if (duzenlemeImlecu > 19)
    baslangic = duzenlemeImlecu - 19;
  char gorMetin[22] = {0};
  strncpy(gorMetin, duzenlemeBuf + baslangic, 20);
  ekran.setCursor(4, 16);
  ekran.print(gorMetin);
  int imlecX = duzenlemeImlecu - baslangic;
  if ((millis() / 300) % 2 == 0)
    ekran.fillRect(4 + imlecX * 6, 23, 5, 2, SH110X_WHITE);
  for (int i = 0; i < 26; i++) {
    int cX = i % 13;
    int cY = i / 13;
    int pX = 6 + cX * 9;
    int pY = 31 + cY * 11;
    if (cX == karSecimiX && cY == karSecimiY) {
      ekran.fillRect(pX - 1, pY - 1, 7, 9, SH110X_WHITE);
      ekran.setTextColor(SH110X_BLACK);
    } else
      ekran.setTextColor(SH110X_WHITE);
    ekran.setCursor(pX, pY);
    ekran.print(klavyeSetleri[aktifKarSeti][i]);
  }
  ekran.setTextColor(SH110X_WHITE);
  ekran.drawLine(0, 53, 128, 53, SH110X_WHITE);
  ekran.setCursor(2, 55);
  ekran.printf("%s L3:Kydt L_X:Sil/Bos", klavyeAdlari[aktifKarSeti]);
}

// ===================== AYARLAR =====================
void ayarlarCiz() {
  durumCubugu("Ayarlar");
  if (menuGeriBasti())
    ayarlarSecim = max(ayarlarSecim - 1, 0);
  if (menuIleriBasti())
    ayarlarSecim = min(ayarlarSecim + 1, 7); // 8 item (0-7)
  if (btnKenarBul(BTN_YESIL, 1) || btnKenarBul(SOL_JOY_BTN, 4)) {
    bipMenuSec();
    if (ayarlarSecim == 0) aktifEkran = EKRAN_TARIH_AYAR;
    else if (ayarlarSecim == 1) aktifEkran = EKRAN_SAAT_AYAR;
    else if (ayarlarSecim == 2) aktifEkran = EKRAN_BAGLANTI_AYAR;
    else if (ayarlarSecim == 3) aktifEkran = EKRAN_NRF_AYAR;
    else if (ayarlarSecim == 4) aktifEkran = EKRAN_SISTEM_GUNCELLEME;
    else if (ayarlarSecim == 5) aktifEkran = EKRAN_HASSASIYET_AYAR;
    else if (ayarlarSecim == 6) aktifEkran = EKRAN_WIFI_AYAR;
    else if (ayarlarSecim == 7) aktifEkran = EKRAN_SES_AYAR;
  }
  if (btnKenarBul(BTN_KIRMIZI, 0))
    aktifEkran = EKRAN_ANA_MENU;
  int baslangic = (ayarlarSecim > 3) ? ayarlarSecim - 3 : 0;
  for (int i = 0; i < 4; i++) {
    int gi = baslangic + i;
    if (gi >= 8)
      break;
    int y = 16 + i * 9;
    if (gi == ayarlarSecim)
      ekran.fillTriangle(4, y, 4, y + 6, 8, y + 3, SH110X_WHITE);
    ekran.setCursor(12, y);
    ekran.printf("[+] %s", ayarOgeleri[gi]);
  }
  ekran.drawLine(124, 16, 124, 50, SH110X_WHITE);
  int sY = map(ayarlarSecim, 0, 7, 16, 46);
  ekran.fillRect(123, sY, 3, 4, SH110X_WHITE);
  altCubukGeri();
}

void hassasiyetAyarCiz() {
  durumCubugu("Hassasiyet");
  ekran.drawRoundRect(10, 16, 108, 36, 4, SH110X_WHITE);
  ekran.fillRect(10, 16, 108, 10, SH110X_WHITE);
  ekran.setTextColor(SH110X_BLACK);
  ekran.setCursor(28, 17);
  ekran.print("HIZ KATSAYISI");
  ekran.setTextColor(SH110X_WHITE);
  // Joystick ile kontrol
  if (solTetikSaga()) {
    globalHassasiyet = min(globalHassasiyet + 0.1f, 3.0f);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  if (solTetikSola()) {
    globalHassasiyet = max(globalHassasiyet - 0.1f, 0.5f);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  // Mavi buton artir, Sari buton azalt
  if (btnKenarBul(BTN_MAVI, 2)) {
    globalHassasiyet = min(globalHassasiyet + 0.1f, 3.0f);
    bipMenuSec();
  }
  if (btnKenarBul(BTN_SARI, 3)) {
    globalHassasiyet = max(globalHassasiyet - 0.1f, 0.5f);
    bipMenuSec();
  }
  ekran.setCursor(50, 28);
  ekran.printf("x%.1f", globalHassasiyet);
  ekran.drawRect(20, 38, 88, 6, SH110X_WHITE);
  int barGen = map((int)(globalHassasiyet * 10), 5, 30, 0, 84);
  ekran.fillRect(22, 40, barGen, 2, SH110X_WHITE);
  if (btnKenarBul(BTN_YESIL, 1)) {
    tercihler.begin("azizali", false);
    tercihler.putFloat("hasiy", globalHassasiyet);
    tercihler.end();
    aktifEkran = EKRAN_AYARLAR;
    bipBasari();
  }
  if (btnKenarBul(BTN_KIRMIZI, 0))
    aktifEkran = EKRAN_AYARLAR;
  ekran.setCursor(2, 55);
  ekran.print("[X]+[Y]- [B]Kyd [A]Cik");
}

void wifiAyarCiz() {
  durumCubugu("WiFi Giris");
  if (wifiDuzenleModu) {
    // Baslik: ne duzenleniyor
    ekran.drawRoundRect(2, 7, 124, 14, 2, SH110X_WHITE);
    ekran.fillRect(2, 7, 124, 10, SH110X_WHITE);
    ekran.setTextColor(SH110X_BLACK);
    ekran.setCursor(6, 8);
    ekran.print(wifiDuzenleHedef == 0 ? "SSID Gir:" : "Sifre Gir:");
    ekran.setTextColor(SH110X_WHITE);
    // Yazi alani
    int bas = 0;
    if (duzenlemeImlecu > 19)
      bas = duzenlemeImlecu - 19;
    char gM[22] = {0};
    strncpy(gM, wifiDuzenlemeBuf + bas, 20);
    ekran.setCursor(4, 21);
    ekran.print(gM);
    int iX = duzenlemeImlecu - bas;
    if ((millis() / 300) % 2 == 0)
      ekran.fillRect(4 + iX * 6, 28, 5, 2, SH110X_WHITE);
    // Klavye
    for (int i = 0; i < 26; i++) {
        int cX = i % 13, cY = i / 13, pX = 6 + cX * 9, pY = 32 + cY * 11;
        if (cX == karSecimiX && cY == karSecimiY) {
            ekran.fillRect(pX - 1, pY - 1, 7, 9, SH110X_WHITE);
            ekran.setTextColor(SH110X_BLACK);
        } else
            ekran.setTextColor(SH110X_WHITE);
        ekran.setCursor(pX, pY);
        ekran.print(klavyeSetleri[aktifKarSeti][i]);
    }
    ekran.setTextColor(SH110X_WHITE);
    ekran.drawLine(0, 54, 128, 54, SH110X_WHITE);
    ekran.setCursor(2, 55);
    ekran.printf("%s L3:Kyd [A]Geri", klavyeAdlari[aktifKarSeti]);
    if (btnKenarBul(BTN_KIRMIZI, 0)) { wifiDuzenleModu = false; }
  } else {
    if (menuGeriBasti()) wifiAyarSecim = max(wifiAyarSecim - 1, 0);
    if (menuIleriBasti()) wifiAyarSecim = min(wifiAyarSecim + 1, 1);
    if (btnKenarBul(BTN_YESIL, 1) && !wifiDuzenleModu) {
      wifiDuzenleHedef = wifiAyarSecim;
      wifiDuzenleModu = true;
      if (wifiAyarSecim == 0) strncpy(wifiDuzenlemeBuf, wifiSSID, 63);
      else strncpy(wifiDuzenlemeBuf, wifiSifre, 63);
      duzenlemeImlecu = strlen(wifiDuzenlemeBuf);
      karSecimiX = 0; karSecimiY = 0; aktifKarSeti = 0;
      bipMenuSec();
    }
    if (btnKenarBul(BTN_MAVI, 2)) {
        // WiFi Tarama Baslat
        int n = WiFi.scanNetworks();
        if(n > 0) {
            for(int i=0; i<min(n, 15); i++) {
                agIsimleri[i] = WiFi.SSID(i);
            }
            bulunanAgSayisi = min(n, 15);
            aktifEkran = EKRAN_WIFI_TARA;
            wifiTaramaSecim = 0;
            bipTek();
        }
    }
    if (btnKenarBul(BTN_KIRMIZI, 0)) aktifEkran = EKRAN_AYARLAR;
    // UI 6px up
    ekran.drawRoundRect(4, 9, 120, 18, 3, SH110X_WHITE);
    if (wifiAyarSecim == 0) ekran.fillRoundRect(4, 9, 120, 18, 3, SH110X_WHITE);
    ekran.setTextColor(wifiAyarSecim == 0 ? SH110X_BLACK : SH110X_WHITE);
    ekran.setCursor(8, 11); ekran.print("SSID:");
    char ssTmp[17] = {0}; strncpy(ssTmp, wifiSSID, 16);
    ekran.setCursor(8, 19); ekran.print(ssTmp);
    ekran.drawRoundRect(4, 30, 120, 18, 3, SH110X_WHITE);
    if (wifiAyarSecim == 1) ekran.fillRoundRect(4, 30, 120, 18, 3, SH110X_WHITE);
    ekran.setTextColor(wifiAyarSecim == 1 ? SH110X_BLACK : SH110X_WHITE);
    ekran.setCursor(8, 32); ekran.print("Sifre:");
    ekran.setCursor(8, 40); for (int j = 0; j < min((int)strlen(wifiSifre), 16); j++) ekran.print('*');
    ekran.setTextColor(SH110X_WHITE);
    ekran.drawLine(0, 51, 128, 51, SH110X_WHITE);
    ekran.setCursor(6, 53); ekran.print("[B]Sec [X]Tara [A]Geri");
  }
}

void wifiTaramaCiz() {
    durumCubugu("WiFi Aglari");
    if(menuGeriBasti()) wifiTaramaSecim = max(wifiTaramaSecim-1, 0);
    if(menuIleriBasti()) wifiTaramaSecim = min(wifiTaramaSecim+1, bulunanAgSayisi-1);
    
    int bas = (wifiTaramaSecim > 3) ? wifiTaramaSecim - 3 : 0;
    for(int i=0; i<4; i++) {
        int gi = bas + i;
        if(gi >= bulunanAgSayisi) break;
        int y = 16 + i * 10;
        if(gi == wifiTaramaSecim) ekran.fillTriangle(4, y, 4, y+6, 8, y+3, SH110X_WHITE);
        ekran.setCursor(12, y);
        ekran.print(agIsimleri[gi].substring(0, 18));
    }
    if(btnKenarBul(BTN_YESIL, 1)) {
        strncpy(wifiSSID, agIsimleri[wifiTaramaSecim].c_str(), 31);
        aktifEkran = EKRAN_WIFI_AYAR;
        wifiAyarSecim = 1; // Sifreye gec
        bipTek();
    }
    if(btnKenarBul(BTN_KIRMIZI, 0)) aktifEkran = EKRAN_WIFI_AYAR;
}

void oyunlarMenuCiz() {
    durumCubugu("Oyunlar");
    if(menuGeriBasti()) gameMenuSecim = max(gameMenuSecim-1, 0);
    if(menuIleriBasti()) gameMenuSecim = min(gameMenuSecim+1, TOTAL_GAME_ITEMS-1);
    if(btnKenarBul(BTN_YESIL, 1)) {
        bipTek();
        switch(gameMenuSecim) {
            case 0: aktifEkran=EKRAN_GAME_GALAGA; gal_act=false; break;
            case 1: aktifEkran=EKRAN_GAME_LANDER; lnd_act=false; break;
            case 2: aktifEkran=EKRAN_GAME_FLAPPY; flp_act=false; break;
            case 3: aktifEkran=EKRAN_GAME_TREX; rex_act=false; break;
            case 4: aktifEkran=EKRAN_GAME_ARKANOID; ark_act=false; break;
            case 5: aktifEkran=EKRAN_GAME_2048; g2048_act=false; break;
            case 6: aktifEkran=EKRAN_GAME_MAZE; maze_act=false; break;
            case 7: aktifEkran=EKRAN_GAME_PACMAN; pac_act=false; break;
            case 8: aktifEkran=EKRAN_SNAKE_SECIM; break;
            case 9: aktifEkran=EKRAN_PONG_SECIM; break;
            case 10: aktifEkran=EKRAN_GAME_XOX; xox_active=false; break;
            case 11: aktifEkran=EKRAN_GAME_MINESWEEPER; mine_act=false; break;
            case 12: aktifEkran=EKRAN_GAME_TETRIS; tet_act=false; break;
        }
    }
    if(btnKenarBul(BTN_KIRMIZI, 0)) aktifEkran = EKRAN_ANA_MENU;
    int bas = (gameMenuSecim > 3) ? gameMenuSecim - 3 : 0;
    for(int i=0; i<4; i++) {
        int gi = bas + i;
        if(gi >= TOTAL_GAME_ITEMS) break;
        int y = 16 + i * 10;
        if(gi == gameMenuSecim) ekran.fillTriangle(4, y, 4, y+6, 8, y+3, SH110X_WHITE);
        ekran.setCursor(12, y);
        ekran.print(oyunMenuOgeleri[gi]);
    }
}

void uygMenuCiz() {
    durumCubugu("Uygulamalar");
    if(menuGeriBasti()) uygMenuSecim = max(uygMenuSecim-1, 0);
    if(menuIleriBasti()) uygMenuSecim = min(uygMenuSecim+1, TOTAL_APP_ITEMS-1);
    if(btnKenarBul(BTN_YESIL, 1)) {
        bipTek();
        switch(uygMenuSecim) {
            case 0: aktifEkran=EKRAN_APP_FAL; fal_act=false; break;
            case 1: aktifEkran=EKRAN_APP_POMODORO; break;
            case 2: aktifEkran=EKRAN_GAME_DICE; break;
            case 3: aktifEkran=EKRAN_APP_CALC; break;
        }
    }
    if(btnKenarBul(BTN_KIRMIZI, 0)) aktifEkran = EKRAN_ANA_MENU;
    int bas = (uygMenuSecim > 3) ? uygMenuSecim - 3 : 0;
    for(int i=0; i<4; i++) {
        int gi = bas + i;
        if(gi >= TOTAL_APP_ITEMS) break;
        int y = 16 + i * 10;
        if(gi == uygMenuSecim) ekran.fillTriangle(4, y, 4, y+6, 8, y+3, SH110X_WHITE);
        ekran.setCursor(12, y);
        ekran.print(uygMenuOgeleri[gi]);
    }
}

void sesAyarCiz() {
    durumCubugu("Ses Ayari");
    if(guncelPaket.sagJoyX < 1000) { buzzerVolume = max(buzzerVolume-1, 0); vTaskDelay(pdMS_TO_TICKS(150)); bipTek(); }
    if(guncelPaket.sagJoyX > 3000) { buzzerVolume = min(buzzerVolume+1, 10); vTaskDelay(pdMS_TO_TICKS(150)); bipTek(); }
    ekran.setCursor(45, 28);
    ekran.printf("Vol: %d", buzzerVolume);
    ekran.drawRect(20, 38, 88, 8, SH110X_WHITE);
    ekran.fillRect(22, 40, map(buzzerVolume, 0, 10, 0, 84), 4, SH110X_WHITE);
    if(btnKenarBul(BTN_YESIL, 1)) {
        tercihler.begin("azizali", false);
        tercihler.putUChar("vol", buzzerVolume);
        tercihler.end();
        aktifEkran = EKRAN_AYARLAR;
        bipBasari();
    }
    if(btnKenarBul(BTN_KIRMIZI, 0)) aktifEkran = EKRAN_AYARLAR;
    ekran.setCursor(2, 55);
    ekran.print("R-Joy: +/- [B]Kyd [A]Cik");
}

void guncellemeAyarCiz() {
  durumCubugu("Guncelleme");
  ekran.fillTriangle(4, 20, 4, 26, 8, 23, SH110X_WHITE);
  ekran.setCursor(12, 20);
  ekran.print("[+] Saat ve Tarihi");
  ekran.setCursor(35, 30);
  ekran.print("Guncelle");
  if (btnKenarBul(BTN_YESIL, 1)) {
    auto popUpCiz = [](const char *mesaj, int xK) {
      ekran.clearDisplay();
      ekran.drawRoundRect(4, 15, 120, 34, 4, SH110X_WHITE);
      ekran.fillRect(4, 15, 120, 12, SH110X_WHITE);
      ekran.setTextColor(SH110X_BLACK);
      ekran.setCursor(15, 17);
      ekran.print("INTERNET SENKRONU");
      ekran.setTextColor(SH110X_WHITE);
      ekran.setCursor(xK, 33);
      ekran.print(mesaj);
      ekran.display();
    };
    if (baglantiModu != MOD_WIFI || !wifiBagliMi) {
      popUpCiz("Hata: WiFi Yok!", 18);
      bipHata();
      vTaskDelay(pdMS_TO_TICKS(2000));
    } else {
      popUpCiz("Veri Cekiliyor...", 16);
      configTime(10800, 0, "pool.ntp.org");
      struct tm timeinfo;
      if (!getLocalTime(&timeinfo, 5000)) {
        popUpCiz("NTP Sunucu Hatasi!", 12);
        bipHata();
      } else {
        saatSaat = timeinfo.tm_hour;
        saatDakika = timeinfo.tm_min;
        saatSaniye = timeinfo.tm_sec;
        tarihGun = timeinfo.tm_mday;
        tarihAy = timeinfo.tm_mon + 1;
        tarihYil = timeinfo.tm_year + 1900;
        ayarlariKaydet();
        popUpCiz("Basariyla Eslendi!", 12);
        bipBasari();
      }
      vTaskDelay(pdMS_TO_TICKS(2000));
    }
    aktifEkran = EKRAN_AYARLAR;
  }
  if (btnKenarBul(BTN_KIRMIZI, 0))
    aktifEkran = EKRAN_AYARLAR;
  altCubukGeri();
}

void modGecisIslemi(int yeniMod) {
  unsigned long bas = millis();
  int gecisSuresi = 2500;
  while (millis() - bas < gecisSuresi) {
    ekran.clearDisplay();
    int yuzde = (int)((millis() - bas) * 100 / gecisSuresi);
    if (yuzde > 100)
      yuzde = 100;
    ekran.drawRoundRect(2, 2, 124, 60, 5, SH110X_WHITE);
    ekran.fillRect(2, 2, 124, 14, SH110X_WHITE);
    ekran.setTextColor(SH110X_BLACK);
    ekran.setTextSize(1);
    ekran.setCursor(15, 5);
    ekran.print("BAGLANTI DEGISIMI");
    ekran.setTextColor(SH110X_WHITE);
    if (yeniMod == MOD_WIFI) {
      ekran.setCursor(20, 22);
      ekran.print("-> WiFi Modu <-");
    } else if (yeniMod == MOD_BLUETOOTH) {
      ekran.setCursor(18, 22);
      ekran.print("-> Bluetooth <-");
    } else {
      ekran.setCursor(14, 22);
      ekran.print("-> NRF Telsiz <-");
    }
    ekran.drawRoundRect(14, 38, 100, 10, 3, SH110X_WHITE);
    ekran.fillRoundRect(16, 40, (yuzde * 96 / 100), 6, 2, SH110X_WHITE);
    ekran.setCursor(55, 50);
    ekran.printf("%d%%", yuzde);
    ekran.display();
    vTaskDelay(pdMS_TO_TICKS(15));
  }
  if (baglantiModu == MOD_WIFI) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    wifiBagliMi = false;
  }
  if (baglantiModu == MOD_NRF_KOPRU)
    nrfAktif = false;
  baglantiModu = yeniMod;
  ayarlariKaydet();
  String sm = "";
  if (yeniMod == MOD_WIFI) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSSID, wifiSifre);
    int d = 0;
    while (WiFi.status() != WL_CONNECTED && d < 10) {
      vTaskDelay(pdMS_TO_TICKS(500));
      d++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      wifiBagliMi = true;
      sm = "WiFi Baglandi!";
      bipTek();
    } else {
      sm = "Baglanti Hatasi!";
      bipCift();
    }
  } else if (yeniMod == MOD_BLUETOOTH) {
    if (!bleBasladiMi) {
      bleGamepadBaslat();
      bleBasladiMi = true;
    }
    sm = "Bluetooth Aktif!";
    bipTek();
  } else if (yeniMod == MOD_NRF_KOPRU) {
    digitalWrite(OLED_CS_PIN, HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    if (!nrfRadyo.begin()) {
      sm = "Hata: NRF Yok!";
      bipUclu();
    } else {
      nrfRadyo.setChannel(nrfKanal);
      nrfRadyo.setDataRate(RF24_250KBPS);
      nrfRadyo.setPALevel(RF24_PA_MIN);
      nrfRadyo.openWritingPipe((const uint8_t *)"AZGBY");
      nrfRadyo.stopListening();
      nrfAktif = true;
      sm = "NRF Baglandi!";
      bipTek();
    }
  }
  ekran.clearDisplay();
  ekran.drawRoundRect(6, 14, 116, 36, 4, SH110X_WHITE);
  ekran.drawRect(8, 16, 112, 32, SH110X_WHITE);
  ekran.fillRect(8, 16, 112, 12, SH110X_WHITE);
  ekran.setTextColor(SH110X_BLACK);
  ekran.setCursor(26, 18);
  ekran.print("SISTEM DURUMU");
  ekran.setTextColor(SH110X_WHITE);
  int xPos = 64 - (sm.length() * 3);
  ekran.setCursor(xPos, 34);
  ekran.print(sm);
  ekran.display();
  vTaskDelay(pdMS_TO_TICKS(2000));
  aktifEkran = EKRAN_AYARLAR;
}

void baglantiAyarCiz() {
  durumCubugu("Baglanti");
  if (menuGeriBasti())
    baglantiSecim = max(baglantiSecim - 1, 0);
  if (menuIleriBasti())
    baglantiSecim = min(baglantiSecim + 1, 2);
  if (btnKenarBul(BTN_YESIL, 1)) {
    modGecisIslemi(baglantiSecim);
    return;
  }
  if (btnKenarBul(BTN_KIRMIZI, 0))
    aktifEkran = EKRAN_AYARLAR;
  for (int i = 0; i < 3; i++) {
    int y = 18 + i * 11;
    if (i == baglantiSecim)
      ekran.fillTriangle(4, y, 4, y + 6, 8, y + 3, SH110X_WHITE);
    ekran.setCursor(12, y);
    ekran.printf("[%c] %s", (i == baglantiModu ? 'X' : ' '), bagModAdi[i]);
  }
  altCubukGeri();
}

void hakkindaCiz() {
  durumCubugu("Hakkinda");
  if (solTetikYukari())
    hakkindaScrollY = max(hakkindaScrollY - 1, 0);
  if (solTetikAsagi())
    hakkindaScrollY = min(hakkindaScrollY + 1, HAKKINDA_SATIR_SAYISI - 3);
  if (btnKenarBul(BTN_KIRMIZI, 0))
    aktifEkran = EKRAN_ANA_MENU;
  for (int i = 0; i < 3 && (i + hakkindaScrollY) < HAKKINDA_SATIR_SAYISI; i++) {
    char buf[30];
    strcpy_P(buf, hakkindaMetni[i + hakkindaScrollY]);
    ekran.setCursor(4, 18 + i * 10);
    ekran.print(buf);
  }
  ekran.drawLine(124, 16, 124, 50, SH110X_WHITE);
  int sY = map(hakkindaScrollY, 0, HAKKINDA_SATIR_SAYISI - 3, 16, 46);
  ekran.fillRect(123, sY, 3, 4, SH110X_WHITE);
  altCubukGeri();
}

void tarihAyarCiz() {
  durumCubugu("Tarih Ayari");
  ekran.drawRoundRect(8, 16, 112, 34, 4, SH110X_WHITE);
  ekran.fillRect(8, 16, 112, 10, SH110X_WHITE);
  ekran.setTextColor(SH110X_BLACK);
  ekran.setCursor(22, 17);
  ekran.print("GUN / AY / YIL");
  ekran.setTextColor(SH110X_WHITE);
  if (solTetikSaga()) {
    tarihSeciliAlan = min(tarihSeciliAlan + 1, 2);
    vTaskDelay(pdMS_TO_TICKS(150));
  }
  if (solTetikSola()) {
    tarihSeciliAlan = max(tarihSeciliAlan - 1, 0);
    vTaskDelay(pdMS_TO_TICKS(150));
  }
  if (solTetikYukari()) {
    if (tarihSeciliAlan == 0)
      tarihGun = (tarihGun % 31) + 1;
    else if (tarihSeciliAlan == 1)
      tarihAy = (tarihAy % 12) + 1;
    else
      tarihYil++;
    vTaskDelay(pdMS_TO_TICKS(150));
  }
  if (solTetikAsagi()) {
    if (tarihSeciliAlan == 0)
      tarihGun = (tarihGun > 1) ? tarihGun - 1 : 31;
    else if (tarihSeciliAlan == 1)
      tarihAy = (tarihAy > 1) ? tarihAy - 1 : 12;
    else if (tarihYil > 2020)
      tarihYil--;
    vTaskDelay(pdMS_TO_TICKS(150));
  }
  if (btnKenarBul(BTN_YESIL, 1)) {
    ayarlariKaydet();
    aktifEkran = EKRAN_AYARLAR;
    bipBasari();
  }
  if (btnKenarBul(BTN_KIRMIZI, 0))
    aktifEkran = EKRAN_AYARLAR;
  ekran.setCursor(18, 33);
  ekran.printf("%02d  / %02d / %04d", tarihGun, tarihAy, tarihYil);
  if (tarihSeciliAlan == 0)
    ekran.drawFastHLine(18, 43, 12, SH110X_WHITE);
  else if (tarihSeciliAlan == 1)
    ekran.drawFastHLine(48, 43, 12, SH110X_WHITE);
  else
    ekran.drawFastHLine(72, 43, 24, SH110X_WHITE);
  ekran.setCursor(2, 55);
  ekran.print("Joy:Yon [B]Kyd [A]Cik");
}

void saatAyarCiz() {
  durumCubugu("Saat Ayari");
  ekran.drawRoundRect(20, 16, 88, 34, 4, SH110X_WHITE);
  ekran.fillRect(20, 16, 88, 10, SH110X_WHITE);
  ekran.setTextColor(SH110X_BLACK);
  ekran.setCursor(24, 17);
  ekran.print("SAAT : DAKIKA");
  ekran.setTextColor(SH110X_WHITE);
  if (solTetikSaga()) {
    saatSeciliAlan = 1;
    vTaskDelay(pdMS_TO_TICKS(150));
  }
  if (solTetikSola()) {
    saatSeciliAlan = 0;
    vTaskDelay(pdMS_TO_TICKS(150));
  }
  if (solTetikYukari()) {
    if (saatSeciliAlan == 0)
      saatSaat = (saatSaat + 1) % 24;
    else
      saatDakika = (saatDakika + 1) % 60;
    vTaskDelay(pdMS_TO_TICKS(150));
  }
  if (solTetikAsagi()) {
    if (saatSeciliAlan == 0)
      saatSaat = (saatSaat > 0) ? saatSaat - 1 : 23;
    else
      saatDakika = (saatDakika > 0) ? saatDakika - 1 : 59;
    vTaskDelay(pdMS_TO_TICKS(150));
  }
  if (btnKenarBul(BTN_YESIL, 1)) {
    ayarlariKaydet();
    aktifEkran = EKRAN_AYARLAR;
    bipBasari();
  }
  if (btnKenarBul(BTN_KIRMIZI, 0))
    aktifEkran = EKRAN_AYARLAR;
  ekran.setCursor(35, 33);
  ekran.printf("%02d", saatSaat);
  ekran.setCursor(61, 33);
  ekran.print(":");
  ekran.setCursor(80, 33);
  ekran.printf("%02d", saatDakika);
  if (saatSeciliAlan == 0)
    ekran.drawFastHLine(35, 43, 12, SH110X_WHITE);
  else
    ekran.drawFastHLine(80, 43, 12, SH110X_WHITE);
  ekran.setCursor(2, 55);
  ekran.print("Joy:Yon [B]Kyd [A]Cik");
}

void nrfAyarCiz() {
  durumCubugu("NRF Ayari");
  ekran.drawRoundRect(24, 16, 80, 34, 4, SH110X_WHITE);
  ekran.fillRect(24, 16, 80, 10, SH110X_WHITE);
  ekran.setTextColor(SH110X_BLACK);
  ekran.setCursor(28, 17);
  ekran.print("KANAL AYARI");
  ekran.setTextColor(SH110X_WHITE);
  if (menuGeriBasti())
    nrfKanal = max(nrfKanal - 1, 0);
  if (menuIleriBasti())
    nrfKanal = min((int)nrfKanal + 1, 125);
  ekran.setCursor(44, 33);
  ekran.printf("-> %03d", nrfKanal);
  if (btnKenarBul(BTN_KIRMIZI, 0))
    aktifEkran = EKRAN_AYARLAR;
  altCubukGeri();
}

// Zorluk secim ekranlari
void snakeSecimCiz() {
  durumCubugu("Yilan Modu");
  static int sec = 0;
  if (menuGeriBasti())
    sec = 0;
  if (menuIleriBasti())
    sec = 1;
  ekran.drawRoundRect(10, 16, 108, 36, 4, SH110X_WHITE);
  ekran.fillRect(10, 16, 108, 12, SH110X_WHITE);
  ekran.setTextColor(SH110X_BLACK);
  ekran.setCursor(30, 18);
  ekran.print("ZORLUK SEC");
  ekran.setTextColor(SH110X_WHITE);
  if (sec == 0)
    ekran.fillTriangle(16, 31, 16, 37, 20, 34, SH110X_WHITE);
  ekran.setCursor(24, 31);
  ekran.print("Kolay");
  if (sec == 1)
    ekran.fillTriangle(16, 41, 16, 47, 20, 44, SH110X_WHITE);
  ekran.setCursor(24, 41);
  ekran.print("Zor");
  ekran.drawLine(8, 57, 120, 57, SH110X_WHITE);
  if (btnKenarBul(BTN_YESIL, 1)) {
    snk_mode = sec;
    snk_active = false;
    aktifEkran = EKRAN_GAME_SNAKE;
    bipMenuSec();
  }
  if (btnKenarBul(BTN_KIRMIZI, 0))
    aktifEkran = EKRAN_ANA_MENU;
}

void pongSecimCiz() {
  durumCubugu("Pong Modu");
  static int sec = 0;
  if (menuGeriBasti())
    sec = 0;
  if (menuIleriBasti())
    sec = 1;
  ekran.drawRoundRect(14, 18, 100, 33, 4, SH110X_WHITE);
  ekran.fillRect(14, 18, 100, 10, SH110X_WHITE);
  ekran.setTextColor(SH110X_BLACK);
  ekran.setCursor(26, 20);
  ekran.print("MOD SEC");
  ekran.setTextColor(SH110X_WHITE);
  if (sec == 0)
    ekran.fillTriangle(20, 31, 20, 37, 24, 34, SH110X_WHITE);
  ekran.setCursor(28, 31);
  ekran.print("Insan vs AI");
  if (sec == 1)
    ekran.fillTriangle(20, 41, 20, 47, 24, 44, SH110X_WHITE);
  ekran.setCursor(28, 41);
  ekran.print("Insan vs Insan");
  if (btnKenarBul(BTN_YESIL, 1)) {
    png_mode = sec;
    png_active = false;
    aktifEkran = EKRAN_GAME_PONG;
    bipMenuSec();
  }
  if (btnKenarBul(BTN_KIRMIZI, 0))
    aktifEkran = EKRAN_ANA_MENU;
}

// ========================================================================= //
// ----------------------------- OYUN MOTORLARI ----------------------------- //
// ========================================================================= //

// ===================== GALAGA V2 (5 Can, 10 Seviye) =====================
void runGalaga() {
  if (!gal_act) {
    gal_px = 60;
    gal_fire = false;
    gal_score = 0;
    gal_go = false;
    gal_wave = 1;
    gal_edir = 1;
    gal_can = 5;
    for (int i = 0; i < 5; i++) {
      gal_ex[i] = 15 + (i * 20);
      gal_ey[i] = 18;
      gal_eAlive[i] = true;
    }
    for (int i = 0; i < 15; i++) {
      gal_stars[i].x = random(2, 126);
      gal_stars[i].y = random(13, 62);
      gal_stars[i].speed = random(1, 4) * 0.5;
    }
    gal_act = true;
  }
  ekran.drawRect(0, 0, 128, 64, SH110X_WHITE);
  ekran.drawLine(0, 12, 128, 12, SH110X_WHITE);
  canCiz(gal_can, 5);
  ekran.setCursor(70, 3);
  ekran.printf("Svy>%d", gal_wave);
  ekran.setCursor(105, 3);
  ekran.print(gal_score);
  if (btnKenarBul(BTN_KIRMIZI, 0)) {
    gal_act = false;
    aktifEkran = EKRAN_ANA_MENU;
    return;
  }
  if (gal_go) {
    popupGoster("OYUN BITTI", "[B] Yeni Oyun");
    bipOyunBitti();
    if (popupYenidenBaslat()) {
      gal_act = false;
    }
    return;
  }

  for (int i = 0; i < 15; i++) {
    ekran.drawPixel(gal_stars[i].x, gal_stars[i].y, SH110X_WHITE);
    gal_stars[i].y += gal_stars[i].speed;
    if (gal_stars[i].y > 62) {
      gal_stars[i].y = 13;
      gal_stars[i].x = random(2, 126);
    }
  }
  if (solTetikSola() && gal_px > 5)
    gal_px -= 3;
  if (solTetikSaga() && gal_px < 115)
    gal_px += 3;
  if ((btnKenarBul(BTN_YESIL, 1) || btnKenarBul(SOL_JOY_BTN, 4)) && !gal_fire) {
    gal_fire = true;
    gal_bx = gal_px + 3;
    gal_by = 55;
    bipAtes();
  }
  if (gal_fire) {
    gal_by -= 4;
    ekran.drawLine(gal_bx, gal_by, gal_bx, gal_by + 4, SH110X_WHITE);
    if (gal_by < 13)
      gal_fire = false;
  }

  bool allDead = true;
  int enemySpeed = min(gal_wave, 5); // Seviye arttikca hizlaniyor
  for (int i = 0; i < 5; i++) {
    if (gal_eAlive[i]) {
      allDead = false;
      // Aktif dusman hareketi - seviyeye gore hizlaniyor
      if (millis() % (max(20 - gal_wave * 2, 5)) == 0)
        gal_ex[i] += gal_edir * enemySpeed;
      // Dusmanlar asagi iniyor (seviyeye gore)
      if (gal_wave > 3 && random(0, 200) == 0)
        gal_ey[i]++;
      ekran.drawBitmap(gal_ex[i], gal_ey[i], gal_enemy1_sprite, 8, 8,
                       SH110X_WHITE);
      if (gal_fire && gal_bx >= gal_ex[i] && gal_bx <= gal_ex[i] + 8 &&
          gal_by <= gal_ey[i] + 8) {
        gal_eAlive[i] = false;
        gal_fire = false;
        gal_score += 50;
        bipPatlama();
        ekran.fillCircle(gal_ex[i] + 4, gal_ey[i] + 4, 3, SH110X_INVERSE);
      }
      if (gal_ey[i] + 8 >= 55 && gal_ex[i] + 8 >= gal_px &&
          gal_ex[i] <= gal_px + 8) {
        gal_can--;
        bipCanKaybi();
        if (gal_can <= 0)
          gal_go = true;
        else {
          gal_eAlive[i] = false;
          gal_px = 60;
        }
      }
    }
  }
  if (allDead) {
    gal_wave++;
    if (gal_wave > 10) {
      popupGoster("TEBRIKLER!", "Tum Seviyeler Bitti!");
      bipSeviyeAtla();
      gal_go = true;
      return;
    }
    bipSeviyeAtla();
    for (int i = 0; i < 5; i++) {
      gal_ey[i] = 18 + min(gal_wave, 6) * 3;
      gal_ex[i] = 10 + (i * 22);
      gal_eAlive[i] = true;
    }
    popupGoster("DALGA", "Yeni Seviye!");
    ekran.display();
    vTaskDelay(pdMS_TO_TICKS(800));
  }
  if (millis() % 500 < 10)
    gal_edir = -gal_edir;
  ekran.drawBitmap(gal_px, 55, gal_player_sprite, 8, 8, SH110X_WHITE);
}

// ===================== AYA İNİŞ =====================
void runLander() {
  if (!lnd_act) {
    lnd_x = 60;
    lnd_y = 15;
    lnd_vy = 0;
    lnd_vx = 0;
    lnd_fuel = 100;
    lnd_go = false;
    lnd_win = false;
    lnd_isThrusting = false;
    for (int i = 0; i < 10; i++)
      lnd_particles[i].life = 0;
    lnd_act = true;
  }
  drawGameUI("Aya Inis V2", String((int)lnd_fuel));
  if (btnKenarBul(BTN_KIRMIZI, 0)) {
    lnd_act = false;
    aktifEkran = EKRAN_ANA_MENU;
    return;
  }
  if (lnd_go || lnd_win) {
    if (lnd_go) {
      popupGoster("CAKILDIN!", "Hiz Cok Yuksekti");
      bipOyunBitti();
    }
    if (lnd_win) {
      popupGoster("MUKEMMEL!", "Sakin Inis Basarili");
      bipInis();
    }
    if (popupYenidenBaslat())
      lnd_act = false;
    return;
  }
  if (!lnd_go && !lnd_win) {
    lnd_isThrusting = (guncelPaket.buton2 || guncelPaket.solJoyButon);
    lnd_vy += 0.04;
    if (solTetikSola() && lnd_fuel > 0) {
      lnd_vx -= 0.05;
      lnd_fuel -= 0.1;
    }
    if (solTetikSaga() && lnd_fuel > 0) {
      lnd_vx += 0.05;
      lnd_fuel -= 0.1;
    }
    if (lnd_isThrusting && lnd_fuel > 0) {
      lnd_vy -= 0.15;
      lnd_fuel -= 0.4;
      bipItki();
    }
    lnd_vx = constrain(lnd_vx, -1.5, 1.5);
    lnd_vy = constrain(lnd_vy, -2.0, 2.0);
    lnd_x += lnd_vx;
    lnd_y += lnd_vy;
    if (lnd_x < 2) {
      lnd_x = 2;
      lnd_vx = 0;
    }
    if (lnd_x > 116) {
      lnd_x = 116;
      lnd_vx = 0;
    }
    if (lnd_y < 13) {
      lnd_y = 13;
      lnd_vy = 0;
    }
    if (lnd_y > 51) {
      lnd_y = 51;
      if (lnd_x >= 50 && lnd_x <= 72 && lnd_vy < 1.0 && abs(lnd_vx) < 0.5)
        lnd_win = true;
      else
        lnd_go = true;
      lnd_vx = 0;
      lnd_vy = 0;
    }
    if (lnd_isThrusting && lnd_fuel > 0) {
      for (int i = 0; i < 3; i++) {
        int p;
        for (p = 0; p < 10; p++)
          if (lnd_particles[p].life <= 0)
            break;
        if (p < 10) {
          lnd_particles[p].x = lnd_x + 5 + random(-2, 3);
          lnd_particles[p].y = lnd_y + 10;
          lnd_particles[p].vx = lnd_vx + (random(-5, 6) * 0.1);
          lnd_particles[p].vy = 2 + random(1, 3);
          lnd_particles[p].life = random(5, 12);
        }
      }
    }
    for (int i = 0; i < 10; i++) {
      if (lnd_particles[i].life > 0) {
        lnd_particles[i].x += lnd_particles[i].vx;
        lnd_particles[i].y += lnd_particles[i].vy;
        lnd_particles[i].life--;
      }
    }
  }
  ekran.drawLine(0, 60, 20, 56, SH110X_WHITE);
  ekran.drawLine(20, 56, 40, 62, SH110X_WHITE);
  ekran.drawLine(40, 62, 50, 60, SH110X_WHITE);
  ekran.fillRect(50, 59, 28, 5, SH110X_WHITE);
  ekran.drawRect(50, 59, 28, 5, SH110X_BLACK);
  ekran.drawLine(78, 60, 90, 55, SH110X_WHITE);
  ekran.drawLine(90, 55, 110, 63, SH110X_WHITE);
  ekran.drawLine(110, 63, 128, 58, SH110X_WHITE);
  for (int i = 0; i < 10; i++) {
    if (lnd_particles[i].life > 0)
      ekran.drawPixel(lnd_particles[i].x, lnd_particles[i].y, SH110X_WHITE);
  }
  ekran.drawBitmap((int)lnd_x, (int)lnd_y, lnd_lander_sprite, 10, 10,
                   SH110X_WHITE);
  ekran.drawRect(2, 14, 22, 5, SH110X_WHITE);
  ekran.fillRect(3, 15, (int)(lnd_fuel / 5), 3, SH110X_WHITE);
  ekran.setCursor(100, 14);
  if (lnd_vy > 1.0)
    ekran.print("!");
  ekran.print("V:");
  ekran.print(lnd_vy, 1);
}

// ===================== FLAPPY UFO (5 Can) =====================
void runFlappy() {
  if (!flp_act) {
    flp_y = 30;
    flp_vy = 0;
    flp_px = 128;
    flp_gapY = random(25, 42);
    flp_score = 0;
    flp_go = false;
    flp_act = true;
    flp_can = 5;
    for (int i = 0; i < 5; i++)
      flp_particles[i].life = 0;
  }
  ekran.drawRect(0, 0, 128, 64, SH110X_WHITE);
  ekran.drawLine(0, 12, 128, 12, SH110X_WHITE);
  canCiz(flp_can, 5);
  ekran.setCursor(80, 3);
  ekran.print("Flappy ");
  ekran.print(flp_score);
  if (btnKenarBul(BTN_KIRMIZI, 0)) {
    flp_act = false;
    aktifEkran = EKRAN_ANA_MENU;
    return;
  }
  if (flp_go) {
    popupGoster("OYUN BITTI", "[B] Yeni Oyun");
    bipOyunBitti();
    if (popupYenidenBaslat())
      flp_act = false;
    return;
  }
  if (btnKenarBul(BTN_YESIL, 1) || btnKenarBul(SOL_JOY_BTN, 4)) {
    flp_vy = -3.5;
    bipZipla();
    for (int i = 0; i < 5; i++) {
      flp_particles[i].x = 24 + random(-2, 3);
      flp_particles[i].y = flp_y + 8;
      flp_particles[i].vx = random(-1, 1) * 0.5;
      flp_particles[i].vy = random(1, 3);
      flp_particles[i].life = random(3, 8);
    }
  }
  flp_vy += 0.35;
  flp_y += flp_vy;
  flp_px -= 2;
  if (flp_px < -16) {
    flp_px = 128;
    flp_gapY = random(25, 42);
    flp_score++;
  }
  bool hit = false;
  if (flp_y < 13 || flp_y > 58)
    hit = true;
  int ur = 34, ul = 22, pr = flp_px + 16;
  if (ur > flp_px && ul < pr) {
    if (flp_y + 1 < flp_gapY || flp_y + 7 > flp_gapY + 20)
      hit = true;
  }
  if (hit) {
    flp_can--;
    bipCanKaybi();
    if (flp_can <= 0)
      flp_go = true;
    else {
      flp_y = 30;
      flp_vy = 0;
      flp_px = 128;
      flp_gapY = random(25, 42);
    }
  }
  ekran.drawRect(flp_px + 2, 13, 12, flp_gapY - 13 - 4, SH110X_WHITE);
  ekran.fillRect(flp_px, flp_gapY - 4, 16, 4, SH110X_WHITE);
  ekran.fillRect(flp_px, flp_gapY + 20, 16, 4, SH110X_WHITE);
  ekran.drawRect(flp_px + 2, flp_gapY + 24, 12, 64 - (flp_gapY + 24),
                 SH110X_WHITE);
  for (int i = 0; i < 5; i++) {
    if (flp_particles[i].life > 0) {
      ekran.drawPixel((int)flp_particles[i].x, (int)flp_particles[i].y,
                      SH110X_WHITE);
      flp_particles[i].x += flp_particles[i].vx;
      flp_particles[i].y += flp_particles[i].vy;
      flp_particles[i].life--;
    }
  }
  ekran.drawBitmap(20, (int)flp_y, flp_ufo_sprite, 16, 8, SH110X_WHITE);
}

// ===================== T-REX KAÇIŞ(5 Can, Zorluk Artiyor)=====================
void runTRexGame() {
  if (!rex_act) {
    rex_y = 44;
    rex_vy = 0;
    rex_obsX = 128;
    rex_obsW = 10;
    rex_score = 0;
    rex_act = true;
    rex_go = false;
    rex_can = 5;
    rex_speed = 2;
  }
  ekran.drawRect(0, 0, 128, 64, SH110X_WHITE);
  ekran.drawLine(0, 12, 128, 12, SH110X_WHITE);
  canCiz(rex_can, 5);
  ekran.setCursor(80, 3);
  ekran.print("Kacis ");
  ekran.print(rex_score);
  if (rex_go) {
    popupGoster("YAKALANDIN!", "[B] Yeni Oyun");
    bipOyunBitti();
    if (popupYenidenBaslat()) {
      rex_act = false;
      aktifEkran = EKRAN_ANA_MENU;
    }
    return;
  }
  if (btnKenarBul(BTN_KIRMIZI, 0)) {
    rex_act = false;
    aktifEkran = EKRAN_ANA_MENU;
    return;
  }
  if ((btnKenarBul(BTN_YESIL, 1) || solTetikYukari()) && rex_y >= 44) {
    rex_vy = -7;
    bipZipla();
  }
  rex_vy += 1;
  rex_y += rex_vy;
  if (rex_y > 44) {
    rex_y = 44;
    rex_vy = 0;
  }
  // Zorluk her 20 puanda artar
  rex_speed = 2 + (rex_score / 20);
  rex_obsX -= rex_speed;
  if (rex_obsX < -10) {
    rex_obsX = 128;
    rex_score++;
    rex_obsW = random(8, 14);
  }
  if (rex_obsX < 26 && rex_obsX + 4 > 22 && rex_y + 12 > 56 - rex_obsW) {
    rex_can--;
    bipCanKaybi();
    if (rex_can <= 0)
      rex_go = true;
    else {
      rex_obsX = 128;
      rex_y = 44;
      rex_vy = 0;
    }
  }
  ekran.drawLine(0, 56, 128, 56, SH110X_WHITE);
  if (millis() % 200 < 100) {
    ekran.drawPixel(40, 58, SH110X_WHITE);
    ekran.drawPixel(90, 59, SH110X_WHITE);
  } else {
    ekran.drawPixel(38, 58, SH110X_WHITE);
    ekran.drawPixel(88, 59, SH110X_WHITE);
  }
  int c_y = 56 - rex_obsW;
  ekran.drawLine(rex_obsX + 2, c_y, rex_obsX + 2, 56, SH110X_WHITE);
  ekran.drawLine(rex_obsX, c_y + 3, rex_obsX + 1, c_y + 3, SH110X_WHITE);
  ekran.drawLine(rex_obsX + 3, c_y + 4, rex_obsX + 4, c_y + 4, SH110X_WHITE);
  int d_x = 20;
  ekran.fillRect(d_x + 4, rex_y, 5, 4, SH110X_WHITE);
  ekran.fillRect(d_x + 2, rex_y + 4, 5, 6, SH110X_WHITE);
  ekran.drawLine(d_x, rex_y + 6, d_x + 1, rex_y + 8, SH110X_WHITE);
  if (rex_y < 44) {
    ekran.drawLine(d_x + 2, rex_y + 10, d_x + 3, rex_y + 11, SH110X_WHITE);
    ekran.drawLine(d_x + 5, rex_y + 10, d_x + 6, rex_y + 11, SH110X_WHITE);
  } else {
    if (millis() % 200 < 100) {
      ekran.drawLine(d_x + 2, rex_y + 10, d_x + 2, rex_y + 11, SH110X_WHITE);
      ekran.drawLine(d_x + 5, rex_y + 10, d_x + 6, rex_y + 10, SH110X_WHITE);
    } else {
      ekran.drawLine(d_x + 2, rex_y + 10, d_x + 1, rex_y + 10, SH110X_WHITE);
      ekran.drawLine(d_x + 5, rex_y + 10, d_x + 5, rex_y + 11, SH110X_WHITE);
    }
  }
}

// ===================== ARKANOID (5 Can, Popup) =====================
void runArkanoid() {
  if (!ark_act) {
    ark_px = 50;
    ark_bx = 64;
    ark_by = 40;
    ark_bvx = 2;
    ark_bvy = -2;
    ark_score = 0;
    ark_go = false;
    ark_can = 5;
    ark_level = 1;
    for (int r = 0; r < 3; r++)
      for (int c = 0; c < 8; c++)
        ark_br[r][c] = 1;
    ark_act = true;
  }
  ekran.drawRect(0, 0, 128, 64, SH110X_WHITE);
  ekran.drawLine(0, 12, 128, 12, SH110X_WHITE);
  canCiz(ark_can, 5);
  ekran.setCursor(50, 3);
  ekran.printf("Svy:%d", ark_level);
  ekran.setCursor(90, 3);
  ekran.print(ark_score);
  if (btnKenarBul(BTN_KIRMIZI, 0)) {
    ark_act = false;
    aktifEkran = EKRAN_ANA_MENU;
    return;
  }
  if (!ark_go) {
    if (solTetikSola() && ark_px > 2)
      ark_px -= 3;
    if (solTetikSaga() && ark_px < 106)
      ark_px += 3;
    ark_bx += ark_bvx;
    ark_by += ark_bvy;
    if (ark_bx < 2 || ark_bx > 124)
      ark_bvx = -ark_bvx;
    if (ark_by < 14)
      ark_bvy = -ark_bvy;
    if (ark_by + 2 >= 58 && ark_bx >= ark_px && ark_bx <= ark_px + 20) {
      ark_bvy = -ark_bvy;
      ark_by = 55;
      bipTek();
    }
    if (ark_by > 62) {
      ark_can--;
      bipCanKaybi();
      if (ark_can <= 0)
        ark_go = true;
      else {
        ark_bx = 64;
        ark_by = 40;
        ark_bvx = (ark_bvx > 0 ? 2 : -2);
        ark_bvy = -2;
      }
    }
    bool allBroken = true;
    for (int r = 0; r < 3; r++) {
      for (int c = 0; c < 8; c++) {
        if (ark_br[r][c] == 1) {
          allBroken = false;
          int tx = 5 + (c * 15);
          int ty = 18 + (r * 8);
          if (ark_bx + 2 >= tx && ark_bx <= tx + 14 && ark_by + 2 >= ty &&
              ark_by <= ty + 6) {
            ark_br[r][c] = 0;
            ark_bvy = -ark_bvy;
            ark_score += 10;
            bipPatlama();
          }
        }
      }
    }
    // Tum tuglalar kirildiysa seviye atla
    if (allBroken) {
      ark_level++;
      bipSeviyeAtla();
      for (int r = 0; r < 3; r++)
        for (int c = 0; c < 8; c++)
          ark_br[r][c] = 1;
      ark_bx = 64;
      ark_by = 40;
      int spd = min(2 + ark_level / 3, 5);
      ark_bvx = (ark_bvx > 0 ? spd : -spd);
      ark_bvy = -spd;
      char buf[20];
      snprintf(buf, 20, "Seviye %d", ark_level);
      popupGoster("SEVIYE ATLANDI", buf);
      ekran.display();
      vTaskDelay(pdMS_TO_TICKS(800));
    }
  }
  ekran.fillRect(ark_px, 58, 20, 3, SH110X_WHITE);
  ekran.fillRect(ark_bx, ark_by, 2, 2, SH110X_WHITE);
  for (int r = 0; r < 3; r++)
    for (int c = 0; c < 8; c++)
      if (ark_br[r][c] == 1)
        ekran.drawRect(5 + (c * 15), 18 + (r * 8), 14, 6, SH110X_WHITE);
  if (ark_go) {
    popupGoster("OYUN BITTI", "[B] Yeni Oyun");
    bipOyunBitti();
    if (popupYenidenBaslat())
      ark_act = false;
  }
}

// ===================== 2048 (Popup + Restart) =====================
void run2048() {
  auto addR = []() {
    int e[16][2], c = 0;
    for (int r = 0; r < 4; r++)
      for (int cl = 0; cl < 4; cl++)
        if (g2048_board[r][cl] == 0) {
          e[c][0] = r;
          e[c][1] = cl;
          c++;
        }
    if (c > 0) {
      int rnd = random(0, c);
      g2048_board[e[rnd][0]][e[rnd][1]] = (random(0, 10) < 9) ? 2 : 4;
    }
  };
  if (!g2048_act) {
    for (int r = 0; r < 4; r++)
      for (int c = 0; c < 4; c++)
        g2048_board[r][c] = 0;
    g2048_score = 0;
    g2048_act = true;
    g2048_go = false;
    addR();
    addR();
  }
  drawGameUI("2048", String(g2048_score));
  if (btnKenarBul(BTN_KIRMIZI, 0)) {
    g2048_act = false;
    aktifEkran = EKRAN_ANA_MENU;
    return;
  }
  // Game over kontrolu
  if (!g2048_go) {
    bool canMove = false;
    for (int r = 0; r < 4; r++)
      for (int c = 0; c < 4; c++) {
        if (g2048_board[r][c] == 0)
          canMove = true;
        if (c < 3 && g2048_board[r][c] == g2048_board[r][c + 1])
          canMove = true;
        if (r < 3 && g2048_board[r][c] == g2048_board[r + 1][c])
          canMove = true;
      }
    if (!canMove)
      g2048_go = true;
  }
  if (g2048_go) {
    popupGoster("OYUN BITTI", "[B] Yeni Oyun");
    bipOyunBitti();
    if (popupYenidenBaslat())
      g2048_act = false;
    return;
  }
  if (solTetikSola() || solTetikSaga() || solTetikYukari() || solTetikAsagi()) {
    bool moved = false;
    for (int r = 0; r < 4; r++) {
      if (solTetikSola()) {
        int wc = 0;
        for (int c = 0; c < 4; c++)
          if (g2048_board[r][c] != 0) {
            if (wc != c) {
              g2048_board[r][wc] = g2048_board[r][c];
              g2048_board[r][c] = 0;
              moved = true;
            }
            wc++;
          }
        for (int c = 0; c < 3; c++)
          if (g2048_board[r][c] != 0 &&
              g2048_board[r][c] == g2048_board[r][c + 1]) {
            g2048_board[r][c] *= 2;
            g2048_score += g2048_board[r][c];
            g2048_board[r][c + 1] = 0;
            moved = true;
          }
        wc = 0;
        for (int c = 0; c < 4; c++)
          if (g2048_board[r][c] != 0) {
            if (wc != c) {
              g2048_board[r][wc] = g2048_board[r][c];
              g2048_board[r][c] = 0;
            }
            wc++;
          }
      }
      if (solTetikSaga()) {
        int wc = 3;
        for (int c = 3; c >= 0; c--)
          if (g2048_board[r][c] != 0) {
            if (wc != c) {
              g2048_board[r][wc] = g2048_board[r][c];
              g2048_board[r][c] = 0;
              moved = true;
            }
            wc--;
          }
        for (int c = 3; c > 0; c--)
          if (g2048_board[r][c] != 0 &&
              g2048_board[r][c] == g2048_board[r][c - 1]) {
            g2048_board[r][c] *= 2;
            g2048_score += g2048_board[r][c];
            g2048_board[r][c - 1] = 0;
            moved = true;
          }
        wc = 3;
        for (int c = 3; c >= 0; c--)
          if (g2048_board[r][c] != 0) {
            if (wc != c) {
              g2048_board[r][wc] = g2048_board[r][c];
              g2048_board[r][c] = 0;
            }
            wc--;
          }
      }
    }
    for (int c = 0; c < 4; c++) {
      if (solTetikYukari()) {
        int wr = 0;
        for (int r = 0; r < 4; r++)
          if (g2048_board[r][c] != 0) {
            if (wr != r) {
              g2048_board[wr][c] = g2048_board[r][c];
              g2048_board[r][c] = 0;
              moved = true;
            }
            wr++;
          }
        for (int r = 0; r < 3; r++)
          if (g2048_board[r][c] != 0 &&
              g2048_board[r][c] == g2048_board[r + 1][c]) {
            g2048_board[r][c] *= 2;
            g2048_score += g2048_board[r][c];
            g2048_board[r + 1][c] = 0;
            moved = true;
          }
        wr = 0;
        for (int r = 0; r < 4; r++)
          if (g2048_board[r][c] != 0) {
            if (wr != r) {
              g2048_board[wr][c] = g2048_board[r][c];
              g2048_board[r][c] = 0;
            }
            wr++;
          }
      }
      if (solTetikAsagi()) {
        int wr = 3;
        for (int r = 3; r >= 0; r--)
          if (g2048_board[r][c] != 0) {
            if (wr != r) {
              g2048_board[wr][c] = g2048_board[r][c];
              g2048_board[r][c] = 0;
              moved = true;
            }
            wr--;
          }
        for (int r = 3; r > 0; r--)
          if (g2048_board[r][c] != 0 &&
              g2048_board[r][c] == g2048_board[r - 1][c]) {
            g2048_board[r][c] *= 2;
            g2048_score += g2048_board[r][c];
            g2048_board[r - 1][c] = 0;
            moved = true;
          }
        wr = 3;
        for (int r = 3; r >= 0; r--)
          if (g2048_board[r][c] != 0) {
            if (wr != r) {
              g2048_board[wr][c] = g2048_board[r][c];
              g2048_board[r][c] = 0;
            }
            wr--;
          }
      }
    }
    if (moved) {
      addR();
      bipMenuSec();
      vTaskDelay(pdMS_TO_TICKS(150));
    }
  }
  int oX = 40, oY = 14, bS = 12;
  for (int r = 0; r < 4; r++)
    for (int c = 0; c < 4; c++) {
      int px = oX + (c * bS);
      int py = oY + (r * bS);
      ekran.drawRoundRect(px, py, bS - 1, bS - 1, 2, SH110X_WHITE);
      if (g2048_board[r][c] > 0) {
        if (g2048_board[r][c] < 10) {
          ekran.setCursor(px + 3, py + 2);
          ekran.print(g2048_board[r][c]);
        } else if (g2048_board[r][c] < 100) {
          ekran.setCursor(px + 1, py + 2);
          ekran.print(g2048_board[r][c]);
        } else if (g2048_board[r][c] < 1000) {
          ekran.setCursor(px - 1, py + 2);
          ekran.print(g2048_board[r][c]);
        } else {
          ekran.setCursor(px, py + 2);
          ekran.print("K");
        }
      }
    }
}

// ===================== LABiRENT (100 Seviye, 7sn, 3 Can) =====================
void runMaze() {
  if (!maze_act) {
    maze_px = 0;
    maze_py = 1;
    maze_level = 0;
    maze_win = false;
    maze_can = 3;
    maze_act = true;
    mazeGenerate(0);
    maze_startTime = millis();
  }
  // UI - ust bar
  ekran.drawRect(0, 0, 128, 64, SH110X_WHITE);
  ekran.drawLine(0, 12, 128, 12, SH110X_WHITE);
  canCiz(maze_can, 3);
  // Sure gostergesi
  int kalanSn = 7 - (int)((millis() - maze_startTime) / 1000);
  if (kalanSn < 0)
    kalanSn = 0;
  ekran.setCursor(42, 3);
  ekran.printf("Svy:%d", maze_level + 1);
  ekran.setCursor(95, 3);
  ekran.printf("%ds", kalanSn);
  if (btnKenarBul(BTN_KIRMIZI, 0)) {
    maze_act = false;
    aktifEkran = EKRAN_ANA_MENU;
    return;
  }
  if (!maze_win) {
    // Sure doldu
    if (kalanSn <= 0) {
      maze_can--;
      bipCanKaybi();
      if (maze_can <= 0) {
        maze_win = true;
      } // oyun bitti
      else {
        maze_px = 0;
        maze_py = 1;
        maze_startTime = millis();
      } // yeniden basla ayni seviye
    }
    if (maze_py > 0 && solTetikYukari() &&
        maze_grid[maze_py - 1][maze_px] != 1) {
      maze_py--;
      vTaskDelay(pdMS_TO_TICKS(120));
    }
    if (maze_py < 7 && solTetikAsagi() &&
        maze_grid[maze_py + 1][maze_px] != 1) {
      maze_py++;
      vTaskDelay(pdMS_TO_TICKS(120));
    }
    if (maze_px > 0 && solTetikSola() && maze_grid[maze_py][maze_px - 1] != 1) {
      maze_px--;
      vTaskDelay(pdMS_TO_TICKS(120));
    }
    if (maze_px < 20 && solTetikSaga() &&
        maze_grid[maze_py][maze_px + 1] != 1) {
      maze_px++;
      vTaskDelay(pdMS_TO_TICKS(120));
    }
    if (maze_px == 20 && maze_py == 6) {
      if (maze_level < 99) {
        maze_level++;
        maze_px = 0;
        maze_py = 1;
        bipSeviyeAtla();
        // Her 3 seviyede can yenile
        if (maze_level % 3 == 0) {
          maze_can = 3;
          bipBasari();
        }
        mazeGenerate(maze_level);
        maze_startTime = millis();
        char svyBuf[20];
        snprintf(svyBuf, 20, "Svy> %d", maze_level + 1);
        popupGoster("SEVIYE ATLANDI", svyBuf);
        ekran.display();
        vTaskDelay(pdMS_TO_TICKS(800));
      } else {
        maze_win = true;
      }
    }
  }
  int sx = 2, sy = 14, cs = 6;
  for (int r = 0; r < 8; r++)
    for (int c = 0; c < 21; c++) {
      int x = sx + c * cs;
      int y = sy + r * cs;
      if (maze_grid[r][c] == 1)
        ekran.drawRect(x, y, cs, cs, SH110X_WHITE);
    }
  if (millis() % 400 < 200)
    ekran.fillCircle(sx + 20 * cs + 3, sy + 6 * cs + 3, 2, SH110X_WHITE);
  ekran.drawRect(sx + maze_px * cs + 1, sy + maze_py * cs + 1, cs - 2, cs - 2,
                 SH110X_WHITE);
  ekran.drawPixel(sx + maze_px * cs + 3, sy + maze_py * cs + 3, SH110X_WHITE);
  if (maze_win && maze_can <= 0) {
    popupGoster("OYUN BITTI", "[B] Yeni Oyun");
    bipOyunBitti();
    if (popupYenidenBaslat())
      maze_act = false;
  }
  if (maze_win && maze_can > 0) {
    popupGoster("100 SEVIYE TAMAM!", "Tebrikler!");
    bipSeviyeAtla();
    if (popupYenidenBaslat())
      maze_act = false;
  }
}

// ===================== PACMAN (Standart Kurallar) =====================
void runPacman() {
  if (!pac_act) {
    pac_score = 0;
    pac_dots = 0;
    pac_x = 10;
    pac_y = 4;
    pac_dir = 2;
    pac_nextDir = 2;
    gh_x = 10;
    gh_y = 3;
    gh_dir = 0;
    pac_go = false;
    pac_win = false;
    pac_act = true;
    pac_can = 5;
    for (int r = 0; r < 8; r++)
      for (int c = 0; c < 21; c++) {
        pac_map[r][c] = pac_layout[r][c];
        if (pac_map[r][c] == 2)
          pac_dots++;
      }
  }
  drawGameUI("Pacman", String(pac_score));
  if (btnKenarBul(BTN_KIRMIZI, 0)) {
    pac_act = false;
    aktifEkran = EKRAN_ANA_MENU;
    return;
  }
  if (!pac_go && !pac_win) {
    if (solTetikYukari())
      pac_nextDir = 0;
    else if (solTetikSaga())
      pac_nextDir = 1;
    else if (solTetikAsagi())
      pac_nextDir = 2;
    else if (solTetikSola())
      pac_nextDir = 3;
    if (millis() - pac_lastMove > 150) {
      pac_lastMove = millis();
      int nx = pac_x, ny = pac_y;
      if (pac_nextDir == 0)
        ny--;
      else if (pac_nextDir == 1)
        nx++;
      else if (pac_nextDir == 2)
        ny++;
      else if (pac_nextDir == 3)
        nx--;
      if (ny >= 0 && ny < 8 && nx >= 0 && nx < 21 && pac_map[ny][nx] != 1)
        pac_dir = pac_nextDir;
      if (pac_dir == 0 && pac_y > 0 && pac_map[pac_y - 1][pac_x] != 1)
        pac_y--;
      else if (pac_dir == 1 && pac_x < 20 && pac_map[pac_y][pac_x + 1] != 1)
        pac_x++;
      else if (pac_dir == 2 && pac_y < 7 && pac_map[pac_y + 1][pac_x] != 1)
        pac_y++;
      else if (pac_dir == 3 && pac_x > 0 && pac_map[pac_y][pac_x - 1] != 1)
        pac_x--;
      if (pac_map[pac_y][pac_x] == 2) {
        pac_map[pac_y][pac_x] = 0;
        pac_score += 10;
        pac_dots--;
        bipYem();
        if (pac_dots == 0)
          pac_win = true;
      }
      // Hayalet AI
      int gnx = gh_x, gny = gh_y;
      if (gh_dir == 0)
        gny--;
      else if (gh_dir == 1)
        gnx++;
      else if (gh_dir == 2)
        gny++;
      else if (gh_dir == 3)
        gnx--;
      if (gny < 0 || gny >= 8 || gnx < 0 || gnx >= 21 ||
          pac_map[gny][gnx] == 1 || random(0, 5) == 0) {
        gh_dir = random(0, 4);
      } else {
        gh_x = gnx;
        gh_y = gny;
      }
      if (pac_x == gh_x && pac_y == gh_y) {
        pac_can--;
        bipCanKaybi();
        if (pac_can <= 0)
          pac_go = true;
        else {
          pac_x = 10;
          pac_y = 4;
        }
      }
    }
  }
  int sx = 2, sy = 14, cs = 6;
  for (int r = 0; r < 8; r++)
    for (int c = 0; c < 21; c++) {
      int x = sx + c * cs;
      int y = sy + r * cs;
      if (pac_map[r][c] == 1)
        ekran.drawRect(x, y, cs, cs, SH110X_WHITE);
      else if (pac_map[r][c] == 2)
        ekran.drawPixel(x + 3, y + 3, SH110X_WHITE);
    }
  ekran.fillRoundRect(sx + gh_x * cs + 1, sy + gh_y * cs + 1, 4, 4, 1,
                      SH110X_WHITE);
  int px = sx + pac_x * cs + 3;
  int py = sy + pac_y * cs + 3;
  ekran.fillCircle(px, py, 2, SH110X_WHITE);
  if (millis() % 300 < 150 && !pac_go) {
    if (pac_dir == 0)
      ekran.fillTriangle(px, py, px - 3, py - 3, px + 3, py - 3, SH110X_BLACK);
    if (pac_dir == 1)
      ekran.fillTriangle(px, py, px + 3, py - 3, px + 3, py + 3, SH110X_BLACK);
    if (pac_dir == 2)
      ekran.fillTriangle(px, py, px - 3, py + 3, px + 3, py + 3, SH110X_BLACK);
    if (pac_dir == 3)
      ekran.fillTriangle(px, py, px - 3, py - 3, px - 3, py + 3, SH110X_BLACK);
  }
  if (pac_go) {
    popupGoster("YAKALANDIN!", "[B] Yeni Oyun");
    bipOyunBitti();
    if (popupYenidenBaslat())
      pac_act = false;
  }
  if (pac_win) {
    popupGoster("TEBRIKLER!", "Hepsini Temizledin!");
    bipSeviyeAtla();
    if (popupYenidenBaslat())
      pac_act = false;
  }
}

// ===================== YILAN (2 Zorluk, 5 Can, Hızlanma) =====================
void runSnakeGame() {
  if (!snk_active) {
    snk_length = 4;
    snk_dir = 1;
    for (int i = 0; i < 4; i++) {
      snk_x[i] = 15 - i;
      snk_y[i] = 5;
    }
    snk_score = 0;
    snk_gameOver = false;
    snk_foodX = random(1, 29);
    snk_foodY = random(1, 11);
    snk_active = true;
    snk_can = 5;
    snk_speed = 100;
  }
  ekran.drawRect(0, 0, 128, 64, SH110X_WHITE);
  ekran.drawLine(0, 12, 128, 12, SH110X_WHITE);
  canCiz(snk_can, 5);
  ekran.setCursor(50, 3);
  ekran.print("puan:");
  ekran.print(snk_score);
  if (snk_gameOver) {
    popupGoster("OYUN BITTI", "[B] Yeni Oyun");
    bipOyunBitti();
    if (popupYenidenBaslat()) {
      snk_active = false;
      aktifEkran = EKRAN_ANA_MENU;
    }
    return;
  }
  if (btnKenarBul(BTN_KIRMIZI, 0)) {
    snk_active = false;
    aktifEkran = EKRAN_ANA_MENU;
    return;
  }
  if (solTetikYukari() && snk_dir != 2)
    snk_dir = 0;
  else if (solTetikAsagi() && snk_dir != 0)
    snk_dir = 2;
  else if (solTetikSola() && snk_dir != 1)
    snk_dir = 3;
  else if (solTetikSaga() && snk_dir != 3)
    snk_dir = 1;
  if (millis() - snk_lastMoveTime > snk_speed) {
    snk_lastMoveTime = millis();
    for (int i = snk_length - 1; i > 0; i--) {
      snk_x[i] = snk_x[i - 1];
      snk_y[i] = snk_y[i - 1];
    }
    if (snk_dir == 0)
      snk_y[0]--;
    else if (snk_dir == 1)
      snk_x[0]++;
    else if (snk_dir == 2)
      snk_y[0]++;
    else if (snk_dir == 3)
      snk_x[0]--;
    // Mod kontrolu: Kolay = duvar gecis, Zor = olum
    if (snk_mode == 0) {
      if (snk_x[0] < 0)
        snk_x[0] = 30;
      if (snk_x[0] > 30)
        snk_x[0] = 0;
      if (snk_y[0] < 0)
        snk_y[0] = 11;
      if (snk_y[0] > 11)
        snk_y[0] = 0;
    } else {
      if (snk_x[0] < 0 || snk_x[0] > 30 || snk_y[0] < 0 || snk_y[0] > 11) {
        snk_can--;
        bipCanKaybi();
        if (snk_can <= 0)
          snk_gameOver = true;
        else {
          snk_length = 4;
          snk_dir = 1;
          for (int i = 0; i < 4; i++) {
            snk_x[i] = 15 - i;
            snk_y[i] = 5;
          }
        }
      }
    }
    for (int i = 1; i < snk_length; i++)
      if (snk_x[0] == snk_x[i] && snk_y[0] == snk_y[i]) {
        snk_can--;
        bipCanKaybi();
        if (snk_can <= 0)
          snk_gameOver = true;
        else {
          snk_length = 4;
          snk_dir = 1;
          for (int i2 = 0; i2 < 4; i2++) {
            snk_x[i2] = 15 - i2;
            snk_y[i2] = 5;
          }
        }
      }
    if (snk_x[0] == snk_foodX && snk_y[0] == snk_foodY) {
      if (snk_length < 100)
        snk_length++;
      snk_score += 10;
      snk_foodX = random(1, 29);
      snk_foodY = random(1, 11);
      bipYem();
      // Her 50 puanda hizlan
      if (snk_score % 50 == 0 && snk_speed > 40)
        snk_speed -= 10;
    }
  }
  ekran.fillRect((snk_foodX * 4) + 2, (snk_foodY * 4) + 14, 4, 4, SH110X_WHITE);
  for (int i = 0; i < snk_length; i++)
    ekran.fillRect((snk_x[i] * 4) + 2, (snk_y[i] * 4) + 14, 4, 4, SH110X_WHITE);
}

// ===================== PONG (2 Mod: AI/ï¿½nsan, Popup) =====================
void runPongGame() {
  if (!png_active) {
    png_pL_Y = 30;
    png_pR_Y = 30;
    png_sL = 0;
    png_sR = 0;
    png_gameOver = false;
    png_bX = 63;
    png_bY = 37;
    png_bDX = 2;
    png_bDY = 1;
    png_active = true;
  }
  drawGameUI("Pong", String(png_sL) + "-" + String(png_sR));
  if (png_gameOver) {
    if (png_sL > png_sR)
      popupGoster("KAZANDIN!", "[B] Yeni Oyun");
    else
      popupGoster("KAYBETTIN", "[B] Yeni Oyun");
    bipOyunBitti();
    if (popupYenidenBaslat()) {
      png_active = false;
      aktifEkran = EKRAN_ANA_MENU;
    }
    return;
  }
  if (btnKenarBul(BTN_KIRMIZI, 0)) {
    png_active = false;
    aktifEkran = EKRAN_ANA_MENU;
    return;
  }
  if (millis() - png_lastMoveTime > 30) {
    png_lastMoveTime = millis();
    // Sol oyuncu - sag joystick
    if (solTetikYukari() && png_pL_Y > 14)
      png_pL_Y -= 2;
    if (solTetikAsagi() && png_pL_Y < 48)
      png_pL_Y += 2;
    // Sag oyuncu
    if (png_mode == 0) {
      // AI modu - biraz aptal
      if (random(0, 3) != 0) { // %33 sans ile hareket etmez
        if (png_bY < png_pR_Y + 7 && png_pR_Y > 14)
          png_pR_Y -= 1;
        if (png_bY > png_pR_Y + 7 && png_pR_Y < 48)
          png_pR_Y += 1;
      }
    } else {
      // Insan modu - sol joystick
      if (guncelPaket.solJoyY < 1000 && png_pR_Y > 14)
        png_pR_Y -= 2;
      if (guncelPaket.solJoyY > 3000 && png_pR_Y < 48)
        png_pR_Y += 2;
    }
    png_bX += png_bDX;
    png_bY += png_bDY;
    if (png_bY <= 14 || png_bY >= 60)
      png_bDY = -png_bDY;
    if (png_bX <= 4 && png_bY + 2 >= png_pL_Y && png_bY <= png_pL_Y + 14) {
      png_bX = 4;
      png_bDX = -png_bDX;
      bipTek();
    }
    if (png_bX >= 122 && png_bY + 2 >= png_pR_Y && png_bY <= png_pR_Y + 14) {
      png_bX = 122;
      png_bDX = -png_bDX;
      bipTek();
    }
    if (png_bX < 0) {
      png_sR++;
      png_bX = 63;
      png_bY = 37;
      png_bDX = 2;
      png_bDY = 1;
      bipCanKaybi();
    } else if (png_bX > 128) {
      png_sL++;
      png_bX = 63;
      png_bY = 37;
      png_bDX = -2;
      png_bDY = -1;
      bipCanKaybi();
    }
    if (png_sL >= 5 || png_sR >= 5)
      png_gameOver = true;
  }
  for (int i = 14; i < 63; i += 4)
    ekran.drawPixel(64, i, SH110X_WHITE);
  ekran.fillRect(2, png_pL_Y, 2, 14, SH110X_WHITE);
  ekran.fillRect(124, png_pR_Y, 2, 14, SH110X_WHITE);
  ekran.fillRect(png_bX, png_bY, 2, 2, SH110X_WHITE);
}

// ===================== XOX (Minimax AI, 10 Seviye) =====================
void runXOXGame() {
  if (!xox_active) {
    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++)
        xox_board[i][j] = 0;
    xox_curX = 1;
    xox_curY = 1;
    xox_turn = 1;
    xox_win = 0;
    xox_active = true;
    xox_seviye = 1;
    xox_skor_x = 0;
    xox_skor_o = 0;
  }
  drawGameUI("XOX", "Svy>" + String(xox_seviye));
  if (btnKenarBul(BTN_KIRMIZI, 0)) {
    xox_active = false;
    aktifEkran = EKRAN_ANA_MENU;
    return;
  }
  if (xox_win == 0) {
    if (solTetikYukari()) {
      xox_curY--;
      vTaskDelay(pdMS_TO_TICKS(200));
    } else if (solTetikAsagi()) {
      xox_curY++;
      vTaskDelay(pdMS_TO_TICKS(200));
    }
    if (solTetikSola()) {
      xox_curX--;
      vTaskDelay(pdMS_TO_TICKS(200));
    } else if (solTetikSaga()) {
      xox_curX++;
      vTaskDelay(pdMS_TO_TICKS(200));
    }
    if (xox_curX < 0)
      xox_curX = 2;
    if (xox_curX > 2)
      xox_curX = 0;
    if (xox_curY < 0)
      xox_curY = 2;
    if (xox_curY > 2)
      xox_curY = 0;
    if (btnKenarBul(BTN_YESIL, 1) && xox_board[xox_curY][xox_curX] == 0) {
      xox_board[xox_curY][xox_curX] = 1; // Oyuncu X
      bipMenuSec();
      // Kazanma kontrolu
      int w = 0;
      bool f = true;
      for (int i = 0; i < 3; i++) {
        if (xox_board[i][0] != 0 && xox_board[i][0] == xox_board[i][1] &&
            xox_board[i][1] == xox_board[i][2])
          w = xox_board[i][0];
        if (xox_board[0][i] != 0 && xox_board[0][i] == xox_board[1][i] &&
            xox_board[1][i] == xox_board[2][i])
          w = xox_board[0][i];
        for (int j = 0; j < 3; j++)
          if (xox_board[i][j] == 0)
            f = false;
      }
      if (xox_board[0][0] != 0 && xox_board[0][0] == xox_board[1][1] &&
          xox_board[1][1] == xox_board[2][2])
        w = xox_board[0][0];
      if (xox_board[0][2] != 0 && xox_board[0][2] == xox_board[1][1] &&
          xox_board[1][1] == xox_board[2][0])
        w = xox_board[0][2];
      if (w != 0)
        xox_win = w;
      else if (f)
        xox_win = 3;
      // AI hamlesi (Minimax)
      if (xox_win == 0) {
        int best = enIyiHamleXOX(xox_board);
        if (best >= 0) {
          xox_board[best / 3][best % 3] = 2;
        }
        // Tekrar kazanma kontrolu
        w = 0;
        f = true;
        for (int i = 0; i < 3; i++) {
          if (xox_board[i][0] != 0 && xox_board[i][0] == xox_board[i][1] &&
              xox_board[i][1] == xox_board[i][2])
            w = xox_board[i][0];
          if (xox_board[0][i] != 0 && xox_board[0][i] == xox_board[1][i] &&
              xox_board[1][i] == xox_board[2][i])
            w = xox_board[0][i];
          for (int j = 0; j < 3; j++)
            if (xox_board[i][j] == 0)
              f = false;
        }
        if (xox_board[0][0] != 0 && xox_board[0][0] == xox_board[1][1] &&
            xox_board[1][1] == xox_board[2][2])
          w = xox_board[0][0];
        if (xox_board[0][2] != 0 && xox_board[0][2] == xox_board[1][1] &&
            xox_board[1][1] == xox_board[2][0])
          w = xox_board[0][2];
        if (w != 0)
          xox_win = w;
        else if (f)
          xox_win = 3;
      }
    }
  }
  int ox = 43, oy = 18, cs = 14;
  ekran.drawLine(ox + cs, oy, ox + cs, oy + cs * 3, SH110X_WHITE);
  ekran.drawLine(ox + cs * 2, oy, ox + cs * 2, oy + cs * 3, SH110X_WHITE);
  ekran.drawLine(ox, oy + cs, ox + cs * 3, oy + cs, SH110X_WHITE);
  ekran.drawLine(ox, oy + cs * 2, ox + cs * 3, oy + cs * 2, SH110X_WHITE);
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) {
      int cx = ox + (j * cs) + 3, cy = oy + (i * cs) + 3;
      if (xox_board[i][j] == 1) {
        ekran.drawLine(cx, cy, cx + 8, cy + 8, SH110X_WHITE);
        ekran.drawLine(cx + 8, cy, cx, cy + 8, SH110X_WHITE);
      } else if (xox_board[i][j] == 2)
        ekran.drawCircle(cx + 4, cy + 4, 4, SH110X_WHITE);
    }
  if (millis() % 500 < 250 && xox_win == 0)
    ekran.drawRect(ox + (xox_curX * cs), oy + (xox_curY * cs), cs, cs,
                   SH110X_WHITE);
  if (xox_win != 0) {
    if (xox_win == 1) {
      popupGoster("X KAZANDI!", "Seviye Atla...");
      bipSeviyeAtla();
      xox_skor_x++;
    } else if (xox_win == 2) {
      popupGoster("O KAZANDI", "Tekrar Dene");
      bipCanKaybi();
    } else {
      popupGoster("BERABERE", "Seviye Atla...");
      bipMenuSec();
      xox_skor_x++;
    }
    if (popupYenidenBaslat()) {
      if (xox_win == 1 || xox_win == 3) {
        xox_seviye++;
        if (xox_seviye > 10) {
          popupGoster("TEBRIKLER!", "10 Seviye Bitti!");
          xox_active = false;
          return;
        }
      }
      for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
          xox_board[i][j] = 0;
      xox_curX = 1;
      xox_curY = 1;
      xox_win = 0;
    }
  }
}

// ===================== MAYIN TARLASI (Windows Stili, 12x6)=====================
void runMinesweeperGame() {
  if (!mine_act) {
    for (int r = 0; r < MINE_ROWS; r++)
      for (int c = 0; c < MINE_COLS; c++) {
        mine_grid[r][c] = 0;
        mine_rev[r][c] = false;
        mine_flag[r][c] = false;
      }
    int bombs = 8;
    for (int i = 0; i < bombs; i++) {
      int r = random(0, MINE_ROWS), c = random(0, MINE_COLS);
      if (mine_grid[r][c] == -1)
        i--;
      else
        mine_grid[r][c] = -1;
    }
    for (int r = 0; r < MINE_ROWS; r++)
      for (int c = 0; c < MINE_COLS; c++) {
        if (mine_grid[r][c] == -1)
          continue;
        int cnt = 0;
        for (int i = -1; i <= 1; i++)
          for (int j = -1; j <= 1; j++)
            if (r + i >= 0 && r + i < MINE_ROWS && c + j >= 0 &&
                c + j < MINE_COLS && mine_grid[r + i][c + j] == -1)
              cnt++;
        mine_grid[r][c] = cnt;
      }
    mine_cx = 0;
    mine_cy = 0;
    mine_act = true;
    mine_go = false;
  }
  drawGameUI("Mayin Tarlasi", "-");
  if (btnKenarBul(BTN_KIRMIZI, 0)) {
    mine_act = false;
    aktifEkran = EKRAN_ANA_MENU;
    return;
  }
  if (!mine_go) {
    if (solTetikYukari()) {
      mine_cy--;
      vTaskDelay(pdMS_TO_TICKS(150));
    } else if (solTetikAsagi()) {
      mine_cy++;
      vTaskDelay(pdMS_TO_TICKS(150));
    }
    if (solTetikSola()) {
      mine_cx--;
      vTaskDelay(pdMS_TO_TICKS(150));
    } else if (solTetikSaga()) {
      mine_cx++;
      vTaskDelay(pdMS_TO_TICKS(150));
    }
    if (mine_cx < 0)
      mine_cx = 0;
    if (mine_cx >= MINE_COLS)
      mine_cx = MINE_COLS - 1;
    if (mine_cy < 0)
      mine_cy = 0;
    if (mine_cy >= MINE_ROWS)
      mine_cy = MINE_ROWS - 1;
    if (btnKenarBul(BTN_YESIL, 1) && !mine_flag[mine_cy][mine_cx]) {
      mine_rev[mine_cy][mine_cx] = true;
      if (mine_grid[mine_cy][mine_cx] == -1) {
        mine_go = true;
        bipPatlama();
      } else if (mine_grid[mine_cy][mine_cx] == 0) {
        // Flood fill - bos alanlari ac
        for (int pass = 0; pass < 10; pass++)
          for (int r = 0; r < MINE_ROWS; r++)
            for (int c = 0; c < MINE_COLS; c++) {
              if (mine_rev[r][c] && mine_grid[r][c] == 0) {
                for (int i = -1; i <= 1; i++)
                  for (int j = -1; j <= 1; j++) {
                    if (r + i >= 0 && r + i < MINE_ROWS && c + j >= 0 &&
                        c + j < MINE_COLS && !mine_rev[r + i][c + j] &&
                        !mine_flag[r + i][c + j])
                      mine_rev[r + i][c + j] = true;
                  }
              }
            }
        bipMenuSec();
      } else
        bipMenuSec();
    }
    if (btnKenarBul(BTN_MAVI, 2) && !mine_rev[mine_cy][mine_cx]) {
      mine_flag[mine_cy][mine_cx] = !mine_flag[mine_cy][mine_cx];
      bipMenuSec();
    }
    // Kazanma kontrolu
    bool won = true;
    for (int r = 0; r < MINE_ROWS; r++)
      for (int c = 0; c < MINE_COLS; c++) {
        if (mine_grid[r][c] != -1 && !mine_rev[r][c])
          won = false;
      }
    if (won) {
      popupGoster("TEBRIKLER!", "Mayin Temizlendi!");
      bipSeviyeAtla();
      if (popupYenidenBaslat())
        mine_act = false;
      return;
    }
  }
  // 100x50 piksel alanda cizim
  int sx = 14, sy = 14, cw = 8, ch = 8;
  for (int r = 0; r < MINE_ROWS; r++)
    for (int c = 0; c < MINE_COLS; c++) {
      int x = sx + c * cw;
      int y = sy + r * ch;
      if (mine_rev[r][c]) {
        ekran.drawRect(x, y, cw, ch, SH110X_WHITE);
        if (mine_grid[r][c] == -1)
          ekran.fillCircle(x + 3, y + 3, 2, SH110X_WHITE);
        else if (mine_grid[r][c] > 0) {
          ekran.setCursor(x + 2, y);
          ekran.print(mine_grid[r][c]);
        }
      } else {
        ekran.fillRect(x, y, cw, ch, SH110X_WHITE);
        ekran.drawRect(x, y, cw, ch, SH110X_BLACK);
        if (mine_flag[r][c]) {
          ekran.drawLine(x + 2, y + 2, x + 5, y + 5, SH110X_BLACK);
          ekran.drawLine(x + 5, y + 2, x + 2, y + 5, SH110X_BLACK);
        }
      }
    }
  if (!mine_go && millis() % 500 < 250)
    ekran.drawRect(sx + mine_cx * cw + 1, sy + mine_cy * ch + 1, cw - 2, ch - 2,
                   SH110X_INVERSE);
  if (mine_go) {
    popupGoster("PATLADIN!", "[B] Yeni Oyun");
    bipOyunBitti();
    if (popupYenidenBaslat())
      mine_act = false;
  }
}

// ===================== TETRï¿½S (7px Saï¿½a Kaydï¿½rï¿½lmï¿½ï¿½) =====================
bool checkTetrisCollision(int t_type, int t_rot, int tx, int ty) {
  uint16_t shape = tet_shapes[t_type][t_rot];
  for (int i = 0; i < 16; i++) {
    if (shape & (1 << (15 - i))) {
      int px = tx + (i % 4), py = ty + (i / 4);
      if (px < 0 || px >= 10 || py >= 15)
        return true;
      if (py >= 0 && tet_board[py][px] != 0)
        return true;
    }
  }
  return false;
}
void runTetrisGame() {
  if (!tet_act) {
    for (int r = 0; r < 15; r++)
      for (int c = 0; c < 10; c++)
        tet_board[r][c] = 0;
    tet_next = random(0, 7);
    tet_type = random(0, 7);
    tet_rot = 0;
    tet_x = 3;
    tet_y = -1;
    tet_score = 0;
    tet_act = true;
    tet_go = false;
  }
  if (btnKenarBul(BTN_KIRMIZI, 0)) {
    tet_act = false;
    aktifEkran = EKRAN_ANA_MENU;
    return;
  }
  if (!tet_go) {
    if (solTetikSola() &&
        !checkTetrisCollision(tet_type, tet_rot, tet_x - 1, tet_y)) {
      tet_x--;
      vTaskDelay(pdMS_TO_TICKS(100));
    }
    if (solTetikSaga() &&
        !checkTetrisCollision(tet_type, tet_rot, tet_x + 1, tet_y)) {
      tet_x++;
      vTaskDelay(pdMS_TO_TICKS(100));
    }
    if (btnKenarBul(BTN_YESIL, 1) || solTetikYukari()) {
      int nr = (tet_rot + 1) % 4;
      if (!checkTetrisCollision(tet_type, nr, tet_x, tet_y))
        tet_rot = nr;
      vTaskDelay(pdMS_TO_TICKS(150));
    }
    int speed = solTetikAsagi() ? 40 : 400;
    if (millis() - tet_lastDrop > speed) {
      tet_lastDrop = millis();
      if (!checkTetrisCollision(tet_type, tet_rot, tet_x, tet_y + 1))
        tet_y++;
      else {
        uint16_t shape = tet_shapes[tet_type][tet_rot];
        for (int i = 0; i < 16; i++) {
          if (shape & (1 << (15 - i))) {
            int px = tet_x + (i % 4);
            int py = tet_y + (i / 4);
            if (py >= 0)
              tet_board[py][px] = 1;
          }
        }
        int lc = 0;
        for (int r = 14; r >= 0; r--) {
          bool full = true;
          for (int c = 0; c < 10; c++)
            if (tet_board[r][c] == 0)
              full = false;
          if (full) {
            lc++;
            bipPatlama();
            for (int yr = r; yr > 0; yr--)
              for (int c = 0; c < 10; c++)
                tet_board[yr][c] = tet_board[yr - 1][c];
            for (int c = 0; c < 10; c++)
              tet_board[0][c] = 0;
            r++;
          }
        }
        if (lc == 1)
          tet_score += 100;
        else if (lc == 2)
          tet_score += 300;
        else if (lc == 3)
          tet_score += 500;
        else if (lc == 4) {
          tet_score += 800;
          bipSeviyeAtla();
        }
        tet_type = tet_next;
        tet_next = random(0, 7);
        tet_rot = 0;
        tet_x = 3;
        tet_y = -1;
        if (checkTetrisCollision(tet_type, tet_rot, tet_x, tet_y))
          tet_go = true;
      }
    }
  }
  ekran.drawRect(0, 0, 128, 64, SH110X_WHITE);
  // Sonraki parca (sol ust)
  ekran.setCursor(4, 4);
  ekran.print("Sonraki");
  ekran.drawRect(4, 14, 24, 24, SH110X_WHITE);
  uint16_t ns = tet_shapes[tet_next][0];
  for (int i = 0; i < 16; i++) {
    if (ns & (1 << (15 - i)))
      ekran.drawRect(8 + (i % 4) * 4, 18 + (i / 4) * 4, 4, 4, SH110X_WHITE);
  }
  // Skor ve baslik (sag)
  ekran.setCursor(92, 10);
  ekran.print("TETRIS");
  ekran.setCursor(92, 25);
  ekran.print("Skor:");
  ekran.setCursor(92, 35);
  ekran.print(tet_score);
  // Oyun alani - 7 piksel saga kaydirildi
  int sx = 47, sy = 2, bs = 4;
  ekran.drawRect(sx - 1, sy - 1, (10 * bs) + 2, (15 * bs) + 2, SH110X_WHITE);
  for (int r = 0; r < 15; r++)
    for (int c = 0; c < 10; c++) {
      if (tet_board[r][c] == 1)
        ekran.drawRect(sx + (c * bs), sy + (r * bs), bs, bs, SH110X_WHITE);
    }
  if (!tet_go) {
    uint16_t shape = tet_shapes[tet_type][tet_rot];
    for (int i = 0; i < 16; i++) {
      if (shape & (1 << (15 - i))) {
        int px = sx + ((tet_x + (i % 4)) * bs);
        int py = sy + ((tet_y + (i / 4)) * bs);
        if (tet_y + (i / 4) >= 0)
          ekran.drawRect(px, py, bs, bs, SH110X_WHITE);
      }
    }
  } else {
    popupGoster("GAME OVER", "[B] Yeni Oyun");
    bipOyunBitti();
    if (popupYenidenBaslat())
      tet_act = false;
  }
}

// ===================== ZAR OYUNU (Casino Sesleri) =====================
void runDiceGame() {
  // Skor> kaldirildi
  ekran.drawRect(0, 0, 128, 64, SH110X_WHITE);
  ekran.drawLine(0, 12, 128, 12, SH110X_WHITE);
  ekran.setCursor(35, 3);
  ekran.print("Zar Oyunu");
  if (btnKenarBul(BTN_KIRMIZI, 0)) {
    aktifEkran = EKRAN_ANA_MENU;
    return;
  }
  if (btnKenarBul(BTN_YESIL, 1) && !dice_rolling) {
    dice_rolling = true;
    dice_roll_count = 0;
    dice_lastFrameTime = millis();
  }
  if (dice_rolling) {
    int cd;
    if (dice_roll_count < 10)
      cd = 35;
    else if (dice_roll_count < 18)
      cd = 80 + ((dice_roll_count - 10) * 15);
    else
      cd = 200 + ((dice_roll_count - 18) * 100);
    if (dice_roll_count >= 24) {
      dice_rolling = false;
      bipZarSon();
    }
    if (dice_rolling && (millis() - dice_lastFrameTime > cd)) {
      dice_val1 = random(1, 7);
      dice_val2 = random(1, 7);
      dice_lastFrameTime = millis();
      dice_roll_count++;
      bipZarTik(dice_roll_count);
    }
  }
  auto zarCiz = [](int dx, int dy, int val) {
    ekran.drawRoundRect(dx, dy, 28, 28, 4, SH110X_WHITE);
    if (val == 1 || val == 3 || val == 5)
      ekran.fillCircle(dx + 14, dy + 14, 3, SH110X_WHITE);
    if (val > 1) {
      ekran.fillCircle(dx + 6, dy + 6, 3, SH110X_WHITE);
      ekran.fillCircle(dx + 22, dy + 22, 3, SH110X_WHITE);
    }
    if (val > 3) {
      ekran.fillCircle(dx + 22, dy + 6, 3, SH110X_WHITE);
      ekran.fillCircle(dx + 6, dy + 22, 3, SH110X_WHITE);
    }
    if (val == 6) {
      ekran.fillCircle(dx + 6, dy + 14, 3, SH110X_WHITE);
      ekran.fillCircle(dx + 22, dy + 14, 3, SH110X_WHITE);
    }
  };
  zarCiz(16, 20, dice_val1);
  zarCiz(56, 20, dice_val2);
  ekran.setTextSize(2);
  ekran.setCursor(96, 26);
  if (dice_val1 + dice_val2 < 10)
    ekran.print("0");
  ekran.print(dice_val1 + dice_val2);
  ekran.setTextSize(1);
  ekran.setCursor(3, 55);
  if (dice_rolling)
    ekran.print("  Bekle...");
  else
    ekran.print("[B] Zar At  [A] Cik");
}

// ===================== KAHVE FALI (Cï¿½mle ï¿½retici) =====================
void runFal() {
  static String falMetin = "";
  static int falScroll = 0;
  drawGameUI("Kahve Fali", "-");
  if (btnKenarBul(BTN_KIRMIZI, 0)) {
    fal_act = false;
    fal_msg = -1;
    aktifEkran = EKRAN_ANA_MENU;
    return;
  }
  if (!fal_reading && btnKenarBul(BTN_YESIL, 1)) {
    fal_reading = true;
    fal_time = millis();
    fal_msg = -1;
    falScroll = 0;
  }
  ekran.drawCircle(64, 35, 15, SH110X_WHITE);
  ekran.drawCircle(64, 35, 12, SH110X_WHITE);
  if (!fal_reading && fal_msg == -1) {
    ekran.setCursor(25, 54);
    ekran.print("Fincani Ac [B]");
  } else if (fal_reading) {
    if (millis() - fal_time < 3000) {
      ekran.setCursor(30, 54);
      ekran.print("Soguyor...");
      ekran.fillCircle(64, 35, random(5, 10), SH110X_INVERSE);
    } else {
      fal_reading = false;
      fal_msg = 1;
      falMetin = falCumleUret();
    }
  } else {
    ekran.fillCircle(64 + random(-5, 5), 35 + random(-5, 5), 3, SH110X_WHITE);
    ekran.fillRect(0, 52, 128, 12, SH110X_BLACK);
    // Kayan yazi efekti
    String gorunen = falMetin.substring(falScroll, falScroll + 21);
    ekran.setCursor(2, 54);
    ekran.print(gorunen);
    if (millis() % 200 < 50 && falScroll < (int)falMetin.length() - 20)
      falScroll++;
    // Yeni fal cek
    if (solTetikSaga()) {
      fal_reading = true;
      fal_time = millis();
      fal_msg = -1;
      falScroll = 0;
    }
  }
}

// ===================== KRONOMETRE =====================
void runPomodoro() {
  drawGameUI("Kronometre", "-");
  if (btnKenarBul(BTN_KIRMIZI, 0)) {
    aktifEkran = EKRAN_ANA_MENU;
    return;
  }
  if (btnKenarBul(BTN_YESIL, 1)) {
    if (pom_running) {
      pom_running = false;
      pom_elapsed += (millis() - pom_startMillis);
    } else {
      pom_running = true;
      pom_startMillis = millis();
    }
    bipMenuSec();
  }
  if (btnKenarBul(BTN_MAVI, 2)) {
    pom_running = false;
    pom_elapsed = 0;
    bipHata();
  }
  unsigned long total = pom_elapsed;
  if (pom_running)
    total += (millis() - pom_startMillis);
  int s = (total / 1000) % 60;
  int m = (total / 60000) % 60;
  ekran.setTextSize(3);
  ekran.setCursor(20, 25);
  if (m < 10)
    ekran.print("0");
  ekran.print(m);
  ekran.print(":");
  if (s < 10)
    ekran.print("0");
  ekran.print(s);
  ekran.setTextSize(1);
  ekran.setCursor(10, 54);
  ekran.print(pom_running ? "[B] Durdur" : "[B] Baslat");
  ekran.setCursor(80, 54);
  ekran.print("[X] Sifirla");
}

// ===================== HESAP Makinesi (Bilimsel Klavye) =====================
void runCalculatorApp() {
  // Skor kaldirildi, baslik
  ekran.drawRect(0, 0, 128, 64, SH110X_WHITE);
  ekran.drawLine(0, 12, 128, 12, SH110X_WHITE);
  ekran.setCursor(35, 3);
  ekran.print("Hesap Mak.");
  if (btnKenarBul(BTN_KIRMIZI, 0)) {
    aktifEkran = EKRAN_ANA_MENU;
    return;
  }
  // Toggle ile bilimsel mod gecisi
  if (btnKenarBul(BTN_MAVI, 2)) {
    calc_karSet = (calc_karSet == 0) ? 1 : 0;
    bipMenuSec();
  }
  if (solTetikYukari()) {
    calc_cy--;
    vTaskDelay(pdMS_TO_TICKS(150));
  } else if (solTetikAsagi()) {
    calc_cy++;
    vTaskDelay(pdMS_TO_TICKS(150));
  }
  if (solTetikSola()) {
    calc_cx--;
    vTaskDelay(pdMS_TO_TICKS(150));
  } else if (solTetikSaga()) {
    calc_cx++;
    vTaskDelay(pdMS_TO_TICKS(150));
  }
  if (calc_cx < 0)
    calc_cx = 3;
  if (calc_cx > 3)
    calc_cx = 0;
  if (calc_cy < 0)
    calc_cy = 3;
  if (calc_cy > 3)
    calc_cy = 0;
  if (btnKenarBul(BTN_YESIL, 1)) {
    if (calc_karSet == 0) {
      char k = calc_keys[calc_cy][calc_cx];
      if (k >= '0' && k <= '9')
        calc_disp += k;
      else if (k == 'C') {
        calc_disp = "";
        calc_num1 = 0;
        calc_num2 = 0;
        calc_op = ' ';
      } else if (k == '/' || k == '*' || k == '-' || k == '+') {
        calc_num1 = calc_disp.toFloat();
        calc_op = k;
        calc_disp = "";
      } else if (k == '=') {
        calc_num2 = calc_disp.toFloat();
        if (calc_op == '+')
          calc_num1 += calc_num2;
        else if (calc_op == '-')
          calc_num1 -= calc_num2;
        else if (calc_op == '*')
          calc_num1 *= calc_num2;
        else if (calc_op == '/' && calc_num2 != 0)
          calc_num1 /= calc_num2;
        calc_disp = String(calc_num1);
        calc_op = ' ';
      }
    } else {
      // Bilimsel mod
      char k = calc_sci_keys[calc_cy][calc_cx];
      float v = calc_disp.toFloat();
      if (k == 's') {
        calc_disp = String(sin(v * PI / 180.0), 4);
      } else if (k == 'c') {
        calc_disp = String(cos(v * PI / 180.0), 4);
      } else if (k == 't') {
        calc_disp = String(tan(v * PI / 180.0), 4);
      } else if (k == 'l') {
        if (v > 0)
          calc_disp = String(log10(v), 4);
      } else if (k == 'S') {
        calc_disp = String(asin(v) * 180.0 / PI, 4);
      } else if (k == 'C') {
        calc_disp = String(acos(v) * 180.0 / PI, 4);
      } else if (k == 'T') {
        calc_disp = String(atan(v) * 180.0 / PI, 4);
      } else if (k == 'L') {
        if (v > 0)
          calc_disp = String(log(v), 4);
      } else if (k == 'P') {
        calc_disp = String(PI, 6);
      } else if (k == 'e') {
        calc_disp = String(M_E, 6);
      } else if (k == '!') {
        long f = 1;
        int n = (int)v;
        for (int i = 2; i <= n; i++)
          f *= i;
        calc_disp = String(f);
      } else if (k == '^') {
        calc_num1 = v;
        calc_op = '^';
        calc_disp = "";
      } else if (k == '(') {
        calc_disp += "(";
      } else if (k == ')') {
        calc_disp += ")";
      } else if (k == '.') {
        calc_disp += ".";
      } else if (k == 'R') {
        if (v >= 0)
          calc_disp = String(sqrt(v), 4);
      }
      if (calc_op == '^' && calc_disp.length() > 0 && k != '(') {
        float p = calc_disp.toFloat();
        calc_disp = String(pow(calc_num1, p), 4);
        calc_op = ' ';
      }
    }
    bipMenuSec();
  }
  ekran.drawRect(10, 16, 108, 12, SH110X_WHITE);
  ekran.setCursor(12, 18);
  ekran.print(calc_disp);
  int sx = 24, sy = 30, cw = 20, ch = 8;
  char (*keys)[4] = (calc_karSet == 0) ? calc_keys : (char (*)[4])calc_sci_keys;
  for (int r = 0; r < 4; r++)
    for (int c = 0; c < 4; c++) {
      ekran.drawRect(sx + c * cw, sy + r * ch, cw, ch, SH110X_WHITE);
      ekran.setCursor(sx + c * cw + 7, sy + r * ch + 1);
      ekran.print(keys[r][c]);
    }
  ekran.fillRect(sx + calc_cx * cw, sy + calc_cy * ch, cw, ch, SH110X_INVERSE);
  ekran.setCursor(2, 63);
  ekran.print(calc_karSet == 0 ? "[X]Bilimsel" : "[X]Numpad");
}
//KAYDEDİLDİ