// yoradio_sparrow - radio internetowe
// ESP32-S3-N8R2 + PCM5102 (I2S) + ILI9341 2.8" z dotykiem + enkoder
//
// Biblioteki:
//   - LovyanGFX
//   - ESP32-audioI2S (schreibfaul1) - najnowsza wersja z GitHuba
// Płytka: ESP32S3 Dev Module, PSRAM: QSPI PSRAM, Flash: 8MB,
//         Partition Scheme: 8M with spiffs (lub inna z dużą aplikacją)

#include <WiFi.h>
#include <time.h>
#include "Audio.h"
#include "lgfx_config.h"
#include "podswietlenie.h"
#include "enkoder.h"
#include "ekran.h"

// Dane Wi-Fi są w pliku sekrety.h (nie trafia na GitHuba).
// Pierwszy raz: skopiuj sekrety_przyklad.h jako sekrety.h i wpisz swoje dane.
#if __has_include("sekrety.h")
  #include "sekrety.h"
#else
  #error "Brak pliku sekrety.h - skopiuj sekrety_przyklad.h jako sekrety.h i wpisz dane Wi-Fi"
#endif

// ---------- USTAW SWOJE ----------
struct Stacja { const char* nazwa; const char* url; };

// Lista stacji jest w osobnym pliku stacje.h (Twoja) albo stacje_przyklad.h (wzór).
#if __has_include("stacje.h")
  #include "stacje.h"
#else
  #include "stacje_przyklad.h"
#endif
const int LICZBA_STACJI = sizeof(STACJE) / sizeof(STACJE[0]);

// Wklej tu wynik "Kalibracja: {...}" z Serial Monitora.
// Same zera = kalibracja przy każdym starcie.
uint16_t KALIBRACJA[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// Strefa czasowa Polski (zmiana czasu automatycznie)
const char* STREFA_CZASU = "CET-1CEST,M3.5.0,M10.5.0/3";
// ---------------------------------

// I2S -> PCM5102
#define I2S_BCLK 5   // BCK
#define I2S_LRC  6   // LRCK
#define I2S_DOUT 7   // DATA

Audio audio;
LGFX  tft;

int  stacja   = 0;
int  glosnosc = 10;          // 0..21
bool pauza    = false;

String tytul = "";
bool odswiezStacje    = true;
bool odswiezTytul     = true;
bool odswiezGlos      = true;
bool odswiezStan      = true;
bool odswiezPrzyciski = false;

enum TrybEnkodera { TRYB_STACJE, TRYB_GLOSNOSC, TRYB_JASNOSC };
TrybEnkodera trybEnk       = TRYB_STACJE;
uint32_t tEnkoder          = 0;
int      wybieranaStacja   = -1;   // -1 = nie przewijamy listy
uint32_t tWyboru           = 0;

// ---------- Callback biblioteki audio (wywoływany z audio.loop()) ----------
void my_audio_info(Audio::msg_t m) {
  switch (m.e) {
    case Audio::evt_streamtitle:
      tytul = m.msg ? m.msg : "";
      odswiezTytul = true;
      break;
    case Audio::evt_spectrum:
      ustawWidmo(m.vec1, m.vec2);
      break;
    case Audio::evt_bitrate:
      odswiezStan = true;
      break;
    default:
      Serial.printf("%s: %s\n", m.s ? m.s : "audio", m.msg ? m.msg : "");
      break;
  }
}

// ---------- Sterowanie ----------
void grajStacje(int nr) {
  stacja = (nr + LICZBA_STACJI) % LICZBA_STACJI;
  pauza  = false;
  tytul  = "";
  odswiezStacje = odswiezTytul = odswiezStan = odswiezPrzyciski = true;
  Serial.printf("Stacja %d: %s\n", stacja + 1, STACJE[stacja].url);
  audio.connecttohost(STACJE[stacja].url);
}

void ustawGlosnosc(int v) {
  glosnosc = constrain(v, 0, 21);
  audio.setVolume(glosnosc);
  odswiezGlos = true;
}

void przelaczPauze() {
  if (!pauza && !audio.isRunning()) { grajStacje(stacja); return; }  // nic nie gra -> połącz
  audio.pauseResume();
  pauza = !pauza;
  odswiezPrzyciski = odswiezStan = true;
}

void obsluzDotyk() {
  static uint32_t ostatni = 0;
  int32_t x, y;
  if (!tft.getTouch(&x, &y)) return;
  if (millis() - ostatni < 300) return;    // prosty debounce
  ostatni = millis();

  switch (ktoryPrzycisk(x, y)) {
    case 0: grajStacje(stacja - 1);      break;
    case 1: ustawGlosnosc(glosnosc - 1); break;
    case 2: przelaczPauze();             break;
    case 3: ustawGlosnosc(glosnosc + 1); break;
    case 4: grajStacje(stacja + 1);      break;
    default: break;
  }
}

// Enkoder:
//   obrót                    - wybór stacji (zatwierdzana po chwili bez ruchu albo kliknięciem)
//   krótkie naciśnięcie      - tryb głośności: obrót zmienia głośność
//   długie naciśnięcie (1 s) - tryb jasności:  obrót zmienia jasność
//   w trybie głośności/jasności: kliknięcie albo 5 s bez ruchu = powrót do wyboru stacji
const uint32_t OPOZNIENIE_WYBORU = 700;    // ms bez kręcenia -> włącz wybraną stację
const uint32_t POWROT_TRYBU      = 5000;   // ms bez kręcenia -> powrót do wyboru stacji

void ustawTryb(int t) {
  trybEnk  = (TrybEnkodera)t;
  tEnkoder = millis();
  odswiezGlos = true;
}

void obsluzEnkoder() {
  int k = enkoderKroki();
  if (k) {
    tEnkoder = millis();
    switch (trybEnk) {
      case TRYB_STACJE:
        if (wybieranaStacja < 0) wybieranaStacja = stacja;
        wybieranaStacja = ((wybieranaStacja + k) % LICZBA_STACJI + LICZBA_STACJI) % LICZBA_STACJI;
        tWyboru = millis();
        rysujNazweStacji(STACJE[wybieranaStacja].nazwa, TFT_WHITE);   // podgląd na biało
        odswiezStan = true;                                           // numer na pasku stanu
        break;
      case TRYB_GLOSNOSC:
        ustawGlosnosc(glosnosc + k);
        break;
      case TRYB_JASNOSC:
        ustawJasnosc(constrain(blJasnosc + k * 15, 10, 255));
        odswiezGlos = true;
        break;
    }
  }

  // Zatwierdzenie wybranej stacji po chwili bez kręcenia
  if (wybieranaStacja >= 0 && millis() - tWyboru > OPOZNIENIE_WYBORU) {
    int s = wybieranaStacja;
    wybieranaStacja = -1;
    if (s != stacja) grajStacje(s);
    else { odswiezStacje = true; odswiezStan = true; }   // wrócono do tej samej
  }

  switch (enkoderPrzycisk()) {
    case PRZ_KROTKO:
      if (wybieranaStacja >= 0)          tWyboru = 0;                 // kliknięcie = włącz od razu
      else if (trybEnk == TRYB_STACJE)   ustawTryb(TRYB_GLOSNOSC);
      else                               ustawTryb(TRYB_STACJE);
      break;
    case PRZ_DLUGO:
      ustawTryb(trybEnk == TRYB_JASNOSC ? TRYB_STACJE : TRYB_JASNOSC);
      break;
    default: break;
  }

  if (trybEnk != TRYB_STACJE && millis() - tEnkoder > POWROT_TRYBU) ustawTryb(TRYB_STACJE);
}

void obsluzSerial() {   // + / - głośność, n / p stacja, [ / ] jasność, spacja pauza
  if (!Serial.available()) return;
  char c = Serial.read();
  if (c == '[') { ustawJasnosc(blJasnosc - 25); Serial.printf("Jasnosc: %d\n", blJasnosc); }
  if (c == ']') { ustawJasnosc(blJasnosc + 25); Serial.printf("Jasnosc: %d\n", blJasnosc); }
  if (c == '+') ustawGlosnosc(glosnosc + 1);
  if (c == '-') ustawGlosnosc(glosnosc - 1);
  if (c == 'n') grajStacje(stacja + 1);
  if (c == 'p') grajStacje(stacja - 1);
  if (c == ' ') przelaczPauze();
}

void odswiezEkran() {
  if (odswiezStacje) { odswiezStacje = false; rysujNazweStacji(STACJE[stacja].nazwa); }
  if (odswiezTytul)  { odswiezTytul  = false; ustawTytul(tytul); }
  if (odswiezGlos) {
    odswiezGlos = false;
    if (trybEnk == TRYB_JASNOSC) rysujPasekGlosnosci(blJasnosc, 255, true, true);
    else                         rysujPasekGlosnosci(glosnosc, 21, false, trybEnk == TRYB_GLOSNOSC);
  }
  if (odswiezPrzyciski) { odswiezPrzyciski = false; rysujPrzycisk(2, pauza); }

  static uint32_t tStan = 0;
  if (odswiezStan || millis() - tStan > 1000) {
    odswiezStan = false;
    tStan = millis();
    rysujPasekStanu(wybieranaStacja >= 0 ? wybieranaStacja : stacja, LICZBA_STACJI, audio.getCodecname(), audio.getBitRate(), pauza);
  }

  animujTytul();
  rysujWidmo();
}

// ---------- Setup / loop ----------
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.printf("PSRAM: %u bajtow\n", ESP.getPsramSize());

  tft.init();
  tft.setRotation(1);       // poziomo 320x240
  startPodswietlenia(200);  // 0..255
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

  startEkranu();
  rysujRamke(pauza);
  rysujNazweStacji("Laczenie z Wi-Fi...");
  rysujPasekGlosnosci(glosnosc, 21, false);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);     // ważne dla płynnego strumienia
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) { delay(300); Serial.print("."); }
  Serial.printf("\nWi-Fi OK, IP: %s\n", WiFi.localIP().toString().c_str());

  configTzTime(STREFA_CZASU, "pool.ntp.org", "time.google.com");   // zegar z internetu

  Audio::audio_info_callback = my_audio_info;
  audio.settings.SPECTRUM = true;          // analizator widma (15 pasm)
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  ustawGlosnosc(glosnosc);
  grajStacje(0);
}

void loop() {
  audio.loop();

  obsluzDotyk();
  obsluzEnkoder();
  obsluzSerial();
  odswiezEkran();

  // Automatyczne ponowne łączenie po zerwaniu Wi-Fi lub strumienia
  static uint32_t sprawdzWifi = 0;
  if (millis() - sprawdzWifi > 10000) {
    sprawdzWifi = millis();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Wi-Fi zerwane, lacze ponownie...");
      WiFi.reconnect();
    } else if (!pauza && !audio.isRunning()) {
      grajStacje(stacja);
    }
  }

  vTaskDelay(1);
}
