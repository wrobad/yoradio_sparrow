// Zakładki: LISTA STACJI, ULUBIONE, WSKAŹNIK VU, ZEGAR, USTAWIENIA
// (zakładka RADIO jest w ekran.h)
#pragma once
#include "ekran.h"
#include "stan.h"

// =====================================================================
//                          LISTA STACJI
// =====================================================================
const int Y_LISTY = 22, H_WIERSZA = 22, WIERSZY = 8;
const int SZER_LISTY = 312;
int listaGora = 0;          // indeks wpisu w pierwszym widocznym wierszu

// Przewiń tak, żeby wpis był widoczny (z nagłówkiem grupy, jeśli się da)
inline void listaPokazWpis(int wpis) {
  if (wpis < listaGora) listaGora = wpis;
  else if (wpis >= listaGora + WIERSZY) listaGora = wpis - WIERSZY + 1;
  if (listaGora > 0 && listaGora == wpis && nrWpisu[listaGora - 1] < 0) listaGora--;
  listaGora = constrain(listaGora, 0, max(0, LICZBA_WPISOW - WIERSZY));
}
inline void listaPokazStacje(int n) { if (n >= 0 && n < liczbaStacji) listaPokazWpis(wpisStacji[n]); }

inline void listaPrzewin(int oIle) {
  listaGora = constrain(listaGora + oIle, 0, max(0, LICZBA_WPISOW - WIERSZY));
}

inline void rysujWierszListy(int r, int grajacy, int wybierany) {
  int e = listaGora + r;
  int y = Y_LISTY + r * H_WIERSZA;
  if (e >= LICZBA_WPISOW) { tft.fillRect(0, y, SZER_LISTY, H_WIERSZA, TFT_BLACK); return; }

  int n = nrWpisu[e];
  if (n < 0) {                                  // nagłówek grupy
    tft.fillRect(0, y, SZER_LISTY, H_WIERSZA, TFT_BLACK);
    tft.fillRect(0, y + 2, SZER_LISTY, H_WIERSZA - 4, kol::naglowek);
    String s = bezOgonkow(STACJE[e].nazwa);
    s.toUpperCase();
    tft.setFont(&fonts::Font2);
    tft.setTextColor(kol::morski);
    tft.setTextDatum(middle_left);
    tft.drawString(s, 8, y + H_WIERSZA / 2);
    return;
  }

  bool graj = (n == grajacy);
  bool wyb  = (n == wybierany);
  uint16_t tlo = graj ? kol::pasek : (wyb ? kol::linia : TFT_BLACK);
  tft.fillRect(0, y, SZER_LISTY, H_WIERSZA, tlo);
  if (wyb) tft.drawRect(0, y, SZER_LISTY, H_WIERSZA, TFT_WHITE);

  char nr[6];
  snprintf(nr, sizeof(nr), "%d", n + 1);
  tft.setFont(&fonts::Font2);
  tft.setTextColor(graj ? kol::zolty : kol::szary);
  tft.setTextDatum(middle_left);
  tft.drawString(nr, 6, y + H_WIERSZA / 2);

  tft.setFont(&fonts::FreeSans9pt7b);
  tft.setTextColor(graj ? kol::zolty : (wyb ? TFT_WHITE : kol::tekst));
  tft.setClipRect(32, y, SZER_LISTY - 54, H_WIERSZA);
  tft.drawString(bezOgonkow(STACJE[e].nazwa), 32, y + H_WIERSZA / 2);
  tft.clearClipRect();

  if (graj) tft.fillTriangle(SZER_LISTY - 16, y + 5, SZER_LISTY - 16, y + 17, SZER_LISTY - 6, y + 11, kol::zolty);
}

inline void rysujPasekPrzewijania() {
  const int x = 315, y0 = Y_LISTY + 2, h = WIERSZY * H_WIERSZA - 4;
  tft.fillRect(x, y0, 3, h, kol::linia);
  int hs = max(10, h * WIERSZY / max(LICZBA_WPISOW, 1));
  int maxGora = max(1, LICZBA_WPISOW - WIERSZY);
  int ys = y0 + (h - hs) * min(listaGora, maxGora) / maxGora;
  tft.fillRect(x, ys, 3, hs, kol::jasnyNieb);
}

inline void rysujListe(int grajacy, int wybierany) {
  for (int r = 0; r < WIERSZY; r++) rysujWierszListy(r, grajacy, wybierany);
  tft.fillRect(0, Y_LISTY + WIERSZY * H_WIERSZA, E_W, Y_ZAKL - (Y_LISTY + WIERSZY * H_WIERSZA), TFT_BLACK);
  rysujPasekPrzewijania();
}

// Zwraca nr stacji pod palcem, -1 = nagłówek/puste
inline int listaDotyk(int y) {
  if (y < Y_LISTY || y >= Y_LISTY + WIERSZY * H_WIERSZA) return -1;
  int e = listaGora + (y - Y_LISTY) / H_WIERSZA;
  return (e < LICZBA_WPISOW) ? nrWpisu[e] : -1;
}

// =====================================================================
//                            ULUBIONE
// =====================================================================
const int UL_X[3] = {6, 110, 214}, UL_Y[2] = {26, 116};
const int UL_W = 98, UL_H = 84;

// Tekst w maks. dwóch liniach (dzieli przy spacji najbliżej środka)
inline void tekstDwaWiersze(const String& s, int cx, int cy, int maxW) {
  tft.setTextDatum(middle_center);
  if (tft.textWidth(s) <= maxW) { tft.drawString(s, cx, cy); return; }
  int najl = -1, polowa = s.length() / 2;
  for (int i = 0; i < (int)s.length(); i++)
    if (s[i] == ' ' && (najl < 0 || abs(i - polowa) < abs(najl - polowa))) najl = i;
  if (najl < 0) { tft.drawString(s, cx, cy); return; }
  tft.drawString(s.substring(0, najl), cx, cy - 9);
  tft.drawString(s.substring(najl + 1), cx, cy + 9);
}

inline void rysujUlubione(int grajacy) {
  wyczyscTresc();
  for (int i = 0; i < LICZBA_ULUBIONYCH; i++) {
    int n = ULUBIONE[i] - 1;
    int x = UL_X[i % 3], y = UL_Y[i / 3];
    bool ok  = (n >= 0 && n < liczbaStacji);
    bool akt = ok && n == grajacy;
    if (akt) {
      tft.fillRoundRect(x, y, UL_W, UL_H, 8, kol::pasek);
      tft.drawRoundRect(x, y, UL_W, UL_H, 8, kol::zolty);
      tft.drawRoundRect(x + 1, y + 1, UL_W - 2, UL_H - 2, 7, kol::zolty);
    } else {
      tft.drawRoundRect(x, y, UL_W, UL_H, 8, kol::niebieski);
    }
    char nr[4];
    snprintf(nr, sizeof(nr), "%d", i + 1);
    tft.setFont(&fonts::FreeSansBold12pt7b);
    tft.setTextColor(akt ? kol::zolty : kol::jasnyNieb);
    tft.setTextDatum(middle_center);
    tft.drawString(nr, x + UL_W / 2, y + 24);

    tft.setFont(&fonts::Font2);
    tft.setTextColor(akt ? kol::zolty : kol::tekst);
    tft.setClipRect(x + 3, y + 40, UL_W - 6, UL_H - 42);
    tekstDwaWiersze(ok ? bezOgonkow(nazwaStacji(n)) : String("-"), x + UL_W / 2, y + 60, UL_W - 8);
    tft.clearClipRect();
  }
}

// Zwraca nr stacji z dotkniętego kafelka, -1 = nic
inline int ulubioneDotyk(int x, int y) {
  for (int i = 0; i < LICZBA_ULUBIONYCH; i++) {
    int tx = UL_X[i % 3], ty = UL_Y[i / 3];
    if (x >= tx && x < tx + UL_W && y >= ty && y < ty + UL_H) {
      int n = ULUBIONE[i] - 1;
      return (n >= 0 && n < liczbaStacji) ? n : -1;
    }
  }
  return -1;
}

// =====================================================================
//                         WSKAŹNIK VU
// =====================================================================
// Tarcza 308x164 rysowana raz do sprite'a (w PSRAM); w każdej klatce
// kopiujemy tylko obszar igieł, dorysowujemy igły i wysyłamy na ekran.
static LGFX_Sprite sprTarcza(&tft);
static LGFX_Sprite sprIgly(&tft);
const int VU_X = 6, VU_Y = 24, VU_W = 308, VU_H = 164;
const int VU_CX = 154, VU_CY = 172, VU_R = 120;     // oś igieł (współrzędne tarczy)
const int IG_X = 50, IG_Y = 40, IG_W = 208, IG_H = 124;
static bool  tarczaGotowa = false;
static float igL = -50, igP = -50;
static int   ostatniPeak = -1;

inline void punktVU(float a, float r, float& x, float& y) {
  float t = a * DEG_TO_RAD;
  x = VU_CX + r * sinf(t);
  y = VU_CY - r * cosf(t);
}

inline void przygotujTarcze() {
  sprTarcza.setPsram(true);
  sprTarcza.setColorDepth(16);
  sprTarcza.createSprite(VU_W, VU_H);
  sprIgly.setPsram(true);
  sprIgly.setColorDepth(16);
  sprIgly.createSprite(IG_W, IG_H);

  auto& s = sprTarcza;
  s.fillScreen(TFT_BLACK);
  s.fillRoundRect(0, 0, VU_W, VU_H, 10, kol::krem);
  s.drawRoundRect(0, 0, VU_W, VU_H, 10, kol::bursztyn);

  // Łuk skali i czerwona strefa (0 .. +3 dB)
  s.fillArc(VU_CX, VU_CY, VU_R, VU_R - 1, 270 - 50, 270 + 50, kol::linia);
  s.fillArc(VU_CX, VU_CY, VU_R + 7, VU_R + 2, 270 + 22, 270 + 50, kol::widmoCzer);

  // Drobne kreski co 5°
  for (int a = -50; a <= 50; a += 5) {
    float x1, y1, x2, y2;
    punktVU(a, VU_R, x1, y1); punktVU(a, VU_R + 5, x2, y2);
    s.drawLine(x1, y1, x2, y2, kol::szary);
  }

  // Główne kreski i opisy: {wartość dB, kąt}
  const int skala[][2] = {{-20, -50}, {-10, -31}, {-7, -20}, {-5, -10}, {-3, 2},
                          {-2, 9}, {-1, 15}, {0, 22}, {1, 31}, {2, 40}, {3, 50}};
  s.setFont(&fonts::Font2);
  s.setTextDatum(middle_center);
  for (auto& p : skala) {
    int v = p[0], a = p[1];
    uint16_t c = v > 0 ? kol::ciemnyCzer : kol::linia;
    float x1, y1, x2, y2, lx, ly;
    punktVU(a, VU_R, x1, y1); punktVU(a, VU_R + 11, x2, y2); punktVU(a, VU_R + 21, lx, ly);
    s.drawWideLine(x1, y1, x2, y2, 0.8f, c);
    if (v == -2 || v == -1) continue;              // bez opisu, tylko kreska
    char buf[5];
    snprintf(buf, sizeof(buf), v > 0 ? "+%d" : "%d", v);
    s.setTextColor(c);
    s.drawString(buf, lx, ly);
  }

  s.setFont(&fonts::FreeSansBold12pt7b);
  s.setTextColor(kol::szary);
  s.drawString("VU", VU_CX, 126);

  // Legenda igieł
  s.fillRect(112, 141, 14, 2, kol::linia);
  s.fillRect(170, 141, 14, 2, kol::ciemnyCzer);
  s.setFont(&fonts::Font2);
  s.setTextDatum(middle_left);
  s.setTextColor(kol::linia);      s.drawString("L", 130, 142);
  s.setTextColor(kol::ciemnyCzer); s.drawString("P", 188, 142);

  s.setTextColor(kol::szary);
  s.drawString("PEAK", 42, 16);
  s.setTextDatum(middle_right);
  s.drawString("dB", VU_W - 14, 16);

  s.fillRect(122, 154, 64, 10, kol::linia);       // osłona osi igieł
  tarczaGotowa = true;
}

inline float vuNaKat(uint8_t v) { return -50.0f + v * 100.0f / 255.0f; }

inline void rysujIgly(bool wymus = false) {
  static uint32_t t0 = 0;
  if (!wymus && (!vuNowe || millis() - t0 < 40)) return;   // max 25 klatek/s
  t0 = millis();
  vuNowe = false;

  igL += (vuNaKat(vuL) - igL) * 0.5f;
  igP += (vuNaKat(vuP) - igP) * 0.5f;

  sprTarcza.pushSprite(&sprIgly, -IG_X, -IG_Y);     // tło z tarczy
  float px = VU_CX - IG_X, py = VU_CY - IG_Y;
  auto igla = [&](float a, uint16_t c) {
    float t = a * DEG_TO_RAD;
    sprIgly.drawWideLine(px, py, px + (VU_R + 6) * sinf(t), py - (VU_R + 6) * cosf(t), 1.0f, c);
  };
  igla(igL, kol::linia);
  igla(igP, kol::ciemnyCzer);
  sprIgly.fillRect(122 - IG_X, 154 - IG_Y, 64, 10, kol::linia);
  sprIgly.pushSprite(VU_X + IG_X, VU_Y + IG_Y);

  // Lampki przesterowania (L, P)
  int peak = (vuSzczytL > 235 ? 1 : 0) | (vuSzczytP > 235 ? 2 : 0);
  if (peak != ostatniPeak || wymus) {
    ostatniPeak = peak;
    tft.fillCircle(VU_X + 16, VU_Y + 16, 4, (peak & 1) ? kol::widmoCzer : kol::bordo);
    tft.fillCircle(VU_X + 28, VU_Y + 16, 4, (peak & 2) ? kol::widmoCzer : kol::bordo);
  }
}

inline void rysujStopkeVU(const char* nazwa, const String& tyt) {
  tft.fillRect(0, 190, E_W, 15, TFT_BLACK);
  tft.setFont(&fonts::Font2);
  tft.setTextDatum(middle_left);
  tft.setTextColor(kol::zolty);
  String n = bezOgonkow(nazwa);
  tft.drawString(n, 8, 197);
  int x0 = 8 + tft.textWidth(n) + 12;
  tft.setTextColor(kol::szary);
  tft.setTextDatum(middle_right);
  tft.setClipRect(x0, 190, E_W - 8 - x0, 15);
  tft.drawString(bezOgonkow(tyt), E_W - 8, 197);
  tft.clearClipRect();
}

inline void rysujVU(const char* nazwa, const String& tyt) {
  if (!tarczaGotowa) przygotujTarcze();
  wyczyscTresc();
  sprTarcza.pushSprite(VU_X, VU_Y);
  ostatniPeak = -1;
  rysujIgly(true);
  rysujStopkeVU(nazwa, tyt);
}

// =====================================================================
//                              ZEGAR
// =====================================================================
static const char* DNI[]   = {"niedziela", "poniedzialek", "wtorek", "sroda", "czwartek", "piatek", "sobota"};
static const char* MIESC[] = {"stycznia", "lutego", "marca", "kwietnia", "maja", "czerwca", "lipca",
                              "sierpnia", "wrzesnia", "pazdziernika", "listopada", "grudnia"};
static int zegMin = -1, zegSek = -1, zegDzien = -1;

inline void rysujStopkeZegara(const char* nazwa, int glos) {
  tft.fillRect(0, 170, E_W, 30, TFT_BLACK);
  tft.fillTriangle(56, 178, 56, 190, 66, 184, kol::zolty);
  tft.setFont(&fonts::FreeSansBold9pt7b);
  tft.setTextColor(kol::zolty);
  tft.setTextDatum(middle_left);
  tft.setClipRect(74, 170, 160, 30);
  tft.drawString(bezOgonkow(nazwa), 74, 184);
  tft.clearClipRect();
  char buf[10];
  snprintf(buf, sizeof(buf), "vol %d", glos);
  tft.setFont(&fonts::Font2);
  tft.setTextColor(kol::szary, TFT_BLACK);
  tft.setTextDatum(middle_right);
  tft.drawString(buf, 264, 184);
}

inline void rysujZegar(bool pelny, const char* nazwa, int glos) {
  if (pelny) {
    wyczyscTresc();
    zegMin = zegSek = zegDzien = -1;
    tft.drawFastHLine(40, 162, 240, kol::linia);
    rysujStopkeZegara(nazwa, glos);
  }
  struct tm t;
  if (!getLocalTime(&t, 0)) {
    if (pelny) {
      tft.setFont(&fonts::Font8);
      tft.setTextColor(kol::szary, TFT_BLACK);
      tft.setTextDatum(middle_center);
      tft.drawString("--:--", 140, 88);
    }
    return;
  }
  char buf[40];
  if (t.tm_min != zegMin) {
    zegMin = t.tm_min;
    snprintf(buf, sizeof(buf), "%02d:%02d", t.tm_hour, t.tm_min);
    tft.fillRect(8, 46, 258, 82, TFT_BLACK);
    tft.setFont(&fonts::Font8);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(middle_center);
    tft.drawString(buf, 140, 88);
  }
  if (t.tm_sec != zegSek) {
    zegSek = t.tm_sec;
    snprintf(buf, sizeof(buf), "%02d", t.tm_sec);
    tft.setFont(&fonts::Font4);
    tft.setTextColor(kol::szary, TFT_BLACK);
    tft.setTextDatum(bottom_left);
    tft.drawString(buf, 272, 120);
  }
  if (t.tm_mday != zegDzien) {
    zegDzien = t.tm_mday;
    snprintf(buf, sizeof(buf), "%s, %d %s %d", DNI[t.tm_wday], t.tm_mday, MIESC[t.tm_mon], t.tm_year + 1900);
    tft.fillRect(0, 132, E_W, 24, TFT_BLACK);
    tft.setFont(&fonts::FreeSans9pt7b);
    tft.setTextColor(kol::jasnyNieb);
    tft.setTextDatum(middle_center);
    tft.drawString(buf, E_W / 2, 144);
  }
}

// =====================================================================
//                     USTAWIENIA I INFORMACJE
// =====================================================================
const int US_SUWAK_X = 100, US_SUWAK_W = 180, US_SUWAK_Y = 140;
const int US_SEN_X[4] = {92, 148, 204, 260}, US_SEN_W[4] = {50, 50, 50, 52};
const int US_SEN_MIN[4] = {0, 15, 30, 60};
const int US_SEN_Y = 164, US_SEN_H = 26;

inline void wierszInfo(int y, const char* etykieta, const String& wartosc, uint16_t kolor) {
  tft.setFont(&fonts::Font2);
  tft.setTextColor(kol::szary, TFT_BLACK);
  tft.setTextDatum(middle_left);
  tft.drawString(etykieta, 8, y);
  tft.setTextColor(kolor, TFT_BLACK);
  tft.setTextDatum(middle_right);
  tft.drawString(wartosc, E_W - 8, y);
}

inline void rysujUstawieniaInfo(const char* kodek, uint32_t bitrate, uint32_t probkowanie,
                                int buforProc, const char* wersja) {
  tft.fillRect(0, 24, E_W, 94, TFT_BLACK);
  int rssi = WiFi.RSSI();
  const char* jakosc = rssi > -55 ? "bardzo dobry" : rssi > -65 ? "dobry" : rssi > -75 ? "sredni" : "slaby";
  uint16_t kSyg = rssi > -65 ? kol::wifiOn : rssi > -75 ? kol::widmoPom : kol::widmoCzer;
  char buf[40];

  wierszInfo(34, "Adres IP", WiFi.localIP().toString(), kol::tekst);
  snprintf(buf, sizeof(buf), "%d dBm  %s", rssi, jakosc);
  wierszInfo(52, "Sygnal Wi-Fi", buf, kSyg);
  if (bitrate > 0)
    snprintf(buf, sizeof(buf), "%s  %u kb/s  %u,%u kHz", kodek ? kodek : "", (unsigned)(bitrate / 1000),
             (unsigned)(probkowanie / 1000), (unsigned)(probkowanie % 1000 / 100));
  else
    snprintf(buf, sizeof(buf), "%s", kodek ? kodek : "-");
  wierszInfo(70, "Strumien", buf, kol::tekst);

  wierszInfo(88, "Bufor", "", kol::tekst);
  tft.drawRect(150, 83, 162, 10, kol::szary);
  tft.fillRect(152, 85, 158 * constrain(buforProc, 0, 100) / 100, 6, kol::niebieski);

  snprintf(buf, sizeof(buf), "Sparrow %s", wersja);
  wierszInfo(106, "Wersja", buf, kol::tekst);
}

inline void rysujJasnoscUst(uint8_t jasnosc) {
  tft.fillRect(0, 126, E_W, 30, TFT_BLACK);
  ikonaSlonce(8, 133, kol::zolty);
  tft.setFont(&fonts::Font2);
  tft.setTextColor(kol::tekst);
  tft.setTextDatum(middle_left);
  tft.drawString("Jasnosc", 28, US_SUWAK_Y);
  tft.fillRoundRect(US_SUWAK_X, US_SUWAK_Y - 3, US_SUWAK_W, 6, 3, kol::linia);
  int xk = US_SUWAK_X + US_SUWAK_W * jasnosc / 255;
  tft.fillRoundRect(US_SUWAK_X, US_SUWAK_Y - 3, xk - US_SUWAK_X, 6, 3, kol::zolty);
  tft.fillCircle(xk, US_SUWAK_Y, 7, kol::zolty);
  char buf[6];
  snprintf(buf, sizeof(buf), "%d%%", jasnosc * 100 / 255);
  tft.setTextDatum(middle_right);
  tft.drawString(buf, E_W - 4, US_SUWAK_Y);
}

inline void rysujUsypianieUst(int minuty, int zostalo) {
  tft.fillRect(0, US_SEN_Y - 2, E_W, US_SEN_H + 6, TFT_BLACK);
  tft.setFont(&fonts::Font2);
  tft.setTextColor(kol::tekst);
  tft.setTextDatum(middle_left);
  if (minuty > 0) {
    char buf[16];
    snprintf(buf, sizeof(buf), "Zz %d min", zostalo);
    tft.setTextColor(kol::zolty);
    tft.drawString(buf, 8, US_SEN_Y + US_SEN_H / 2);
  } else {
    tft.drawString("Wylacz za", 8, US_SEN_Y + US_SEN_H / 2);
  }
  const char* opis[4] = {"Wyl.", "15 min", "30 min", "60 min"};
  for (int i = 0; i < 4; i++) {
    bool akt = (US_SEN_MIN[i] == minuty);
    int x = US_SEN_X[i], w = US_SEN_W[i];
    if (akt) tft.fillRoundRect(x, US_SEN_Y, w, US_SEN_H, 5, kol::pasek);
    tft.drawRoundRect(x, US_SEN_Y, w, US_SEN_H, 5, akt ? kol::zolty : kol::niebieski);
    tft.setTextColor(akt ? kol::zolty : kol::jasnyNieb);
    tft.setTextDatum(middle_center);
    tft.drawString(opis[i], x + w / 2, US_SEN_Y + US_SEN_H / 2);
  }
}

inline void rysujUstawienia(const char* kodek, uint32_t bitrate, uint32_t probkowanie, int buforProc,
                            const char* wersja, uint8_t jasnosc, int usypMin, int usypZostalo) {
  wyczyscTresc();
  rysujUstawieniaInfo(kodek, bitrate, probkowanie, buforProc, wersja);
  tft.drawFastHLine(0, 121, E_W, kol::linia);
  rysujJasnoscUst(jasnosc);
  rysujUsypianieUst(usypMin, usypZostalo);
}

// Dotyk w ustawieniach: 1 = suwak jasności (wartosc 10..255), 2 = usypianie (wartosc = minuty), 0 = nic
inline int ustawieniaDotyk(int x, int y, int& wartosc) {
  if (y >= US_SUWAK_Y - 14 && y <= US_SUWAK_Y + 14 && x >= US_SUWAK_X - 10 && x <= US_SUWAK_X + US_SUWAK_W + 10) {
    wartosc = constrain((x - US_SUWAK_X) * 255 / US_SUWAK_W, 10, 255);
    return 1;
  }
  if (y >= US_SEN_Y - 3 && y <= US_SEN_Y + US_SEN_H + 3) {
    for (int i = 0; i < 4; i++)
      if (x >= US_SEN_X[i] && x < US_SEN_X[i] + US_SEN_W[i]) { wartosc = US_SEN_MIN[i]; return 2; }
  }
  return 0;
}
