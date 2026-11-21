// Enkoder obrotowy z przyciskiem (np. moduł KY-040 / okrągły EC11)
// Obrót liczony w przerwaniach (nie gubi kroków podczas łączenia ze stacją),
// przycisk z eliminacją drgań styków: krótkie, podwójne i długie naciśnięcie.
#pragma once
#include <Arduino.h>

#define ENC_A      15   // CLK / S1
#define ENC_B      16   // DT  / S2
#define ENC_SW     17   // SW  / KEY

#define ENC_KROKI      4    // impulsów na jedno "kliknięcie" pokrętła (jeśli skacze o 2 → ustaw 2)
#define ENC_KIERUNEK   1    // jeśli kręci odwrotnie → ustaw -1
#define ENC_DLUGIE_MS  800  // czas długiego naciśnięcia
#define ENC_PODWOJNE_MS 350 // maks. odstęp między kliknięciami podwójnego kliknięcia

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

enum ZdarzeniePrzycisku { PRZ_BRAK, PRZ_KROTKO, PRZ_PODWOJNE, PRZ_DLUGO };

// Wywoływać często w loop().
// Krótkie kliknięcie jest zgłaszane po ENC_PODWOJNE_MS (czekamy, czy nie będzie drugiego).
inline ZdarzeniePrzycisku enkoderPrzycisk() {
  static bool stabilny = HIGH, ostatniOdczyt = HIGH;
  static uint32_t tZmiany = 0, tWcisniecia = 0, tKliku = 0;
  static bool dlugieZgloszone = false;
  static bool czekaNaDrugi = false;

  bool r = digitalRead(ENC_SW);
  uint32_t t = millis();
  if (r != ostatniOdczyt) { ostatniOdczyt = r; tZmiany = t; }

  if (t - tZmiany > 30 && r != stabilny) {          // 30 ms debounce
    stabilny = r;
    if (stabilny == LOW) {
      tWcisniecia = t;
      dlugieZgloszone = false;
    } else if (!dlugieZgloszone) {                   // puszczony przed czasem = klik
      if (czekaNaDrugi && t - tKliku < ENC_PODWOJNE_MS) {
        czekaNaDrugi = false;
        return PRZ_PODWOJNE;
      }
      czekaNaDrugi = true;
      tKliku = t;
    }
  }

  if (stabilny == LOW && !dlugieZgloszone && t - tWcisniecia > ENC_DLUGIE_MS) {
    dlugieZgloszone = true;
    czekaNaDrugi = false;
    return PRZ_DLUGO;                                // zgłoszone jeszcze w trakcie trzymania
  }

  if (czekaNaDrugi && stabilny == HIGH && t - tKliku >= ENC_PODWOJNE_MS) {
    czekaNaDrugi = false;
    return PRZ_KROTKO;
  }
  return PRZ_BRAK;
}
