// 10-pasmowy korektor graficzny (31 Hz - 16 kHz, ±12 dB)
// Część DSP działa w zadaniu audio (audio_process_raw_samples), część ekranowa w zakładce KOREKTOR.
// Ustawienia zapisywane w pamięci (Preferences) i przywracane po starcie.
#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "Audio.h"
#include "ekran.h"

extern Audio audio;

const int   EQ_PASM = 10;
const float EQ_F[EQ_PASM]       = {31, 62, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};
const char* EQ_OPIS[EQ_PASM]    = {"31", "62", "125", "250", "500", "1k", "2k", "4k", "8k", "16k"};
const int   EQ_MAX = 12;           // ±12 dB

struct PresetEQ { const char* nazwa; int8_t g[EQ_PASM]; };
const PresetEQ EQ_PRESETY[] = {
  {"Plasko", { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0}},
  {"Rock",   { 5,  4,  3,  1, -1, -1,  1,  3,  4,  5}},
  {"Pop",    {-1,  1,  3,  4,  3,  0, -1, -1,  1,  2}},
  {"Jazz",   { 3,  2,  1,  2, -1, -1,  0,  1,  2,  3}},
  {"Bas+",   { 7,  6,  4,  2,  0,  0,  0,  0,  0,  0}},
};
const int EQ_LICZBA_PRESETOW = sizeof(EQ_PRESETY) / sizeof(EQ_PRESETY[0]);

int8_t        eqZysk[EQ_PASM] = {0};
int           eqPreset   = 0;      // -1 = własne
int           eqPasmo    = 2;      // wybrane pasmo (enkoder / ekran)
bool          eqEdycja   = false;  // enkoder 1: false = wybór pasma, true = zmiana wartości
volatile bool eqZmiana   = true;   // DSP ma przeliczyć filtry
uint32_t      eqZapisz   = 0;      // kiedy zapisać do pamięci (0 = nie trzeba)

// ---------- DSP (zadanie audio) ----------
struct Biquad {
  float b0, b1, b2, a1, a2;
  float x1[2], x2[2], y1[2], y2[2];
  bool  aktywny;
};
static Biquad   eqF[EQ_PASM];
static float    eqPre = 1.0f;
static uint32_t eqFs = 0;
static bool     eqWlaczony = false;

inline int eqPreDb() {                 // wzmocnienie wstępne chroniące przed przesterowaniem
  int maxG = 0;
  for (int i = 0; i < EQ_PASM; i++) maxG = max(maxG, (int)eqZysk[i]);
  return -(maxG * 3 + 2) / 4;          // ok. -0,75 * największe podbicie
}

static void eqPrzelicz(uint32_t fs) {
  eqPre = powf(10.0f, eqPreDb() / 20.0f);
  eqWlaczony = false;
  for (int i = 0; i < EQ_PASM; i++) {
    Biquad& f = eqF[i];
    int g = eqZysk[i];
    f.aktywny = (g != 0) && (EQ_F[i] < fs * 0.45f);
    memset(f.x1, 0, sizeof(f.x1)); memset(f.x2, 0, sizeof(f.x2));
    memset(f.y1, 0, sizeof(f.y1)); memset(f.y2, 0, sizeof(f.y2));
    if (!f.aktywny) continue;
    eqWlaczony = true;
    // Filtr "peaking EQ" (R. Bristow-Johnson), szerokość ok. 1 oktawy
    float A  = powf(10.0f, g / 40.0f);
    float w0 = 2.0f * PI * EQ_F[i] / fs;
    float al = sinf(w0) / (2.0f * 1.41f);
    float c  = cosf(w0);
    float a0 = 1.0f + al / A;
    f.b0 = (1.0f + al * A) / a0;
    f.b1 = (-2.0f * c) / a0;
    f.b2 = (1.0f - al * A) / a0;
    f.a1 = (-2.0f * c) / a0;
    f.a2 = (1.0f - al / A) / a0;
  }
}

// Wywoływane przez bibliotekę audio dla każdej porcji próbek (przed głośnością, VU i widmem)
void audio_process_raw_samples(int32_t* buf, int16_t n) {
  uint32_t fs = audio.getSampleRate();
  if (fs == 0) return;
  if (eqZmiana || fs != eqFs) { eqZmiana = false; eqFs = fs; eqPrzelicz(fs); }
  if (!eqWlaczony) return;

  for (int i = 0; i < n / 2; i++) {
    for (int ch = 0; ch < 2; ch++) {
      float x = buf[2 * i + ch] * eqPre;
      for (int b = 0; b < EQ_PASM; b++) {
        Biquad& f = eqF[b];
        if (!f.aktywny) continue;
        float y = f.b0 * x + f.b1 * f.x1[ch] + f.b2 * f.x2[ch] - f.a1 * f.y1[ch] - f.a2 * f.y2[ch];
        f.x2[ch] = f.x1[ch]; f.x1[ch] = x;
        f.y2[ch] = f.y1[ch]; f.y1[ch] = y;
        x = y;
      }
      if (x > 2147483520.0f) x = 2147483520.0f;
      if (x < -2147483520.0f) x = -2147483520.0f;
      buf[2 * i + ch] = (int32_t)x;
    }
  }
}

// ---------- Ustawienia ----------
inline void eqUstawPreset(int p) {
  if (p < 0 || p >= EQ_LICZBA_PRESETOW) return;
  eqPreset = p;
  memcpy(eqZysk, EQ_PRESETY[p].g, EQ_PASM);
  eqZmiana = true;
  eqZapisz = millis() + 2000;
}

inline void eqUstawPasmo(int pasmo, int db) {
  eqZysk[pasmo] = constrain(db, -EQ_MAX, EQ_MAX);
  eqPreset = -1;
  eqZmiana = true;
  eqZapisz = millis() + 2000;
}

inline void eqWczytaj() {
  Preferences p;
  p.begin("sparrow", true);
  eqPreset = p.getInt("eqPreset", 0);
  if (p.getBytes("eqZysk", eqZysk, EQ_PASM) != EQ_PASM) memset(eqZysk, 0, EQ_PASM);
  p.end();
  eqZmiana = true;
}

inline void eqZapiszJesliTrzeba() {      // zapis do pamięci 2 s po ostatniej zmianie
  if (!eqZapisz || (int32_t)(millis() - eqZapisz) < 0) return;
  eqZapisz = 0;
  Preferences p;
  p.begin("sparrow", false);
  p.putInt("eqPreset", eqPreset);
  p.putBytes("eqZysk", eqZysk, EQ_PASM);
  p.end();
  Serial.println("Korektor zapisany");
}

// ---------- Zakładka KOREKTOR ----------
const int EQ_X0 = 30;
const float EQ_W = 28.4f;
const int EQ_YC = 98, EQ_YG = 46, EQ_YD = 150;     // zero, góra, dół suwaków
const float EQ_PIX_DB = 50.0f / EQ_MAX;
const int EQ_PR_Y = 176, EQ_PR_H = 24, EQ_PR_W = 56, EQ_PR_KROK = 62;

inline int eqCx(int i) { return EQ_X0 + (int)(i * EQ_W + EQ_W / 2); }

inline void rysujNaglowekEQ() {
  tft.fillRect(0, 24, E_W, 20, TFT_BLACK);
  tft.setFont(&fonts::Font2);
  tft.setTextDatum(middle_left);
  tft.setTextColor(kol::morski);
  tft.drawString("Korektor", 8, 34);
  tft.setFont(&fonts::FreeSansBold9pt7b);
  tft.setTextColor(kol::zolty);
  tft.drawString(eqPreset >= 0 ? EQ_PRESETY[eqPreset].nazwa : "Wlasne", 70, 34);
  char buf[24];
  snprintf(buf, sizeof(buf), "%s Hz  %+d dB", EQ_OPIS[eqPasmo], eqZysk[eqPasmo]);
  tft.setFont(&fonts::Font2);
  tft.setTextColor(kol::jasnyNieb);
  tft.setTextDatum(middle_center);
  tft.drawString(buf, 200, 34);
  snprintf(buf, sizeof(buf), "PRE %d", eqPreDb());
  tft.setTextColor(kol::szary);
  tft.setTextDatum(middle_right);
  tft.drawString(buf, E_W - 6, 34);
}

inline void rysujSuwakiEQ() {
  tft.fillRect(0, 44, E_W, 130, TFT_BLACK);
  tft.setFont(&fonts::Font2);
  tft.setTextColor(kol::szary);
  tft.setTextDatum(middle_right);
  tft.drawString("+12", 26, EQ_YG + 2);
  tft.drawString("0", 26, EQ_YC);
  tft.drawString("-12", 26, EQ_YD - 2);
  for (int x = 28; x < E_W - 4; x += 6) tft.drawFastHLine(x, EQ_YC, 3, kol::linia);

  int px = 0, py = 0;
  for (int i = 0; i < EQ_PASM; i++) {
    int cx = eqCx(i);
    bool akt = (i == eqPasmo);
    int ky = EQ_YC - (int)(eqZysk[i] * EQ_PIX_DB);
    tft.fillRect(cx - 1, EQ_YG, 2, EQ_YD - EQ_YG, kol::wifiOff);
    tft.fillRect(cx - 1, min(ky, EQ_YC), 3, abs(ky - EQ_YC) + 1, akt ? kol::widmoPom : 0x1AF4);
    tft.fillRoundRect(cx - 8, ky - 4, 16, 8, 2, akt ? kol::zolty : kol::niebieski);
    if (akt) {
      uint16_t r = eqEdycja ? kol::zolty : TFT_WHITE;
      tft.drawRect(cx - 13, EQ_YG - 2, 26, EQ_YD - EQ_YG + 4, r);
    }
    if (i > 0) tft.drawLine(px, py, cx, ky, kol::wifiOn);
    px = cx; py = ky;
    tft.setTextColor(akt ? kol::zolty : kol::szary);
    tft.setTextDatum(middle_center);
    tft.drawString(EQ_OPIS[i], cx, 166);
  }
}

inline void rysujPresetyEQ() {
  tft.fillRect(0, EQ_PR_Y - 2, E_W, EQ_PR_H + 4, TFT_BLACK);
  tft.setFont(&fonts::Font2);
  tft.setTextDatum(middle_center);
  for (int i = 0; i < EQ_LICZBA_PRESETOW && i < 5; i++) {
    int x = 6 + i * EQ_PR_KROK;
    bool akt = (i == eqPreset);
    if (akt) tft.fillRoundRect(x, EQ_PR_Y, EQ_PR_W, EQ_PR_H, 5, kol::pasek);
    tft.drawRoundRect(x, EQ_PR_Y, EQ_PR_W, EQ_PR_H, 5, akt ? kol::zolty : kol::niebieski);
    tft.setTextColor(akt ? kol::zolty : kol::jasnyNieb);
    tft.drawString(EQ_PRESETY[i].nazwa, x + EQ_PR_W / 2, EQ_PR_Y + EQ_PR_H / 2);
  }
}

inline void rysujKorektor() {
  wyczyscTresc();
  rysujNaglowekEQ();
  rysujSuwakiEQ();
  rysujPresetyEQ();
}

// Dotyk: 1 = suwak (pasmo, db), 2 = preset (pasmo = nr presetu), 0 = nic
inline int korektorDotyk(int x, int y, int& pasmo, int& db) {
  if (y >= EQ_YG - 8 && y <= EQ_YD + 20 && x >= EQ_X0 - 4) {
    pasmo = constrain((int)((x - EQ_X0) / EQ_W), 0, EQ_PASM - 1);
    db = constrain((int)lroundf((EQ_YC - y) / EQ_PIX_DB), -EQ_MAX, EQ_MAX);
    return 1;
  }
  if (y >= EQ_PR_Y - 3 && y <= EQ_PR_Y + EQ_PR_H + 3) {
    int i = (x - 6) / EQ_PR_KROK;
    if (i >= 0 && i < EQ_LICZBA_PRESETOW && (x - 6) % EQ_PR_KROK < EQ_PR_W) { pasmo = i; return 2; }
  }
  return 0;
}
