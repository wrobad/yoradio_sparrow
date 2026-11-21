// Podświetlenie obu wyświetlaczy (PWM przez LEDC rdzenia Arduino ESP32 3.x/4.x)
// Ekran główny: GPIO 8, świeci stanem wysokim.
// Ekran 2 (podłużny): GPIO 47, świeci stanem NISKIM - wypełnienie odwrócone.
#pragma once
#include <Arduino.h>
#include "lgfx_config.h"   // PIN_TFT_BL, P2_BL

#define BL_FREQ      5000   // Hz
#define BL_ROZDZ     8      // bitów -> jasność 0..255

static bool    blPwmOk  = false, bl2PwmOk = false;
static uint8_t blJasnosc = 200;

inline void ustawJasnosc(int j) {
  blJasnosc = constrain(j, 0, 255);
  if (blPwmOk)  ledcWrite(PIN_TFT_BL, blJasnosc);
  else          digitalWrite(PIN_TFT_BL, blJasnosc > 0 ? HIGH : LOW);
  if (bl2PwmOk) ledcWrite(P2_BL, 255 - blJasnosc);
  else          digitalWrite(P2_BL, blJasnosc > 0 ? LOW : HIGH);
}

inline void startPodswietlenia(int jasnosc = 200) {
  blPwmOk  = ledcAttach(PIN_TFT_BL, BL_FREQ, BL_ROZDZ);
  bl2PwmOk = ledcAttach(P2_BL, BL_FREQ, BL_ROZDZ);
  if (!blPwmOk)  pinMode(PIN_TFT_BL, OUTPUT);
  if (!bl2PwmOk) pinMode(P2_BL, OUTPUT);
  Serial.printf("Podswietlenie PWM: ekran1 %s, ekran2 %s\n", blPwmOk ? "OK" : "wl/wyl", bl2PwmOk ? "OK" : "wl/wyl");
  ustawJasnosc(jasnosc);
}
