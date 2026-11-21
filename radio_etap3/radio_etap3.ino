// RADIO SPARROW SYSTEM - etap 3
// ESP32-S3-N8R2 + PCM5102 (I2S) + ILI9341 2.8" z dotykiem + podłużny 2.25" ST7789 + 2 enkodery
//
// Ekran główny - 8 zakładek: Radio, Lista, Ulubione, Skala, Korektor, VU, Zegar, Ustawienia
// Ekran 2 (284x76) - analizator widma w stylu Technics
// Korektor 10-pasmowy (31 Hz - 16 kHz) działający na dźwięku, zapamiętywany po restarcie.
//
// Biblioteki: LovyanGFX, ESP32-audioI2S (schreibfaul1, najnowsza z GitHuba)
// Płytka: ESP32S3 Dev Module, PSRAM: QSPI PSRAM, Flash: 8MB, Partition: 8M with spiffs
//
// Twoje pliki (nie trafiają na GitHuba): sekrety.h, stacje.h, kalibracja.h

#include <WiFi.h>
#include <time.h>
#include "Audio.h"
#include "lgfx_config.h"
#include "podswietlenie.h"
#include "enkoder.h"
#include "stan.h"
#include "ekran.h"
#include "zakladki.h"
#include "skala.h"
#include "korektor.h"
#include "analizator.h"

#if __has_include("sekrety.h")
  #include "sekrety.h"
#else
  #error "Brak pliku sekrety.h - skopiuj sekrety_przyklad.h jako sekrety.h i wpisz dane Wi-Fi"
#endif

#if __has_include("kalibracja.h")
  #include "kalibracja.h"
#else
  #include "kalibracja_przyklad.h"
#endif

#define WERSJA "v0.7"
const char* STREFA_CZASU = "CET-1CEST,M3.5.0,M10.5.0/3";   // czas polski

const uint32_t OPOZNIENIE_WYBORU       = 700;    // ms bez kręcenia -> włącz wybraną stację
const uint32_t OPOZNIENIE_WYBORU_LISTA = 3000;   // w zakładkach Lista i Skala (czas na przeczytanie)
const uint32_t POWROT_TRYBU            = 5000;   // ms bez kręcenia -> powrót do wyboru stacji
const uint32_t CZAS_OKIENKA            = 1500;   // jak długo widać okienko głośności z enkodera 2

// I2S -> PCM5102
#define I2S_BCLK 5
#define I2S_LRC  6
#define I2S_DOUT 7

Audio      audio;
LGFX       tft;
LGFX_Pasek ekran2;

// ---------- Odtwarzacz w osobnym zadaniu (rdzeń 0) ----------
// connecttohost() potrafi blokować kilka sekund (DNS, przekierowania) -
// w osobnym zadaniu nie zamraża ekranu, dotyku ani enkoderów.
enum { AUDIO_POLACZ = 1, AUDIO_GLOSNOSC, AUDIO_STOP };
struct PolecenieAudio { uint8_t typ; int16_t arg; };
QueueHandle_t     kolejkaAudio;
SemaphoreHandle_t mutexTytul;
char              tytulBuf[256] = "";
volatile bool     nowyTytul = false;
uint32_t          tPolaczenia = 0;

void wyslijAudio(uint8_t typ, int arg) {
  PolecenieAudio p = {typ, (int16_t)arg};
  xQueueSend(kolejkaAudio, &p, 0);
}

void zadanieAudio(void*) {
  PolecenieAudio p;
  for (;;) {
    int polacz = -1;
    while (xQueueReceive(kolejkaAudio, &p, 0) == pdTRUE) {
      switch (p.typ) {
        case AUDIO_POLACZ:   polacz = p.arg; break;        // liczy się tylko ostatnie
        case AUDIO_GLOSNOSC: audio.setVolume(p.arg); break;
        case AUDIO_STOP:     polacz = -1; audio.stopSong(); break;
      }
    }
    if (polacz >= 0) audio.connecttohost(urlStacji(polacz));
    audio.loop();
    vTaskDelay(1);
  }
}

// Flagi odświeżania ekranu
bool odswiezStacje = true, odswiezTytul = true, odswiezGlos = true;
bool odswiezStan = true, odswiezPrzyciski = false, odswiezTresc = false, odswiezListe = false;
bool nakladkaBrudna = false;    // okienko/podgląd enkodera do przerysowania (max co 40 ms)
bool odswiezEQ = false;         // korektor do przerysowania

// ---------- Callback biblioteki audio (wywoływany z zadania audio) ----------
void my_audio_info(Audio::msg_t m) {
  switch (m.e) {
    case Audio::evt_streamtitle:
      xSemaphoreTake(mutexTytul, portMAX_DELAY);
      strlcpy(tytulBuf, m.msg ? m.msg : "", sizeof(tytulBuf));
      xSemaphoreGive(mutexTytul);
      nowyTytul = true;
      break;
    case Audio::evt_spectrum:
      ustawWidmo(m.vec1, m.vec2);
      break;
    case Audio::evt_vu:
      if (m.vec1.size() >= 4) {
        vuL = m.vec1[0]; vuP = m.vec1[1]; vuSzczytL = m.vec1[2]; vuSzczytP = m.vec1[3];
        vuNowe = true;
      }
      break;
    case Audio::evt_bitrate:
      odswiezStan = true;
      break;
    default:
      Serial.printf("%s: %s\n", m.s ? m.s : "audio", m.msg ? m.msg : "");
      break;
  }
}

// ---------- Pomocnicze ----------
int usypianieZostalo() {
  if (!usypianieMin) return 0;
  int32_t ms = (int32_t)(usypianieMin * 60000UL) - (int32_t)(millis() - usypianieStart);
  return ms > 0 ? (ms + 59999) / 60000 : 0;
}

int buforProcent() {
  uint32_t r = audio.getInBufferSize();
  return r ? audio.inBufferFilled() * 100 / r : 0;
}

int pokazywanaStacja() { return wybieranaStacja >= 0 ? wybieranaStacja : stacja; }
bool gra() { return !pauza && audio.isRunning(); }

void rysujStan() {
  rysujPasekStanu(pokazywanaStacja(), liczbaStacji, audio.getCodecname(), audio.getBitRate(),
                  pauza, usypianieZostalo(), wyciszony);
}

// Pełne narysowanie bieżącej zakładki
void rysujZakladke() {
  switch (zakladka) {
    case Z_RADIO:
      wyczyscTresc();
      rysujNazweStacji(nazwaStacji(stacja));
      ustawTytul(tytul, true);
      rysujWidmo(true);
      rysujPasekGlosnosci(trybEnk == TRYB_JASNOSC ? blJasnosc : glosnosc, trybEnk == TRYB_JASNOSC ? 255 : 21,
                          trybEnk == TRYB_JASNOSC, trybEnk != TRYB_STACJE);
      for (int i = 0; i < 5; i++) rysujPrzycisk(i, pauza);
      break;
    case Z_LISTA:
      listaPokazStacje(pokazywanaStacja());
      rysujListe(stacja, wybieranaStacja);
      break;
    case Z_ULUBIONE:
      rysujUlubione(stacja);
      break;
    case Z_SKALA:
      rysujSkale(pokazywanaStacja(), stacja, gra());
      break;
    case Z_KOREKTOR:
      rysujKorektor();
      break;
    case Z_VU:
      rysujVU(nazwaStacji(stacja), tytul);
      break;
    case Z_ZEGAR:
      rysujZegar(true, nazwaStacji(stacja), glosnosc);
      break;
    case Z_USTAWIENIA:
      rysujUstawienia(audio.getCodecname(), audio.getBitRate(), audio.getSampleRate(), buforProcent(),
                      WERSJA, blJasnosc, usypianieMin, usypianieZostalo());
      break;
  }
}

// Okienko / podgląd enkodera (zależnie od zakładki)
void pokazNakladke() {
  if (zakladka == Z_RADIO) {                      // zakładka Radio ma własne elementy
    if (wybieranaStacja >= 0) rysujNazweStacji(nazwaStacji(wybieranaStacja), TFT_WHITE);
    odswiezGlos = true;
    return;
  }
  if (trybEnk == TRYB_JASNOSC)       { toastPasek(blJasnosc, 255, true);  toastWidoczny = true; }
  else if (trybEnk == TRYB_GLOSNOSC || toastKoniec) { toastPasek(glosnosc, 21, false); toastWidoczny = true; }
  else if (wybieranaStacja >= 0) {
    if (zakladka == Z_LISTA)      { listaPokazStacje(wybieranaStacja); odswiezListeOszczednie(stacja, wybieranaStacja); }
    else if (zakladka == Z_SKALA) { /* wskazówka skali pokazuje wybór sama */ }
    else { toastStacja(wybieranaStacja, liczbaStacji, nazwaStacji(wybieranaStacja)); toastWidoczny = true; }
  }
}

void schowajNakladke() {
  toastKoniec = 0;
  if (toastWidoczny) { toastWidoczny = false; rysujZakladke(); }
}

void zmienZakladke(int z) {
  zakladka = (z % LICZBA_ZAKLADEK + LICZBA_ZAKLADEK) % LICZBA_ZAKLADEK;
  toastWidoczny = false;
  toastKoniec = 0;
  eqEdycja = false;
  rysujPasekZakladek(zakladka);
  rysujZakladke();
  if (trybEnk != TRYB_STACJE || (wybieranaStacja >= 0 && zakladka != Z_SKALA)) pokazNakladke();
}

// ---------- Sterowanie ----------
void grajStacje(int n) {
  if (liczbaStacji == 0) return;
  stacja = (n % liczbaStacji + liczbaStacji) % liczbaStacji;
  pauza  = false;
  tytul  = "";
  odswiezStacje = odswiezTytul = odswiezStan = odswiezPrzyciski = odswiezTresc = true;
  Serial.printf("Stacja %d: %s\n", stacja + 1, urlStacji(stacja));
  tPolaczenia = millis();
  wyslijAudio(AUDIO_POLACZ, stacja);
}

void ustawGlosnosc(int v) {
  glosnosc  = constrain(v, 0, 21);
  wyciszony = false;
  wyslijAudio(AUDIO_GLOSNOSC, glosnosc);
  odswiezGlos = odswiezStan = true;
}

void przelaczWyciszenie() {
  wyciszony = !wyciszony;
  wyslijAudio(AUDIO_GLOSNOSC, wyciszony ? 0 : glosnosc);
  odswiezStan = true;
}

void przelaczPauze() {
  if (pauza) { grajStacje(stacja); return; }       // radio na żywo: wznowienie = ponowne połączenie
  wyslijAudio(AUDIO_STOP, 0);
  pauza = true;
  odswiezPrzyciski = odswiezStan = true;
}

void ustawUsypianie(int minuty) {
  usypianieMin   = minuty;
  usypianieStart = millis();
  odswiezStan = true;
  if (zakladka == Z_USTAWIENIA) rysujUsypianieUst(usypianieMin, usypianieZostalo());
}

void sprawdzUsypianie() {
  if (!usypianieMin || millis() - usypianieStart < usypianieMin * 60000UL) return;
  usypianieMin = 0;
  wyslijAudio(AUDIO_STOP, 0);
  pauza = true;
  jasnoscPrzedSnem = blJasnosc;
  ustawJasnosc(0);
  uspiony = true;
  odswiezStan = odswiezPrzyciski = true;
  if (zakladka == Z_USTAWIENIA) rysujUsypianieUst(0, 0);
  Serial.println("Wylacznik czasowy: dobranoc");
}

void obudz() {
  uspiony = false;
  ustawJasnosc(jasnoscPrzedSnem);
}

void ustawTryb(int t) {
  trybEnk  = (TrybEnkodera)t;
  tEnkoder = millis();
  odswiezGlos = true;
  if (trybEnk == TRYB_STACJE) schowajNakladke();
  else                        nakladkaBrudna = true;
}

// Podgląd stacji (zatwierdzany po chwili) - z enkodera albo dotyku skali
void wybierzStacje(int n) {
  if (liczbaStacji == 0) return;
  wybieranaStacja = (n % liczbaStacji + liczbaStacji) % liczbaStacji;
  tWyboru = millis();
  nakladkaBrudna = true;
  odswiezStan = true;
}

// Chwilowe okienko głośności (enkoder 2)
void pokazGlosnoscChwilowo() {
  if (zakladka == Z_RADIO) { odswiezGlos = true; return; }
  toastKoniec = millis() + CZAS_OKIENKA;
  nakladkaBrudna = true;
}

// ---------- Dotyk ----------
void klikniecie(int x, int y) {
  int z = ktoraZakladka(x, y);
  if (z >= 0) { zmienZakladke(z); return; }

  switch (zakladka) {
    case Z_RADIO:
      if (y >= Y_STACJA && y < Y_STACJA + H_STACJA) { zmienZakladke(Z_LISTA); return; }  // nazwa -> lista
      switch (ktoryPrzycisk(x, y)) {
        case 0: grajStacje(stacja - 1);      break;
        case 1: ustawGlosnosc(glosnosc - 1); break;
        case 2: przelaczPauze();             break;
        case 3: ustawGlosnosc(glosnosc + 1); break;
        case 4: grajStacje(stacja + 1);      break;
        default: break;
      }
      break;
    case Z_LISTA: {
      int n = listaDotyk(y);
      if (n >= 0) grajStacje(n);
      break;
    }
    case Z_ULUBIONE: {
      int n = ulubioneDotyk(x, y);
      if (n >= 0) grajStacje(n);
      break;
    }
    case Z_SKALA: {
      int r = skalaDotyk(x, y, pokazywanaStacja());
      GrupaSkali g = grupaStacji(pokazywanaStacja());
      if (r >= 0)       { wybieranaStacja = -1; grajStacje(r); }
      else if (r == -2) wybierzStacje(g.pierwsza - 1);            // ostatnia stacja poprzedniej grupy
      else if (r == -3) wybierzStacje(g.pierwsza + g.ile);        // pierwsza stacja następnej grupy
      break;
    }
    case Z_KOREKTOR: {
      int a, b;
      int r = korektorDotyk(x, y, a, b);
      if (r == 1) { eqPasmo = a; eqUstawPasmo(a, b); odswiezEQ = true; }
      if (r == 2) { eqUstawPreset(a); odswiezEQ = true; }
      break;
    }
    case Z_USTAWIENIA: {
      int w;
      int r = ustawieniaDotyk(x, y, w);
      if (r == 1) { ustawJasnosc(w); rysujJasnoscUst(blJasnosc); }
      if (r == 2) ustawUsypianie(w);
      break;
    }
    default: break;
  }
}

void obsluzDotyk() {
  static bool     wcisniety = false, przeciaganie = false, pominKlik = false;
  static int      x0, y0, yOst;
  static uint32_t tOstatni = 0, tCzytania = 0;

  if (millis() - tCzytania < 15) return;        // odczyt dotyku co 15 ms
  tCzytania = millis();

  int32_t x, y;
  if (tft.getTouch(&x, &y)) {
    tOstatni = millis();
    if (!wcisniety) {                        // początek dotyku
      wcisniety = true; przeciaganie = false; pominKlik = false;
      x0 = x; y0 = y; yOst = y;
      if (uspiony) { obudz(); pominKlik = true; }
      return;
    }
    if (pominKlik) return;
    // Przeciąganie listy
    if (zakladka == Z_LISTA && y0 >= Y_LISTY && y0 < Y_ZAKL - 4) {
      if (!przeciaganie && abs(y - y0) > 12) przeciaganie = true;
      if (przeciaganie) {
        int kroki = (yOst - y) / H_WIERSZA;
        if (kroki) { listaPrzewin(kroki); yOst -= kroki * H_WIERSZA; odswiezListe = true; }
      }
    }
    // Przesuwanie suwaka jasności
    if (zakladka == Z_USTAWIENIA) {
      int w;
      if (ustawieniaDotyk(x, y, w) == 1 && abs(w - blJasnosc) > 3) { ustawJasnosc(w); rysujJasnoscUst(blJasnosc); }
    }
    // Przesuwanie suwaków korektora
    if (zakladka == Z_KOREKTOR) {
      int a, b;
      if (korektorDotyk(x, y, a, b) == 1 && (a != eqPasmo || b != eqZysk[a])) {
        przeciaganie = true;
        eqPasmo = a; eqUstawPasmo(a, b); odswiezEQ = true;
      }
    }
    return;
  }
  // Puszczenie (z tolerancją na chwilowe zaniki odczytu)
  if (wcisniety && millis() - tOstatni > 60) {
    wcisniety = false;
    if (!przeciaganie && !pominKlik) klikniecie(x0, y0);
  }
}

// ---------- Enkoder 1: strojenie ----------
//   obrót                    - wybór stacji (w Korektorze: pasmo albo jego wartość)
//   krótkie naciśnięcie      - włącz wybraną od razu / tryb głośności (w Korektorze: wybór pasma <-> zmiana wartości)
//   długie naciśnięcie (1 s) - tryb jasności
//   podwójne kliknięcie      - następna zakładka
void obsluzEnkoder1() {
  int k = enk1.kroki();
  ZdarzeniePrzycisku p = enk1.przycisk();
  if (uspiony && (k || p != PRZ_BRAK)) { obudz(); return; }

  if (k) {
    tEnkoder = millis();
    if (trybEnk == TRYB_JASNOSC) {
      ustawJasnosc(constrain(blJasnosc + k * 15, 10, 255));
      nakladkaBrudna = true;
    } else if (trybEnk == TRYB_GLOSNOSC) {
      ustawGlosnosc(glosnosc + k);
      nakladkaBrudna = true;
    } else if (zakladka == Z_KOREKTOR) {
      if (eqEdycja) eqUstawPasmo(eqPasmo, eqZysk[eqPasmo] + k);
      else          eqPasmo = constrain(eqPasmo + k, 0, EQ_PASM - 1);
      odswiezEQ = true;
    } else {
      wybierzStacje(pokazywanaStacja() + k);
    }
  }

  // Zatwierdzenie wybranej stacji
  uint32_t opoznienie = (zakladka == Z_LISTA || zakladka == Z_SKALA) ? OPOZNIENIE_WYBORU_LISTA : OPOZNIENIE_WYBORU;
  if (wybieranaStacja >= 0 && millis() - tWyboru > opoznienie) {
    int s = wybieranaStacja;
    wybieranaStacja = -1;
    if (s != stacja) grajStacje(s);
    else { odswiezStacje = odswiezStan = odswiezTresc = true; }
    schowajNakladke();
    if (zakladka == Z_LISTA) odswiezListe = true;     // zdejmij białą ramkę wyboru
  }

  switch (p) {
    case PRZ_KROTKO:
      if (wybieranaStacja >= 0)                              tWyboru = 0;     // włącz od razu
      else if (zakladka == Z_KOREKTOR && trybEnk == TRYB_STACJE) { eqEdycja = !eqEdycja; odswiezEQ = true; }
      else if (trybEnk == TRYB_STACJE)                       ustawTryb(TRYB_GLOSNOSC);
      else                                                   ustawTryb(TRYB_STACJE);
      break;
    case PRZ_DLUGO:
      ustawTryb(trybEnk == TRYB_JASNOSC ? TRYB_STACJE : TRYB_JASNOSC);
      break;
    case PRZ_PODWOJNE:
      zmienZakladke(zakladka + 1);
      break;
    default: break;
  }

  if (trybEnk != TRYB_STACJE && millis() - tEnkoder > POWROT_TRYBU) ustawTryb(TRYB_STACJE);
}

// ---------- Enkoder 2: głośność ----------
//   obrót               - głośność (w Korektorze: wartość wybranego pasma)
//   krótkie naciśnięcie - wyciszenie / przywrócenie dźwięku
//   podwójne kliknięcie - poprzednia zakładka
//   długie naciśnięcie  - przejście do Korektora (i z powrotem do Radia)
void obsluzEnkoder2() {
  int k = enk2.kroki();
  ZdarzeniePrzycisku p = enk2.przycisk();
  if (uspiony && (k || p != PRZ_BRAK)) { obudz(); return; }

  if (k) {
    if (zakladka == Z_KOREKTOR) {
      eqUstawPasmo(eqPasmo, eqZysk[eqPasmo] + k);
      odswiezEQ = true;
    } else {
      ustawGlosnosc(glosnosc + k);
      pokazGlosnoscChwilowo();
    }
  }
  switch (p) {
    case PRZ_KROTKO:   przelaczWyciszenie(); break;
    case PRZ_PODWOJNE: zmienZakladke(zakladka - 1); break;
    case PRZ_DLUGO:    zmienZakladke(zakladka == Z_KOREKTOR ? Z_RADIO : Z_KOREKTOR); break;
    default: break;
  }
}

// ---------- Serial Monitor ----------
// + / - głośność, n / p stacja, [ / ] jasność, spacja pauza, t / T zakładka, m wyciszenie
void obsluzSerial() {
  if (!Serial.available()) return;
  char c = Serial.read();
  if (c == '[') { ustawJasnosc(blJasnosc - 25); Serial.printf("Jasnosc: %d\n", blJasnosc); }
  if (c == ']') { ustawJasnosc(blJasnosc + 25); Serial.printf("Jasnosc: %d\n", blJasnosc); }
  if (c == '+') ustawGlosnosc(glosnosc + 1);
  if (c == '-') ustawGlosnosc(glosnosc - 1);
  if (c == 'n') grajStacje(stacja + 1);
  if (c == 'p') grajStacje(stacja - 1);
  if (c == ' ') przelaczPauze();
  if (c == 't') zmienZakladke(zakladka + 1);
  if (c == 'T') zmienZakladke(zakladka - 1);
  if (c == 'm') przelaczWyciszenie();
}

// ---------- Odświeżanie ekranów ----------
void odswiezEkran() {
  static uint32_t tStan = 0, tWolne = 0, tNakl = 0;
  bool sekunda = millis() - tStan > 1000;

  if (nowyTytul) {
    xSemaphoreTake(mutexTytul, portMAX_DELAY);
    tytul = tytulBuf;
    xSemaphoreGive(mutexTytul);
    nowyTytul = false;
    odswiezTytul = true;
  }

  if ((odswiezStan && millis() - tStan > 150) || sekunda) { odswiezStan = false; tStan = millis(); rysujStan(); }

  if (toastKoniec && (int32_t)(millis() - toastKoniec) > 0 && trybEnk == TRYB_STACJE && wybieranaStacja < 0)
    schowajNakladke();
  if (nakladkaBrudna && millis() - tNakl > 40) { nakladkaBrudna = false; tNakl = millis(); pokazNakladke(); }

  if (!toastWidoczny) {
    switch (zakladka) {
      case Z_RADIO:
        if (odswiezStacje && wybieranaStacja < 0) rysujNazweStacji(nazwaStacji(stacja));
        if (odswiezTytul) ustawTytul(tytul, true);
        if (odswiezGlos) {
          if (trybEnk == TRYB_JASNOSC) rysujPasekGlosnosci(blJasnosc, 255, true, true);
          else                         rysujPasekGlosnosci(glosnosc, 21, false, trybEnk == TRYB_GLOSNOSC);
        }
        if (odswiezPrzyciski) rysujPrzycisk(2, pauza);
        animujTytul();
        rysujWidmo();
        break;
      case Z_LISTA:
        if (odswiezTresc) listaPokazStacje(stacja);
        if (odswiezTresc || odswiezListe) odswiezListeOszczednie(stacja, wybieranaStacja);
        break;
      case Z_ULUBIONE:
        if (odswiezTresc) rysujUlubione(stacja);
        break;
      case Z_SKALA:
        aktualizujSkale(pokazywanaStacja(), stacja, gra());
        break;
      case Z_KOREKTOR:
        if (odswiezEQ) { rysujNaglowekEQ(); rysujSuwakiEQ(); rysujPresetyEQ(); }
        break;
      case Z_VU:
        if (odswiezTresc || odswiezTytul) rysujStopkeVU(nazwaStacji(stacja), tytul);
        rysujIgly();
        break;
      case Z_ZEGAR:
        if (odswiezTresc || odswiezGlos) rysujStopkeZegara(nazwaStacji(stacja), glosnosc);
        if (millis() - tWolne > 200) { tWolne = millis(); rysujZegar(false, nazwaStacji(stacja), glosnosc); }
        break;
      case Z_USTAWIENIA:
        if (sekunda) {
          rysujUstawieniaInfo(audio.getCodecname(), audio.getBitRate(), audio.getSampleRate(), buforProcent(), WERSJA);
          if (usypianieMin) rysujUsypianieUst(usypianieMin, usypianieZostalo());
        }
        break;
    }
    odswiezStacje = odswiezTytul = odswiezGlos = odswiezPrzyciski = odswiezTresc = odswiezListe = odswiezEQ = false;
  }

  // Ekran 2: analizator Technics
  klatkaAnalizatora(!gra() || wyciszony || uspiony);
}

// ---------- Setup / loop ----------
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.printf("PSRAM: %u bajtow\n", ESP.getPsramSize());
  przygotujListe();
  Serial.printf("Stacji na liscie: %d\n", liczbaStacji);
  eqWczytaj();

  tft.init();
  tft.setRotation(1);       // poziomo 320x240
  startAnalizatora();       // ekran 2
  startPodswietlenia(200);
  enk1.start();
  enk2.start();

  bool brakKalibracji = true;
  for (int i = 0; i < 8; i++) if (KALIBRACJA[i]) brakKalibracji = false;
  if (brakKalibracji) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(middle_center);
    tft.setFont(&fonts::Font2);
    tft.drawString("Kalibracja: dotknij strzalek", tft.width() / 2, tft.height() / 2);
    tft.calibrateTouch(KALIBRACJA, TFT_WHITE, TFT_BLACK, 20);
    Serial.print("Kalibracja: {");
    for (int i = 0; i < 8; i++) { Serial.print(KALIBRACJA[i]); if (i < 7) Serial.print(", "); }
    Serial.println("}");
  }
  tft.setTouchCalibrate(KALIBRACJA);

  // Ekran startowy podczas łączenia z Wi-Fi (min. 2,5 s)
  uint32_t tStart = millis();
  ekranStartowy(WERSJA);
  napisStartowy("Laczenie z Wi-Fi...");
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) { klatkaStartowa(); delay(40); }
  Serial.printf("Wi-Fi OK, IP: %s\n", WiFi.localIP().toString().c_str());
  napisStartowy("Wi-Fi OK  " + WiFi.localIP().toString());
  while (millis() - tStart < 2500) { klatkaStartowa(); delay(40); }

  configTzTime(STREFA_CZASU, "pool.ntp.org", "time.google.com");

  startEkranu();
  tft.fillScreen(TFT_BLACK);
  zmienZakladke(Z_RADIO);

  Audio::audio_info_callback = my_audio_info;
  audio.settings.SPECTRUM   = true;    // analizator widma
  audio.settings.VU_LEVEL   = true;    // wskaźnik VU
  audio.settings.IIR_FILTER = false;   // wbudowany 3-pasmowy filtr wyłączony - mamy własny korektor
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(glosnosc);

  kolejkaAudio = xQueueCreate(16, sizeof(PolecenieAudio));
  mutexTytul   = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(zadanieAudio, "audio", 12288, nullptr, 2, nullptr, 0);

  grajStacje(0);
}

void loop() {
  obsluzDotyk();
  obsluzEnkoder1();
  obsluzEnkoder2();
  obsluzSerial();
  sprawdzUsypianie();
  eqZapiszJesliTrzeba();
  odswiezEkran();

  // Ponowne łączenie po zerwaniu Wi-Fi lub strumienia
  static uint32_t sprawdzWifi = 0;
  if (millis() - sprawdzWifi > 10000) {
    sprawdzWifi = millis();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Wi-Fi zerwane, lacze ponownie...");
      WiFi.reconnect();
    } else if (!pauza && !audio.isRunning() && millis() - tPolaczenia > 15000) {
      grajStacje(stacja);
    }
  }

  vTaskDelay(2);
}
