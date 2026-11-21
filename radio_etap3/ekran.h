// Wspólne elementy ekranu (320x240 poziomo): kolory, pasek stanu, pasek zakładek,
// okienko (toast), ekran startowy i zakładka RADIO.
#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <vector>
#include "lgfx_config.h"

extern LGFX tft;

// ---------- Kolory (RGB565) ----------
namespace kol {
  const uint16_t pasek      = 0x1105;  // granatowy pasek stanu / podświetlenie
  const uint16_t naglowek   = 0x08C4;  // tło nagłówków grup na liście
  const uint16_t zolty      = 0xFE2E;
  const uint16_t niebieski  = 0x345B;
  const uint16_t jasnyNieb  = 0x85BD;
  const uint16_t morski     = 0x9F19;
  const uint16_t zielonyVol = 0x64C4;
  const uint16_t jasnyZiel  = 0x962B;
  const uint16_t widmoZiel  = 0x1CEE;
  const uint16_t widmoPom   = 0xECE4;
  const uint16_t widmoCzer  = 0xE249;
  const uint16_t szary      = 0x5AEB;
  const uint16_t linia      = 0x2965;
  const uint16_t tekst      = 0xE73C;  // jasnoszary tekst
  const uint16_t wifiOff    = 0x4228;
  const uint16_t wifiOn     = 0x5E54;
  const uint16_t krem       = 0xF718;  // tarcza VU
  const uint16_t bursztyn   = 0xBBA2;
  const uint16_t ciemnyCzer = 0xA165;
  const uint16_t bordo      = 0x78E3;
}

// ---------- Układ ekranu ----------
const int E_W       = 320;
const int H_PASEK   = 20;
const int Y_TRESC   = 20;                 // obszar zakładki: 20..205
const int H_TRESC   = 186;
const int Y_ZAKL    = 206, H_ZAKL = 34;   // pasek zakładek

// Zakładka RADIO
const int Y_STACJA  = 24,  H_STACJA = 26;
const int Y_TYTUL   = 54,  H_TYTUL  = 18;
const int Y_WIDMO   = 78,  H_WIDMO  = 60;
const int X_WIDMO   = 10,  SZER_WIDMA = 300;
const int Y_GLOS    = 141, H_GLOS   = 16;
const int Y_PRZYC   = 164, H_PRZYC  = 34;
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
  if (c == 0x0EB) return 'e';                     // ë (Kaszëbë)
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

// ---------- Ikony (rysowane prymitywami) ----------
inline void ikonaGlosnik(int x, int y, uint16_t c) {   // ok. 10x14, (x,y) = lewy górny róg
  tft.fillRect(x, y + 4, 4, 6, c);
  tft.fillTriangle(x + 3, y + 7, x + 10, y, x + 10, y + 14, c);
}
inline void ikonaSlonce(int x, int y, uint16_t c) {    // ok. 14x14
  tft.fillCircle(x + 7, y + 7, 3, c);
  tft.drawCircle(x + 7, y + 7, 6, c);
}

// Ikony zakładek, środek (cx, cy), ok. 20x20
// 0 radio, 1 lista, 2 ulubione, 3 skala, 4 korektor, 5 VU, 6 zegar, 7 ustawienia
inline void ikonaZakladki(int z, int cx, int cy, uint16_t c) {
  switch (z) {
    case 0: // radio
      tft.drawRect(cx - 9, cy - 3, 19, 12, c); tft.drawRect(cx - 8, cy - 2, 17, 10, c);
      tft.drawCircle(cx - 3, cy + 3, 2, c);
      tft.drawFastHLine(cx + 2, cy + 1, 5, c); tft.drawFastHLine(cx + 2, cy + 5, 5, c);
      tft.drawLine(cx - 5, cy - 3, cx + 4, cy - 9, c); tft.drawLine(cx - 4, cy - 3, cx + 5, cy - 9, c);
      break;
    case 1: // lista
      for (int i = -1; i <= 1; i++) {
        tft.fillCircle(cx - 7, cy + i * 6, 1, c);
        tft.fillRect(cx - 4, cy + i * 6 - 1, 12, 2, c);
      }
      break;
    case 2: { // gwiazdka
      float px[10], py[10];
      for (int i = 0; i < 10; i++) {
        float a = (-90 + i * 36) * DEG_TO_RAD;
        float r = (i % 2 == 0) ? 9 : 3.8;
        px[i] = cx + r * cosf(a); py[i] = cy + 1 + r * sinf(a);
      }
      for (int i = 0; i < 10; i++) { int j = (i + 1) % 10; tft.drawLine(px[i], py[i], px[j], py[j], c); }
      break;
    }
    case 3: // skala: podziałka + czerwona wskazówka
      tft.fillRect(cx - 10, cy + 4, 21, 2, c);
      for (int i = -8; i <= 8; i += 4) tft.fillRect(cx + i, cy + ((i / 4) % 2 ? 0 : -2), 1, ((i / 4) % 2 ? 4 : 6), c);
      tft.fillRect(cx + 1, cy - 8, 2, 17, kol::widmoCzer);
      break;
    case 4: // korektor: pionowe suwaki
      for (int i = -1; i <= 1; i++) {
        int x = cx + i * 6;
        tft.fillRect(x, cy - 9, 1, 19, c);
        int y = cy + (i == -1 ? -4 : i == 0 ? 3 : -1);
        tft.fillRoundRect(x - 2, y - 1, 5, 3, 1, c);
      }
      break;
    case 5: // VU
      tft.fillArc(cx, cy + 6, 10, 9, 180, 360, c);
      tft.drawWideLine(cx, cy + 6, cx + 5, cy - 3, 1.0f, c);
      tft.fillCircle(cx, cy + 6, 2, c);
      break;
    case 6: // zegar
      tft.drawCircle(cx, cy, 9, c); tft.drawCircle(cx, cy, 8, c);
      tft.drawWideLine(cx, cy, cx, cy - 5, 0.8f, c);
      tft.drawWideLine(cx, cy, cx + 4, cy + 2, 0.8f, c);
      break;
    case 7: // ustawienia: koło zębate
      for (int i = 0; i < 8; i++) {
        float a = i * 45 * DEG_TO_RAD;
        tft.fillCircle(cx + 7.5f * cosf(a), cy + 7.5f * sinf(a), 2, c);
      }
      tft.fillCircle(cx, cy, 6, c);
      tft.fillCircle(cx, cy, 3, TFT_BLACK);
      break;
  }
}

// ---------- Pasek zakładek ----------
const int LICZBA_IKON = 8;
inline int xZakladki(int i) { return i * E_W / LICZBA_IKON; }

inline void rysujPasekZakladek(int aktywna) {
  tft.fillRect(0, Y_ZAKL, E_W, H_ZAKL, TFT_BLACK);
  tft.drawFastHLine(0, Y_ZAKL, E_W, kol::linia);
  for (int i = 0; i < LICZBA_IKON; i++) {
    int x = xZakladki(i), w = xZakladki(i + 1) - x;
    bool akt = (i == aktywna);
    if (akt) {
      tft.fillRect(x + 1, Y_ZAKL + 1, w - 2, H_ZAKL - 1, kol::pasek);
      tft.fillRect(x + 1, Y_ZAKL + 1, w - 2, 2, kol::zolty);
    }
    ikonaZakladki(i, x + w / 2, Y_ZAKL + 18, akt ? kol::zolty : kol::niebieski);
  }
}

inline int ktoraZakladka(int x, int y) {       // -1 = poza paskiem
  if (y < Y_ZAKL - 4) return -1;
  return constrain(x * LICZBA_IKON / E_W, 0, LICZBA_IKON - 1);
}

// ---------- Pasek stanu ----------
inline void rysujPasekStanu(int nr, int ile, const char* kodek, uint32_t bitrate, bool pauza, int usypianieZostalo, bool wycisz = false) {
  char buf[24];
  tft.fillRect(0, 0, E_W, H_PASEK, kol::pasek);
  tft.setFont(&fonts::Font2);

  struct tm t;
  if (getLocalTime(&t, 0)) strftime(buf, sizeof(buf), "%H:%M", &t);
  else strcpy(buf, "--:--");
  tft.setTextColor(TFT_WHITE, kol::pasek);
  tft.setTextDatum(middle_left);
  tft.drawString(buf, 6, H_PASEK / 2);

  if (wycisz)           snprintf(buf, sizeof(buf), "WYCISZONE");
  else if (pauza)       snprintf(buf, sizeof(buf), "PAUZA");
  else if (bitrate > 0) snprintf(buf, sizeof(buf), "%s %uk", kodek ? kodek : "", (unsigned)(bitrate / 1000));
  else                  snprintf(buf, sizeof(buf), "%s", kodek ? kodek : "");
  tft.setTextColor((pauza || wycisz) ? kol::zolty : kol::jasnyNieb, kol::pasek);
  tft.setTextDatum(middle_center);
  tft.drawString(buf, 130, H_PASEK / 2);

  if (usypianieZostalo > 0) {                    // wyłącznik czasowy
    snprintf(buf, sizeof(buf), "Zz%d", usypianieZostalo);
    tft.setTextColor(kol::zolty, kol::pasek);
    tft.drawString(buf, 195, H_PASEK / 2);
  }

  snprintf(buf, sizeof(buf), "%d/%d", nr + 1, ile);
  tft.setTextColor(kol::morski, kol::pasek);
  tft.drawString(buf, 250, H_PASEK / 2);

  int rssi = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : -127;
  int kreski = rssi > -55 ? 4 : rssi > -65 ? 3 : rssi > -75 ? 2 : rssi > -85 ? 1 : 0;
  for (int i = 0; i < 4; i++) {
    int h = 4 + i * 4;
    tft.fillRect(290 + i * 6, 17 - h, 4, h, i < kreski ? kol::wifiOn : kol::wifiOff);
  }
}

inline void wyczyscTresc() { tft.fillRect(0, Y_TRESC, E_W, H_TRESC, TFT_BLACK); }

// ---------- Okienko (toast) na wierzchu zakładki ----------
const int TOAST_X = 30, TOAST_Y = 76, TOAST_W = 260, TOAST_H = 64;

inline void toastRamka(uint16_t kolorRamki) {
  tft.fillRoundRect(TOAST_X, TOAST_Y, TOAST_W, TOAST_H, 8, TFT_BLACK);
  tft.drawRoundRect(TOAST_X, TOAST_Y, TOAST_W, TOAST_H, 8, kolorRamki);
  tft.drawRoundRect(TOAST_X + 1, TOAST_Y + 1, TOAST_W - 2, TOAST_H - 2, 7, kolorRamki);
}

inline void toastStacja(int nr, int ile, const char* nazwa) {
  toastRamka(kol::zolty);
  char buf[12];
  snprintf(buf, sizeof(buf), "%d/%d", nr + 1, ile);
  tft.setFont(&fonts::Font2);
  tft.setTextColor(kol::morski);
  tft.setTextDatum(top_center);
  tft.drawString(buf, E_W / 2, TOAST_Y + 8);
  tft.setFont(&fonts::FreeSansBold12pt7b);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(middle_center);
  tft.setClipRect(TOAST_X + 6, TOAST_Y, TOAST_W - 12, TOAST_H);
  tft.drawString(bezOgonkow(nazwa), E_W / 2, TOAST_Y + 40);
  tft.clearClipRect();
}

inline void toastPasek(int wartosc, int maks, bool jasnosc) {
  toastRamka(jasnosc ? kol::zolty : TFT_WHITE);
  uint16_t c = jasnosc ? kol::zolty : kol::zielonyVol;
  tft.setFont(&fonts::Font2);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(top_left);
  tft.drawString(jasnosc ? "Jasnosc" : "Glosnosc", TOAST_X + 14, TOAST_Y + 8);
  char buf[8];
  if (jasnosc) snprintf(buf, sizeof(buf), "%d%%", wartosc * 100 / maks);
  else         snprintf(buf, sizeof(buf), "%d", wartosc);
  tft.setTextDatum(top_right);
  tft.drawString(buf, TOAST_X + TOAST_W - 14, TOAST_Y + 8);
  if (jasnosc) ikonaSlonce(TOAST_X + 12, TOAST_Y + 33, c);
  else         ikonaGlosnik(TOAST_X + 14, TOAST_Y + 33, c);
  int x0 = TOAST_X + 34, w = TOAST_W - 50;
  tft.drawRect(x0, TOAST_Y + 34, w, 12, kol::szary);
  tft.fillRect(x0 + 2, TOAST_Y + 36, (w - 4) * constrain(wartosc, 0, maks) / maks, 8, c);
}

// =====================================================================
//                          ZAKŁADKA RADIO
// =====================================================================

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
static uint32_t tytulStop  = 0;
const  int      TYTUL_ODSTEP = 40;

inline void rysujTytulKlatka() {
  sprTytul.fillScreen(TFT_BLACK);
  sprTytul.setTextColor(kol::tekst);
  int y = H_TYTUL / 2;
  if (tytulSzer <= sprTytul.width()) {
    sprTytul.drawString(tytulTxt, 0, y);
  } else {
    sprTytul.drawString(tytulTxt, -tytulPoz, y);
    sprTytul.drawString(tytulTxt, -tytulPoz + tytulSzer + TYTUL_ODSTEP, y);
  }
  sprTytul.pushSprite(8, Y_TYTUL);
}

inline void ustawTytul(const String& t, bool rysuj) {
  tytulTxt  = bezOgonkow(t);
  tytulSzer = sprTytul.textWidth(tytulTxt);
  tytulPoz  = 0;
  tytulStop = millis() + 2000;
  if (rysuj) rysujTytulKlatka();
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

inline void rysujWidmo(bool wymus = false) {
  static uint32_t t0 = 0;
  if (!wymus && (!widmoNowe || millis() - t0 < 33)) return;   // max ~30 klatek/s
  t0 = millis();
  widmoNowe = false;

  sprWidmo.fillScreen(TFT_BLACK);
  const int kolW = SZER_WIDMA / LICZBA_PASM;   // 20 px na pasmo
  const int barW = kolW - 4;
  const int strC = H_WIDMO * 20 / 100;
  const int strP = H_WIDMO * 45 / 100;

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
  for (int y = 3; y < H_WIDMO; y += 4) sprWidmo.drawFastHLine(0, y, SZER_WIDMA, TFT_BLACK);
  sprWidmo.pushSprite(X_WIDMO, Y_WIDMO);
}

// ---------- Pasek głośności / jasności ----------
// aktywny = enkoder steruje teraz tym paskiem (biała ramka)
inline void rysujPasekGlosnosci(int wartosc, int maks, bool trybJasnosci, bool aktywny = false) {
  tft.fillRect(0, Y_GLOS, E_W, H_GLOS, TFT_BLACK);
  uint16_t c = trybJasnosci ? kol::zolty : kol::jasnyZiel;
  if (trybJasnosci) ikonaSlonce(8, Y_GLOS + 1, c);
  else              ikonaGlosnik(10, Y_GLOS + 1, c);

  const int x0 = 30, w = 238;
  uint16_t ramka = aktywny ? TFT_WHITE : kol::szary;
  tft.drawRect(x0, Y_GLOS + 2, w, 11, ramka);
  if (aktywny) tft.drawRect(x0 - 1, Y_GLOS + 1, w + 2, 13, ramka);
  int wyp = (w - 4) * constrain(wartosc, 0, maks) / maks;
  tft.fillRect(x0 + 2, Y_GLOS + 4, wyp, 7, trybJasnosci ? kol::zolty : kol::zielonyVol);

  char buf[8];
  if (trybJasnosci) snprintf(buf, sizeof(buf), "%d%%", wartosc * 100 / maks);
  else              snprintf(buf, sizeof(buf), "%d", wartosc);
  tft.setFont(&fonts::Font2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(middle_right);
  tft.drawString(buf, E_W - 8, Y_GLOS + H_GLOS / 2);
}

// ---------- Przyciski sterowania ----------
// 0: poprzednia  1: ciszej  2: pauza/graj  3: głośniej  4: następna
const int X_PRZYC[5] = {8, 70, 132, 194, 256};
const int W_PRZYC = 56;

inline void rysujPrzycisk(int i, bool pauza) {
  int x = X_PRZYC[i];
  uint16_t ramka = (i == 2) ? kol::zolty : kol::niebieski;
  uint16_t ik    = (i == 2) ? kol::zolty : kol::jasnyNieb;
  tft.fillRoundRect(x, Y_PRZYC, W_PRZYC, H_PRZYC, 5, TFT_BLACK);
  tft.drawRoundRect(x, Y_PRZYC, W_PRZYC, H_PRZYC, 5, ramka);
  int cx = x + W_PRZYC / 2, cy = Y_PRZYC + H_PRZYC / 2;

  switch (i) {
    case 0:
      tft.fillRect(cx - 8, cy - 7, 3, 15, ik);
      tft.fillTriangle(cx + 7, cy - 7, cx + 7, cy + 7, cx - 4, cy, ik);
      break;
    case 1:
      ikonaGlosnik(cx - 11, cy - 7, ik);
      tft.fillRect(cx + 2, cy - 1, 9, 3, ik);
      break;
    case 2:
      if (pauza) tft.fillTriangle(cx - 5, cy - 8, cx - 5, cy + 8, cx + 8, cy, ik);
      else { tft.fillRect(cx - 6, cy - 7, 5, 15, ik); tft.fillRect(cx + 2, cy - 7, 5, 15, ik); }
      break;
    case 3:
      ikonaGlosnik(cx - 11, cy - 7, ik);
      tft.fillRect(cx + 2, cy - 1, 9, 3, ik);
      tft.fillRect(cx + 5, cy - 4, 3, 9, ik);
      break;
    case 4:
      tft.fillRect(cx + 6, cy - 7, 3, 15, ik);
      tft.fillTriangle(cx - 7, cy - 7, cx - 7, cy + 7, cx + 4, cy, ik);
      break;
  }
}

inline int ktoryPrzycisk(int x, int y) {
  if (y < Y_PRZYC - 4 || y > Y_PRZYC + H_PRZYC + 3) return -1;
  for (int i = 0; i < 5; i++) if (x >= X_PRZYC[i] - 3 && x < X_PRZYC[i] + W_PRZYC + 3) return i;
  return -1;
}

// ---------- Start ----------
inline void startEkranu() {
  sprTytul.setColorDepth(16);
  sprTytul.createSprite(E_W - 16, H_TYTUL);
  sprTytul.setFont(&fonts::FreeSans9pt7b);
  sprTytul.setTextDatum(middle_left);

  sprWidmo.setColorDepth(16);
  sprWidmo.createSprite(SZER_WIDMA, H_WIDMO);
  sprWidmo.fillScreen(TFT_BLACK);
}

// ---------- Ekran startowy "RADIO SPARROW SYSTEM" ----------
const int Y_EQ_START = 152, H_EQ_START = 36, LICZBA_EQ_START = 21;
static uint8_t eqStart[LICZBA_EQ_START];

inline void ekranStartowy(const char* wersja) {
  tft.fillScreen(TFT_BLACK);
  tft.drawRoundRect(6, 6, E_W - 12, 228, 10, kol::niebieski);

  tft.setTextDatum(middle_center);
  tft.setFont(&fonts::FreeSansBold9pt7b);
  tft.setTextColor(kol::jasnyNieb);
  tft.drawString("R  A  D  I  O", E_W / 2, 46);
  tft.drawString("S  Y  S  T  E  M", E_W / 2, 132);
  tft.drawFastHLine(30, 46, 75, kol::niebieski);
  tft.drawFastHLine(E_W - 105, 46, 75, kol::niebieski);
  tft.drawFastHLine(30, 132, 60, kol::niebieski);
  tft.drawFastHLine(E_W - 90, 132, 60, kol::niebieski);

  tft.setFont(&fonts::FreeSansBold24pt7b);
  tft.setTextColor(kol::zolty);
  tft.drawString("SPARROW", E_W / 2, 89);

  tft.setFont(&fonts::Font0);
  tft.setTextColor(kol::szary);
  tft.setTextDatum(bottom_right);
  tft.drawString(wersja, E_W - 14, 226);

  memset(eqStart, 0, sizeof(eqStart));
}

inline void klatkaStartowa() {
  const int bw = 8, odst = 4;
  const int x0 = (E_W - LICZBA_EQ_START * (bw + odst) + odst) / 2;
  for (int i = 0; i < LICZBA_EQ_START; i++) {
    int cel = random(4, H_EQ_START);
    eqStart[i] = (eqStart[i] * 2 + cel) / 3;
    int h = eqStart[i];
    int x = x0 + i * (bw + odst);
    tft.fillRect(x, Y_EQ_START, bw, H_EQ_START - h, TFT_BLACK);
    for (int y = 0; y < h; y += 3) {
      int yy = Y_EQ_START + H_EQ_START - y - 2;
      uint16_t c = y > H_EQ_START * 75 / 100 ? kol::widmoCzer
                 : y > H_EQ_START * 45 / 100 ? kol::widmoPom : kol::widmoZiel;
      tft.fillRect(x, yy, bw, 2, c);
    }
  }
}

inline void napisStartowy(const String& txt) {
  tft.fillRect(14, 196, E_W - 80, 22, TFT_BLACK);
  tft.setFont(&fonts::Font2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(middle_left);
  tft.drawString(txt, 18, 207);
}
