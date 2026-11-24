// Dioda RGB na płytce (WS2812, pin PIN_RGB) jako wskaźnik stanu radia
//
//   zielona                - radio gra
//   żółta                  - gra, ale bufor prawie pusty (słaby internet / przerwy)
//   żółta migająca         - łączenie z Wi-Fi albo ze stacją
//   czerwona               - brak Wi-Fi
//   czerwona migająca      - stacja nie odpowiada (brak dźwięku mimo prób)
//   niebieska              - pauza
//   fioletowa              - wyciszone
//   zgaszona               - uśpione (wyłącznik czasowy)
#pragma once
#include <Arduino.h>
#include "piny.h"

#define DIODA_JASNOSC  20      // 0..255 - dioda jest bardzo jasna, wystarczy niewiele

static uint32_t diodaKolor = 0xFFFFFFFF;

inline void diodaUstaw(uint8_t r, uint8_t g, uint8_t b) {
  uint32_t k = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
  if (k == diodaKolor) return;                 // wysyłamy tylko przy zmianie
  diodaKolor = k;
  rgbLedWrite(PIN_RGB, r * DIODA_JASNOSC / 255, g * DIODA_JASNOSC / 255, b * DIODA_JASNOSC / 255);
}

enum StanDiody { D_GRA, D_SLABY_BUFOR, D_LACZENIE, D_BRAK_WIFI, D_BLAD_STACJI, D_PAUZA, D_WYCISZONE, D_USPIONE };

inline void diodaPokaz(StanDiody s) {
  bool miga = (millis() / 400) % 2;            // miganie co 0,4 s
  switch (s) {
    case D_GRA:         diodaUstaw(0, 255, 0);   break;
    case D_SLABY_BUFOR: diodaUstaw(255, 160, 0); break;
    case D_LACZENIE:    miga ? diodaUstaw(255, 160, 0) : diodaUstaw(0, 0, 0); break;
    case D_BRAK_WIFI:   diodaUstaw(255, 0, 0);   break;
    case D_BLAD_STACJI: miga ? diodaUstaw(255, 0, 0) : diodaUstaw(0, 0, 0); break;
    case D_PAUZA:       diodaUstaw(0, 0, 255);   break;
    case D_WYCISZONE:   diodaUstaw(160, 0, 255); break;
    case D_USPIONE:     diodaUstaw(0, 0, 0);     break;
  }
}
