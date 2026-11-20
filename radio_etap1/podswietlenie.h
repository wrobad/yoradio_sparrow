// Sterowanie podświetleniem wyświetlacza (PWM przez LEDC rdzenia Arduino ESP32 3.x/4.x)
#pragma once
#include <Arduino.h>
#include "lgfx_config.h"   // PIN_TFT_BL

#define BL_FREQ      5000   // Hz
#define BL_ROZDZ     8      // bitów -> jasność 0..255

static bool    blPwmOk  = false;
static uint8_t blJasnosc = 200;

inline void ustawJasnosc(int j) {
  blJasnosc = constrain(j, 0, 255);
  if (blPwmOk) {
    ledcWrite(PIN_TFT_BL, blJasnosc);
  } else {
    digitalWrite(PIN_TFT_BL, blJasnosc > 0 ? HIGH : LOW);  // bez PWM: tylko wł./wył.
  }
}

inline void startPodswietlenia(int jasnosc = 200) {
  blPwmOk = ledcAttach(PIN_TFT_BL, BL_FREQ, BL_ROZDZ);
  if (!blPwmOk) {
    Serial.println("Podswietlenie: PWM niedostepny, tryb wl./wyl.");
    pinMode(PIN_TFT_BL, OUTPUT);
  } else {
    Serial.println("Podswietlenie: PWM OK");
  }
  ustawJasnosc(jasnosc);
}
