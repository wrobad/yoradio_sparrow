// Wygląd ekranu radia (320x240 poziomo)
// Pasek stanu, nazwa stacji, przewijany tytuł, analizator widma,
// pasek głośności/jasności i przyciski dotykowe z ikonami.
#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <vector>
#include "lgfx_config.h"

extern LGFX tft;

// ---------- Kolory (RGB565) ----------
namespace kol {
  const uint16_t pasek     = 0x1105;  // granatowy pasek stanu
  const uint16_t zolty     = 0xFE2E;
  const uint16_t niebieski = 0x345B;
  const uint16_t jasnyNieb = 0x85BD;
  const uint16_t morski    = 0x9F19;
  const uint16_t zielonyVol= 0x64C4;
  const uint16_t widmoZiel = 0x1CEE;
  const uint16_t widmoPom  = 0xECE4;
  const uint16_t widmoCzer = 0xE249;
  const uint16_t szary     = 0x5AEB;
  const uint16_t linia     = 0x2965;
  const uint16_t wifiOff   = 0x4228;
  const uint16_t wifiOn    = 0x5E54;
}

// ---------- Układ ekranu ----------
const int E_W       = 320;
const int H_PASEK   = 20;
const int Y_STACJA  = 24,  H_STACJA = 28;
const int Y_TYTUL   = 56,  H_TYTUL  = 22;
const int Y_WIDMO   = 82,  H_WIDMO  = 70;
const int X_WIDMO   = 10,  SZER_WIDMA = 300;
const int Y_GLOS    = 158, H_GLOS   = 20;
const int Y_LINIA   = 186;
const int Y_PRZYC   = 192, H_PRZYC  = 44;
const int LICZBA_PASM = 15;

// ---------- Polskie znaki -> bez ogonków (do czasu dodania polskich czcionek) ----------
inline char zamienZnak(uint32_t c) {
  switch (c) {
    case 0x104: return 'A'; case 0x105: return 'a';
    case 0x106: return 'C'; case 0x107: return 'c';
    case 0x118: return 'E'; case 0x119: return 'e';
    case 0x141: return 'L'; case 0x142: return 'l';
    case 0x143: return 'N'; case 0x144: return 'n';
    case 0x0D3: return 'O'; case 0x0F3: return 'o';
    case 0x15A: return 'S'; case 0x15B: return 's';
    case 0x179: case 0x17B: return 'Z';
    case 0x17A: case 0x17C: return 'z';
    case 0x2018: case 0x2019: return '\'';
    case 0x201C: case 0x201D: return '"';
    case 0x2013: case 0x2014: return '-';
  }
  if (c >= 0xC0 && c <= 0xC5) return 'A';  if (c >= 0xE0 && c <= 0xE5) return 'a';
  if (c >= 0xC8 && c <= 0xCB) return 'E';  if (c >= 0xE8 && c <= 0xEB) return 'e';
  if (c >= 0xCC && c <= 0xCF) return 'I';  if (c >= 0xEC && c <= 0xEF) return 'i';
  if (c >= 0xD2 && c <= 0xD6) return 'O';  if (c >= 0xF2 && c <= 0xF6) return 'o';
  if (c >= 0xD9 && c <= 0xDC) return 'U';  if (c >= 0xF9 && c <= 0xFC) return 'u';
  if (c == 0xC7) return 'C';  if (c == 0xE7) return 'c';
  if (c == 0xD1) return 'N';  if (c == 0xF1) return 'n';
  return '?';
}

inline String bezOgonkow(const String& s) {
  String o;
  o.reserve(s.length());
  const uint8_t* p = (const uint8_t*)s.c_str();
  while (*p) {
    uint32_t cp;
    if (*p < 0x80) { o += (char)*p++; continue; }
    if ((*p & 0xE0) == 0xC0 && p[1]) {
      cp = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F); p += 2;
    } else if ((*p & 0xF0) == 0xE0 && p[1] && p[2]) {
      cp = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F); p += 3;
    } else { p++; continue; }           // inne (np. emoji) pomijamy
    o += zamienZnak(cp);
  }
  return o;
}

// ---------- Ikony ----------
inline void ikonaGlosnik(int x, int y, uint16_t c) {   // ok. 10x14
  tft.fillRect(x, y + 4, 4, 6, c);
  tft.fillTriangle(x + 3, y + 7, x + 10, y, x + 10, y + 14, c);
}

inline void ikonaSlonce(int x, int y, uint16_t c) {    // ok. 14x14
  tft.fillCircle(x + 7, y + 7, 3, c);
  tft.drawCircle(x + 7, y + 7, 6, c);
}

// ---------- Pasek stanu ----------
inline void rysujPasekStanu(int nr, int ile, const char* kodek, uint32_t bitrate, bool pauza) {
  char buf[24];
  tft.fillRect(0, 0, E_W, H_PASEK, kol::pasek);
  tft.setFont(&fonts::Font2);

  // Zegar
  struct tm t;
  if (getLocalTime(&t, 0)) strftime(buf, sizeof(buf), "%H:%M", &t);
  else strcpy(buf, "--:--");
  tft.setTextColor(TFT_WHITE, kol::pasek);
  tft.setTextDatum(middle_left);
  tft.drawString(buf, 6, H_PASEK / 2);

  // Format strumienia / pauza
  if (pauza)            snprintf(buf, sizeof(buf), "PAUZA");
  else if (bitrate > 0) snprintf(buf, sizeof(buf), "%s %uk", kodek ? kodek : "", (unsigned)(bitrate / 1000));
  else                  snprintf(buf, sizeof(buf), "%s", kodek ? kodek : "");
  tft.setTextColor(pauza ? kol::zolty : kol::jasnyNieb, kol::pasek);
  tft.setTextDatum(middle_center);
  tft.drawString(buf, 135, H_PASEK / 2);

  // Numer stacji
  snprintf(buf, sizeof(buf), "%d/%d", nr + 1, ile);
  tft.setTextColor(kol::morski, kol::pasek);
  tft.drawString(buf, 240, H_PASEK / 2);

  // Siła sygnału Wi-Fi
  int rssi = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : -127;
  int kreski = rssi > -55 ? 4 : rssi > -65 ? 3 : rssi > -75 ? 2 : rssi > -85 ? 1 : 0;
  for (int i = 0; i < 4; i++) {
    int h = 4 + i * 4;
    tft.fillRect(290 + i * 6, 17 - h, 4, h, i < kreski ? kol::wifiOn : kol::wifiOff);
  }
}

// ---------- Nazwa stacji ----------
// kolor: żółty = grająca stacja, biały = stacja wybierana enkoderem
inline void rysujNazweStacji(const String& nazwa, uint16_t kolor = kol::zolty) {
  tft.fillRect(0, Y_STACJA, E_W, H_STACJA, TFT_BLACK);
  tft.setFont(&fonts::FreeSansBold12pt7b);
  tft.setTextColor(kolor);
  tft.setTextDatum(middle_left);
  tft.setClipRect(8, Y_STACJA, E_W - 16, H_STACJA);
  tft.drawString(bezOgonkow(nazwa), 8, Y_STACJA + H_STACJA / 2);
  tft.clearClipRect();
}

// ---------- Tytuł (przewijany) ----------
static LGFX_Sprite sprTytul(&tft);
static String   tytulTxt;
static int      tytulSzer  = 0;
static int      tytulPoz   = 0;
static uint32_t tytulCzas  = 0;
static uint32_t tytulStop  = 0;     // przerwa przed (ponownym) przewijaniem
const  int      TYTUL_ODSTEP = 40;

inline void rysujTytulKlatka() {
  sprTytul.fillScreen(TFT_BLACK);
  sprTytul.setTextColor(TFT_WHITE);
  int y = H_TYTUL / 2;
  if (tytulSzer <= sprTytul.width()) {
    sprTytul.drawString(tytulTxt, 0, y);
  } else {
    sprTytul.drawString(tytulTxt, -tytulPoz, y);
    sprTytul.drawString(tytulTxt, -tytulPoz + tytulSzer + TYTUL_ODSTEP, y);
  }
  sprTytul.pushSprite(8, Y_TYTUL);
}

inline void ustawTytul(const String& t) {
  tytulTxt  = bezOgonkow(t);
  tytulSzer = sprTytul.textWidth(tytulTxt);
  tytulPoz  = 0;
  tytulStop = millis() + 2000;
  rysujTytulKlatka();
}

inline void animujTytul() {
  if (tytulSzer <= sprTytul.width()) return;
  uint32_t t = millis();
  if ((int32_t)(t - tytulStop) < 0 || t - tytulCzas < 30) return;
  tytulCzas = t;
  tytulPoz += 2;
  if (tytulPoz >= tytulSzer + TYTUL_ODSTEP) { tytulPoz = 0; tytulStop = t + 2000; }
  rysujTytulKlatka();
}

// ---------- Analizator widma ----------
static LGFX_Sprite sprWidmo(&tft);
static uint8_t widmo[LICZBA_PASM];
static uint8_t szczyt[LICZBA_PASM];
static bool    widmoNowe = false;

inline void ustawWidmo(const std::vector<uint32_t>& v, const std::vector<uint32_t>& p) {
  for (int i = 0; i < LICZBA_PASM; i++) {
    widmo[i]  = i < (int)v.size() ? v[i] : 0;
    szczyt[i] = i < (int)p.size() ? p[i] : 0;
  }
  widmoNowe = true;
}

inline void rysujWidmo() {
  static uint32_t t0 = 0;
  if (!widmoNowe || millis() - t0 < 33) return;   // max ~30 klatek/s
  t0 = millis();
  widmoNowe = false;

  sprWidmo.fillScreen(TFT_BLACK);
  const int kolW  = SZER_WIDMA / LICZBA_PASM;   // 20 px na pasmo
  const int barW  = kolW - 4;
  const int strC  = H_WIDMO * 20 / 100;         // górne 20% czerwone
  const int strP  = H_WIDMO * 45 / 100;         // dalej pomarańczowe, reszta zielona

  for (int i = 0; i < LICZBA_PASM; i++) {
    int x    = i * kolW + 2;
    int h    = widmo[i] * H_WIDMO / 255;
    int yTop = H_WIDMO - h;
    auto seg = [&](int a, int b, uint16_t c) {
      a = max(a, yTop);
      if (b > a) sprWidmo.fillRect(x, a, barW, b - a, c);
    };
    seg(0, strC, kol::widmoCzer);
    seg(strC, strP, kol::widmoPom);
    seg(strP, H_WIDMO, kol::widmoZiel);

    int ph = szczyt[i] * H_WIDMO / 255;
    if (ph > 0) sprWidmo.fillRect(x, max(0, H_WIDMO - ph - 2), barW, 2, TFT_WHITE);
  }
  // Poziome przerwy - efekt "diodowych" segmentów
  for (int y = 3; y < H_WIDMO; y += 4) sprWidmo.drawFastHLine(0, y, SZER_WIDMA, TFT_BLACK);

  sprWidmo.pushSprite(X_WIDMO, Y_WIDMO);
}

// ---------- Pasek głośności / jasności ----------
// aktywny = enkoder steruje teraz tym paskiem (biała ramka)
inline void rysujPasekGlosnosci(int wartosc, int maks, bool trybJasnosci, bool aktywny = false) {
  tft.fillRect(0, Y_GLOS, E_W, H_GLOS, TFT_BLACK);
  uint16_t c = trybJasnosci ? kol::zolty : kol::zielonyVol;
  if (trybJasnosci) ikonaSlonce(8, Y_GLOS + 3, c);
  else              ikonaGlosnik(10, Y_GLOS + 3, c);

  const int x0 = 32, w = 236;
  uint16_t ramka = aktywny ? TFT_WHITE : kol::szary;
  tft.drawRect(x0, Y_GLOS + 4, w, 12, ramka);
  if (aktywny) tft.drawRect(x0 - 1, Y_GLOS + 3, w + 2, 14, ramka);
  int wyp = (w - 4) * constrain(wartosc, 0, maks) / maks;
  tft.fillRect(x0 + 2, Y_GLOS + 6, wyp, 8, c);

  char buf[8];
  if (trybJasnosci) snprintf(buf, sizeof(buf), "%d%%", wartosc * 100 / maks);
  else              snprintf(buf, sizeof(buf), "%d", wartosc);
  tft.setFont(&fonts::Font2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(middle_right);
  tft.drawString(buf, E_W - 8, Y_GLOS + H_GLOS / 2);
}

// ---------- Przyciski dotykowe ----------
// 0: poprzednia  1: ciszej  2: pauza/graj  3: głośniej  4: następna
inline void rysujPrzycisk(int i, bool pauza) {
  int w  = E_W / 5;
  int x  = i * w + 3, bw = w - 6;
  uint16_t ramka = (i == 2) ? kol::zolty : kol::niebieski;
  uint16_t ik    = (i == 2) ? kol::zolty : kol::jasnyNieb;
  tft.fillRoundRect(x, Y_PRZYC, bw, H_PRZYC, 6, TFT_BLACK);
  tft.drawRoundRect(x, Y_PRZYC, bw, H_PRZYC, 6, ramka);
  int cx = x + bw / 2, cy = Y_PRZYC + H_PRZYC / 2;

  switch (i) {
    case 0:  // |◀
      tft.fillRect(cx - 9, cy - 8, 3, 17, ik);
      tft.fillTriangle(cx + 8, cy - 8, cx + 8, cy + 8, cx - 5, cy, ik);
      break;
    case 1:  // głośnik −
      ikonaGlosnik(cx - 12, cy - 7, ik);
      tft.fillRect(cx + 2, cy - 1, 10, 3, ik);
      break;
    case 2:
      if (pauza) {  // ▶
        tft.fillTriangle(cx - 6, cy - 9, cx - 6, cy + 9, cx + 9, cy, ik);
      } else {      // ||
        tft.fillRect(cx - 7, cy - 8, 5, 17, ik);
        tft.fillRect(cx + 2, cy - 8, 5, 17, ik);
      }
      break;
    case 3:  // głośnik +
      ikonaGlosnik(cx - 12, cy - 7, ik);
      tft.fillRect(cx + 2, cy - 1, 10, 3, ik);
      tft.fillRect(cx + 6, cy - 5, 3, 11, ik);
      break;
    case 4:  // ▶|
      tft.fillRect(cx + 6, cy - 8, 3, 17, ik);
      tft.fillTriangle(cx - 8, cy - 8, cx - 8, cy + 8, cx + 5, cy, ik);
      break;
  }
}

inline int ktoryPrzycisk(int x, int y) {
  if (y < Y_PRZYC - 4) return -1;
  return constrain(x / (E_W / 5), 0, 4);
}

// ---------- Start i rama ----------
inline void startEkranu() {
  sprTytul.setColorDepth(16);
  sprTytul.createSprite(E_W - 16, H_TYTUL);
  sprTytul.setFont(&fonts::FreeSans9pt7b);
  sprTytul.setTextDatum(middle_left);

  sprWidmo.setColorDepth(16);
  sprWidmo.createSprite(SZER_WIDMA, H_WIDMO);
  sprWidmo.fillScreen(TFT_BLACK);
}

inline void rysujRamke(bool pauza) {
  tft.fillScreen(TFT_BLACK);
  tft.drawFastHLine(0, Y_LINIA, E_W, kol::linia);
  for (int i = 0; i < 5; i++) rysujPrzycisk(i, pauza);
}
