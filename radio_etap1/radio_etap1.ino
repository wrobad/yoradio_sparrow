// Radio internetowe - etap 1
// ESP32-S3-N8R2 + PCM5102 (I2S) + ILI9341 2.8" z dotykiem
//
// Biblioteki (Menedżer bibliotek / GitHub):
//   - LovyanGFX
//   - ESP32-audioI2S (schreibfaul1) - najnowsza wersja z GitHuba
// Płytka: ESP32S3 Dev Module, PSRAM: QSPI PSRAM, Flash: 8MB,
//         Partition Scheme: 8M with spiffs (lub inna z dużą aplikacją)

#include <WiFi.h>
#include "Audio.h"
#include "lgfx_config.h"
#include "podswietlenie.h"
#include "enkoder.h"

// ---------- USTAW SWOJE ----------
const char* WIFI_SSID = "TwojaSiec";
const char* WIFI_PASS = "TwojeHaslo";

// Stacje testowe - podmień/dopisz własne
const char* STACJE[] = {
  "http://stream.srg-ssr.ch/m/rsj/mp3_128",   // Radio Swiss Jazz
  "http://stream.srg-ssr.ch/m/rsc_de/mp3_128" // Radio Swiss Classic
};
const int LICZBA_STACJI = sizeof(STACJE) / sizeof(STACJE[0]);

// Wklej tu wynik "Kalibracja: {...}" z testu wyświetlacza.
// Same zera = kalibracja przy każdym starcie.
uint16_t KALIBRACJA[8] = {0, 0, 0, 0, 0, 0, 0, 0};
// ---------------------------------

// I2S -> PCM5102
#define I2S_BCLK 5   // BCK
#define I2S_LRC  6   // LRCK
#define I2S_DOUT 7   // DATA

Audio audio;
LGFX  tft;

int  stacja   = 0;
int  glosnosc = 10;          // 0..21

// Dane z callbacku - rysujemy je w loop(), nie w callbacku
String nazwaStacji = "";
String tytul       = "";
volatile bool odswiezStacje = true;
volatile bool odswiezTytul  = true;
volatile bool odswiezGlos   = true;

bool     trybJasnosci = false;   // enkoder: false = głośność, true = jasność
uint32_t tEnkoder     = 0;

// ---------- Callback biblioteki audio ----------
void my_audio_info(Audio::msg_t m) {
  switch (m.e) {
    case Audio::evt_name:
      nazwaStacji = m.msg;
      odswiezStacje = true;
      break;
    case Audio::evt_streamtitle:
      tytul = m.msg;
      odswiezTytul = true;
      break;
    case Audio::evt_bitrate:
      Serial.printf("bitrate: %s\n", m.msg);
      break;
    default:
      Serial.printf("audio: %s\n", m.msg);
      break;
  }
}

// ---------- Ekran ----------
const int Y_STACJA = 10;
const int Y_TYTUL  = 70;
const int Y_GLOS   = 170;

void rysujRamke() {
  tft.fillScreen(TFT_BLACK);
  tft.drawFastHLine(0, 60, tft.width(), TFT_DARKGREY);
  tft.drawFastHLine(0, 160, tft.width(), TFT_DARKGREY);

  // Przyciski dotykowe na dole
  int y = 200, h = 38, w = tft.width() / 4;
  const char* et[] = {"<<", "-", "+", ">>"};
  tft.setFont(&fonts::Font4);
  tft.setTextDatum(middle_center);
  for (int i = 0; i < 4; i++) {
    tft.drawRoundRect(i * w + 3, y, w - 6, h, 6, TFT_CYAN);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString(et[i], i * w + w / 2, y + h / 2);
  }
}

void rysujTekst(int y, int h, const String& txt, const lgfx::IFont* font, uint16_t kolor) {
  tft.fillRect(0, y, tft.width(), h, TFT_BLACK);
  tft.setFont(font);
  tft.setTextColor(kolor, TFT_BLACK);
  tft.setTextDatum(top_left);
  tft.setTextWrap(true);
  tft.setCursor(8, y);
  tft.print(txt);
  tft.setTextWrap(false);
}

void rysujGlosnosc() {   // pasek głośności albo jasności (tryb enkodera)
  tft.fillRect(0, Y_GLOS, tft.width(), 24, TFT_BLACK);
  int maxW = tft.width() - 80;
  int w;
  uint16_t kolor;
  String opis;
  if (trybJasnosci) {
    w = map(blJasnosc, 0, 255, 0, maxW);
    kolor = TFT_YELLOW;
    opis = "Jasn " + String(blJasnosc * 100 / 255) + "%";
  } else {
    w = map(glosnosc, 0, 21, 0, maxW);
    kolor = TFT_GREEN;
    opis = "Vol " + String(glosnosc);
  }
  tft.drawRect(8, Y_GLOS + 4, maxW, 14, TFT_DARKGREY);
  tft.fillRect(8, Y_GLOS + 4, w, 14, kolor);
  tft.setFont(&fonts::Font2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(top_left);
  tft.drawString(opis, maxW + 16, Y_GLOS + 4);
}

// ---------- Sterowanie ----------
void grajStacje(int nr) {
  stacja = (nr + LICZBA_STACJI) % LICZBA_STACJI;
  nazwaStacji = "Laczenie...";
  tytul = "";
  odswiezStacje = odswiezTytul = true;
  Serial.printf("Stacja %d: %s\n", stacja, STACJE[stacja]);
  audio.connecttohost(STACJE[stacja]);
}

void ustawGlosnosc(int v) {
  glosnosc = constrain(v, 0, 21);
  audio.setVolume(glosnosc);
  odswiezGlos = true;
}

void obsluzDotyk() {
  static uint32_t ostatni = 0;
  int32_t x, y;
  if (!tft.getTouch(&x, &y)) return;
  if (millis() - ostatni < 300) return;    // prosty debounce
  ostatni = millis();

  if (y < 195) return;                     // tylko pasek przycisków
  int przycisk = x / (tft.width() / 4);
  switch (przycisk) {
    case 0: grajStacje(stacja - 1);        break;
    case 1: ustawGlosnosc(glosnosc - 1);   break;
    case 2: ustawGlosnosc(glosnosc + 1);   break;
    case 3: grajStacje(stacja + 1);        break;
  }
}

// Enkoder:
//   obrót                 - głośność (w trybie jasności: jasność)
//   krótkie naciśnięcie   - następna stacja (w trybie jasności: powrót do głośności)
//   długie naciśnięcie    - włącz/wyłącz tryb jasności (sam wraca po 5 s bezczynności)
void obsluzEnkoder() {
  int k = enkoderKroki();
  if (k) {
    tEnkoder = millis();
    if (trybJasnosci) { ustawJasnosc(constrain(blJasnosc + k * 15, 10, 255)); odswiezGlos = true; }
    else              ustawGlosnosc(glosnosc + k);
  }

  switch (enkoderPrzycisk()) {
    case PRZ_KROTKO:
      if (trybJasnosci) { trybJasnosci = false; odswiezGlos = true; }
      else              grajStacje(stacja + 1);
      break;
    case PRZ_DLUGO:
      trybJasnosci = !trybJasnosci;
      tEnkoder = millis();
      odswiezGlos = true;
      break;
    default: break;
  }

  if (trybJasnosci && millis() - tEnkoder > 5000) { trybJasnosci = false; odswiezGlos = true; }
}

void obsluzSerial() {   // + / - głośność, n / p stacja, [ / ] jasność
  if (!Serial.available()) return;
  char c = Serial.read();
  if (c == '[') { ustawJasnosc(blJasnosc - 25); Serial.printf("Jasnosc: %d\n", blJasnosc); }
  if (c == ']') { ustawJasnosc(blJasnosc + 25); Serial.printf("Jasnosc: %d\n", blJasnosc); }
  if (c == '+') ustawGlosnosc(glosnosc + 1);
  if (c == '-') ustawGlosnosc(glosnosc - 1);
  if (c == 'n') grajStacje(stacja + 1);
  if (c == 'p') grajStacje(stacja - 1);
}

// ---------- Setup / loop ----------
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.printf("PSRAM: %u bajtow\n", ESP.getPsramSize());

  tft.init();
  tft.setRotation(1);       // poziomo 320x240
  startPodswietlenia(200);   // 0..255
  startEnkodera();

  bool brakKalibracji = true;
  for (int i = 0; i < 8; i++) if (KALIBRACJA[i]) brakKalibracji = false;
  if (brakKalibracji) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(middle_center);
    tft.setFont(&fonts::Font2);
    tft.drawString("Kalibracja: dotknij strzalek", tft.width() / 2, tft.height() / 2);
    tft.calibrateTouch(KALIBRACJA, TFT_WHITE, TFT_BLACK, 20);
    Serial.print("Kalibracja: {");
    for (int i = 0; i < 8; i++) { Serial.print(KALIBRACJA[i]); if (i < 7) Serial.print(", "); }
    Serial.println("}");
  }
  tft.setTouchCalibrate(KALIBRACJA);

  rysujRamke();
  rysujTekst(Y_STACJA, 48, "Laczenie z Wi-Fi...", &fonts::Font4, TFT_YELLOW);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);     // ważne dla płynnego strumienia
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) { delay(300); Serial.print("."); }
  Serial.printf("\nWi-Fi OK, IP: %s\n", WiFi.localIP().toString().c_str());

  Audio::audio_info_callback = my_audio_info;
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  ustawGlosnosc(glosnosc);
  grajStacje(0);
}

void loop() {
  audio.loop();

  if (odswiezStacje) { odswiezStacje = false; rysujTekst(Y_STACJA, 48, nazwaStacji, &fonts::Font4, TFT_YELLOW); }
  if (odswiezTytul)  { odswiezTytul  = false; rysujTekst(Y_TYTUL, 88, tytul, &fonts::Font4, TFT_WHITE); }
  if (odswiezGlos)   { odswiezGlos   = false; rysujGlosnosc(); }

  obsluzDotyk();
  obsluzEnkoder();
  obsluzSerial();

  // Automatyczne ponowne łączenie po zerwaniu Wi-Fi
  static uint32_t sprawdzWifi = 0;
  if (millis() - sprawdzWifi > 10000) {
    sprawdzWifi = millis();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Wi-Fi zerwane, lacze ponownie...");
      WiFi.reconnect();
    } else if (!audio.isRunning()) {
      grajStacje(stacja);
    }
  }

  vTaskDelay(1);
}
