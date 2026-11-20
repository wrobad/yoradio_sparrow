// Enkoder obrotowy z przyciskiem (np. moduł KY-040)
// Obrót liczony w przerwaniach (nie gubi kroków podczas łączenia ze stacją),
// przycisk z eliminacją drgań styków, rozróżnia krótkie i długie naciśnięcie.
#pragma once
#include <Arduino.h>

#define ENC_A      15   // CLK
#define ENC_B      16   // DT
#define ENC_SW     17   // SW (przycisk)

#define ENC_KROKI    4    // impulsów na jedno "kliknięcie" pokrętła (jeśli skacze o 2 → ustaw 2)
#define ENC_KIERUNEK 1    // jeśli kręci odwrotnie → ustaw -1
#define ENC_DLUGIE_MS 800 // czas długiego naciśnięcia

static volatile int32_t encLicznik = 0;
static volatile uint8_t encStan    = 0;

static void ARDUINO_ISR_ATTR encISR() {
  // Tablica przejść dekodera kwadraturowego - ignoruje błędne stany (drgania)
  static const int8_t tab[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
  uint8_t s = (digitalRead(ENC_A) << 1) | digitalRead(ENC_B);
  encStan = ((encStan << 2) | s) & 0x0F;
  encLicznik += tab[encStan];
}

inline void startEnkodera() {
  pinMode(ENC_A,  INPUT_PULLUP);
  pinMode(ENC_B,  INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);
  encStan = (digitalRead(ENC_A) << 1) | digitalRead(ENC_B);
  attachInterrupt(digitalPinToInterrupt(ENC_A), encISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), encISR, CHANGE);
}

// Zwraca liczbę "kliknięć" od ostatniego wywołania (+ w prawo, - w lewo)
inline int enkoderKroki() {
  static int32_t ostatni = 0;
  int32_t l = encLicznik;                 // odczyt 32-bit jest atomowy
  int32_t kroki = (l - ostatni) / ENC_KROKI;
  ostatni += kroki * ENC_KROKI;
  return kroki * ENC_KIERUNEK;
}

enum ZdarzeniePrzycisku { PRZ_BRAK, PRZ_KROTKO, PRZ_DLUGO };

// Wywoływać często w loop()
inline ZdarzeniePrzycisku enkoderPrzycisk() {
  static bool stabilny = HIGH, ostatniOdczyt = HIGH;
  static uint32_t tZmiany = 0, tWcisniecia = 0;
  static bool dlugieZgloszone = false;

  bool r = digitalRead(ENC_SW);
  uint32_t t = millis();
  if (r != ostatniOdczyt) { ostatniOdczyt = r; tZmiany = t; }

  if (t - tZmiany > 30 && r != stabilny) {          // 30 ms debounce
    stabilny = r;
    if (stabilny == LOW) { tWcisniecia = t; dlugieZgloszone = false; }
    else if (!dlugieZgloszone) return PRZ_KROTKO;    // puszczony przed czasem
  }
  if (stabilny == LOW && !dlugieZgloszone && t - tWcisniecia > ENC_DLUGIE_MS) {
    dlugieZgloszone = true;
    return PRZ_DLUGO;                                // zgłoszone jeszcze w trakcie trzymania
  }
  return PRZ_BRAK;
}
