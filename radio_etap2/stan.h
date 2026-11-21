// Wspólny stan radia: lista stacji, zakładki, tryby, dane audio
#pragma once
#include <Arduino.h>

// ---------- Lista stacji ----------
struct Stacja { const char* nazwa; const char* url; };
#define GRUPA(nazwa) {nazwa, nullptr}      // nagłówek grupy na liście (nie jest stacją)

#if __has_include("stacje.h")
  #include "stacje.h"
#else
  #include "stacje_przyklad.h"
#endif

#ifndef ULUBIONE_ZDEFINIOWANE
const int ULUBIONE[] = {1, 2, 3, 4, 5, 6};  // numery stacji (jak na liście, od 1)
#endif

const int LICZBA_WPISOW     = sizeof(STACJE) / sizeof(STACJE[0]);
const int LICZBA_ULUBIONYCH = min((int)(sizeof(ULUBIONE) / sizeof(ULUBIONE[0])), 6);

int     liczbaStacji = 0;              // same stacje, bez nagłówków grup
int16_t wpisStacji[LICZBA_WPISOW];     // nr stacji   -> indeks w STACJE
int16_t nrWpisu[LICZBA_WPISOW];        // indeks wpisu -> nr stacji albo -1 (nagłówek)

inline void przygotujListe() {
  liczbaStacji = 0;
  for (int i = 0; i < LICZBA_WPISOW; i++) {
    if (STACJE[i].url) { nrWpisu[i] = liczbaStacji; wpisStacji[liczbaStacji++] = i; }
    else               { nrWpisu[i] = -1; }
  }
}
inline const char* nazwaStacji(int n) { return STACJE[wpisStacji[n]].nazwa; }
inline const char* urlStacji(int n)   { return STACJE[wpisStacji[n]].url; }

// ---------- Zakładki i tryby ----------
enum Zakladka { Z_RADIO, Z_LISTA, Z_ULUBIONE, Z_VU, Z_ZEGAR, Z_USTAWIENIA, LICZBA_ZAKLADEK };
enum TrybEnkodera { TRYB_STACJE, TRYB_GLOSNOSC, TRYB_JASNOSC };

int      zakladka        = Z_RADIO;
int      stacja          = 0;
int      glosnosc        = 10;        // 0..21
bool     pauza           = false;
String   tytul           = "";

TrybEnkodera trybEnk     = TRYB_STACJE;
uint32_t tEnkoder        = 0;
int      wybieranaStacja = -1;        // -1 = nie przewijamy
uint32_t tWyboru         = 0;
bool     toastWidoczny   = false;     // okienko na wierzchu zakładki

// Wyłącznik czasowy
int      usypianieMin    = 0;         // 0 = wyłączony
uint32_t usypianieStart  = 0;
bool     uspiony         = false;
uint8_t  jasnoscPrzedSnem = 200;

// Wskaźnik VU (0..255) z biblioteki audio
uint8_t  vuL = 0, vuP = 0, vuSzczytL = 0, vuSzczytP = 0;
bool     vuNowe = false;
