// Enkodery obrotowe z przyciskiem (np. KY-040 / okrągły EC11)
// Obrót liczony w przerwaniach, przycisk z eliminacją drgań:
// krótkie, podwójne i długie naciśnięcie.
// Niepodłączony enkoder nic nie robi (wejścia podciągnięte do 3,3 V).
#pragma once
#include <Arduino.h>

// Enkoder 1 - strojenie (prawy)
#define ENC1_A   15   // CLK / S1
#define ENC1_B   16   // DT  / S2
#define ENC1_SW  17   // SW  / KEY
// Enkoder 2 - głośność (lewy)
#define ENC2_A   35
#define ENC2_B   36
#define ENC2_SW  37

#define ENC_KROKI       4     // impulsów na jedno "kliknięcie" (jeśli skacze o 2 → ustaw 2)
#define ENC1_KIERUNEK   1     // jeśli kręci odwrotnie → -1
#define ENC2_KIERUNEK   1
#define ENC_DLUGIE_MS   800   // długie naciśnięcie
#define ENC_PODWOJNE_MS 350   // maks. odstęp podwójnego kliknięcia

enum ZdarzeniePrzycisku { PRZ_BRAK, PRZ_KROTKO, PRZ_PODWOJNE, PRZ_DLUGO };

class Enkoder {
public:
  Enkoder(uint8_t a, uint8_t b, uint8_t sw, int8_t kierunek)
    : pinA(a), pinB(b), pinSW(sw), kier(kierunek) {}

  void start() {
    pinMode(pinA, INPUT_PULLUP);
    pinMode(pinB, INPUT_PULLUP);
    pinMode(pinSW, INPUT_PULLUP);
    stan = (digitalRead(pinA) << 1) | digitalRead(pinB);
    attachInterruptArg(digitalPinToInterrupt(pinA), isr, this, CHANGE);
    attachInterruptArg(digitalPinToInterrupt(pinB), isr, this, CHANGE);
  }

  // Liczba "kliknięć" od ostatniego wywołania (+ w prawo, - w lewo)
  int kroki() {
    int32_t l = licznik;
    int32_t k = (l - ostatni) / ENC_KROKI;
    ostatni += k * ENC_KROKI;
    return k * kier;
  }

  // Wywoływać często. Krótkie kliknięcie zgłaszane po ENC_PODWOJNE_MS.
  ZdarzeniePrzycisku przycisk() {
    bool r = digitalRead(pinSW);
    uint32_t t = millis();
    if (r != ostatniOdczyt) { ostatniOdczyt = r; tZmiany = t; }

    if (t - tZmiany > 30 && r != stabilny) {
      stabilny = r;
      if (stabilny == LOW) {
        tWcisniecia = t;
        dlugieZgloszone = false;
      } else if (!dlugieZgloszone) {
        if (czekaNaDrugi && t - tKliku < ENC_PODWOJNE_MS) { czekaNaDrugi = false; return PRZ_PODWOJNE; }
        czekaNaDrugi = true;
        tKliku = t;
      }
    }
    if (stabilny == LOW && !dlugieZgloszone && t - tWcisniecia > ENC_DLUGIE_MS) {
      dlugieZgloszone = true;
      czekaNaDrugi = false;
      return PRZ_DLUGO;
    }
    if (czekaNaDrugi && stabilny == HIGH && t - tKliku >= ENC_PODWOJNE_MS) {
      czekaNaDrugi = false;
      return PRZ_KROTKO;
    }
    return PRZ_BRAK;
  }

private:
  static void ARDUINO_ISR_ATTR isr(void* arg) {
    static const int8_t tab[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
    Enkoder* e = (Enkoder*)arg;
    uint8_t s = (digitalRead(e->pinA) << 1) | digitalRead(e->pinB);
    e->stan = ((e->stan << 2) | s) & 0x0F;
    e->licznik += tab[e->stan];
  }

  uint8_t pinA, pinB, pinSW;
  int8_t  kier;
  volatile int32_t licznik = 0;
  volatile uint8_t stan = 0;
  int32_t  ostatni = 0;
  bool     stabilny = HIGH, ostatniOdczyt = HIGH;
  uint32_t tZmiany = 0, tWcisniecia = 0, tKliku = 0;
  bool     dlugieZgloszone = false, czekaNaDrugi = false;
};

Enkoder enk1(ENC1_A, ENC1_B, ENC1_SW, ENC1_KIERUNEK);   // strojenie
Enkoder enk2(ENC2_A, ENC2_B, ENC2_SW, ENC2_KIERUNEK);   // głośność
