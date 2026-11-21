// Analizator widma w stylu Technics na ekranie 2 (284x76 poziomo)
// 15 pasm x 17 segmentów: jasnoniebieskie, 3 górne czerwone, przygaszona siatka,
// czerwony szczyt zawieszany na chwilę. Rysowane są tylko zmienione segmenty.
#pragma once
#include "lgfx_config.h"
#include "ekran.h"          // widmo[] z biblioteki audio

extern LGFX_Pasek ekran2;

const int AN_PASM   = 15;
const int AN_SEG    = 17;
const int AN_CZERW  = 3;          // ile górnych segmentów jest czerwonych
const int AN_SEG_W  = 15, AN_SEG_H = 3;
const uint32_t AN_ZAWIS_MS = 600; // jak długo szczyt wisi
const uint32_t AN_OPAD_MS  = 50;  // co ile szczyt opada o segment

namespace kolAn {
  const uint16_t niebieski  = 0x6E9F;   // #6FD3FF
  const uint16_t czerwony   = 0xF9C6;   // #FF3B30
  const uint16_t siatkaNieb = 0x00C4;   // przygaszone
  const uint16_t siatkaCzer = 0x2820;
}

static uint8_t  anPoziom[AN_PASM];      // aktualna wysokość słupka (segmenty)
static uint8_t  anSzczyt[AN_PASM];
static uint32_t anSzczytCzas[AN_PASM];
static uint8_t  anRys[AN_PASM];         // co jest narysowane
static int8_t   anRysSzczyt[AN_PASM];

inline int anX(int i) { return 3 + i * 1865 / 100; }
inline int anY(int j) { return 76 - 3 - (j + 1) * (AN_SEG_H + 1) + 1; }   // j = 0 na dole

inline uint16_t anKolor(int j, bool zapalony) {
  bool czerw = j >= AN_SEG - AN_CZERW;
  if (zapalony) return czerw ? kolAn::czerwony : kolAn::niebieski;
  return czerw ? kolAn::siatkaCzer : kolAn::siatkaNieb;
}

inline void anSegment(int i, int j, uint16_t c) { ekran2.fillRect(anX(i), anY(j), AN_SEG_W, AN_SEG_H, c); }

inline void startAnalizatora() {
  ekran2.init();
  ekran2.setRotation(1);       // poziomo 284x76
  ekran2.fillScreen(TFT_BLACK);
  ekran2.startWrite();
  for (int i = 0; i < AN_PASM; i++) {
    anPoziom[i] = anSzczyt[i] = anRys[i] = 0;
    anRysSzczyt[i] = -1;
    for (int j = 0; j < AN_SEG; j++) anSegment(i, j, anKolor(j, false));
  }
  ekran2.endWrite();
}

// Jedna klatka (wołać często, sama ogranicza się do ~33 kl/s).
// cisza = true -> słupki opadają do zera (pauza, brak strumienia)
inline void klatkaAnalizatora(bool cisza) {
  static uint32_t t0 = 0;
  uint32_t t = millis();
  if (t - t0 < 30) return;
  t0 = t;

  ekran2.startWrite();
  for (int i = 0; i < AN_PASM; i++) {
    int cel = cisza ? 0 : (widmo[i] * AN_SEG + 127) / 255;
    int poz = anPoziom[i];
    poz = (cel >= poz) ? cel : poz - 1;          // w górę od razu, w dół płynnie
    anPoziom[i] = poz;

    if (poz >= anSzczyt[i]) { anSzczyt[i] = poz; anSzczytCzas[i] = t; }
    else if (anSzczyt[i] > 0 && t - anSzczytCzas[i] > AN_ZAWIS_MS) {
      anSzczyt[i]--;
      anSzczytCzas[i] = t - AN_ZAWIS_MS + AN_OPAD_MS;
    }

    // Słupek: dorysuj albo zgaś tylko różnicę
    int stary = anRys[i];
    if (poz > stary)      for (int j = stary; j < poz; j++)  anSegment(i, j, anKolor(j, true));
    else if (poz < stary) for (int j = poz; j < stary; j++)  anSegment(i, j, anKolor(j, false));
    anRys[i] = poz;

    // Szczyt nad słupkiem
    int sp = (anSzczyt[i] > poz) ? anSzczyt[i] - 1 : -1;
    if (sp != anRysSzczyt[i]) {
      int stare = anRysSzczyt[i];
      if (stare >= poz && stare >= 0) anSegment(i, stare, anKolor(stare, false));
      if (sp >= 0) anSegment(i, sp, kolAn::czerwony);
      anRysSzczyt[i] = sp;
    }
  }
  ekran2.endWrite();
}
