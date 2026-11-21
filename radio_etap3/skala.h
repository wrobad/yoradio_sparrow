// Zakładka SKALA: skala radiowa grupy stacji z czerwoną wskazówką.
// Tło skali (z napisami) rysowane raz do sprite'a w PSRAM; wskazówka przesuwa się płynnie,
// a przy ruchu odświeżany jest tylko wąski pas tła pod starą wskazówką.
#pragma once
#include "ekran.h"
#include "stan.h"

const int SK_X = 4, SK_Y = 24, SK_W = 312, SK_H = 178;   // pole skali na ekranie
const int SK_LINIA = 110;                                 // linia skali (współrzędne ekranu)
const int SK_L = 16, SK_P = 304;                          // lewy/prawy koniec podziałki
const int SK_IGLA_G = 50, SK_IGLA_D = 170;

namespace kolSk {
  const uint16_t tlo      = 0x0840;   // #0b0906
  const uint16_t ramka    = 0x61C0;   // #633806
  const uint16_t drobne   = 0x8261;   // #854F0B
  const uint16_t igla     = 0xE249;
  const uint16_t ledOff   = 0x2965;
}

struct GrupaSkali { int naglowek; int pierwsza; int ile; };   // naglowek = indeks w STACJE (-1 brak)

inline GrupaSkali grupaStacji(int n) {
  GrupaSkali g = {-1, 0, liczbaStacji};
  if (n < 0 || n >= liczbaStacji) return g;
  int e = wpisStacji[n];
  int h = e;
  while (h >= 0 && nrWpisu[h] >= 0) h--;
  int k = e;
  while (k + 1 < LICZBA_WPISOW && nrWpisu[k + 1] >= 0) k++;
  g.naglowek = h;
  g.pierwsza = nrWpisu[h + 1];
  g.ile      = nrWpisu[k] - g.pierwsza + 1;
  return g;
}

inline const char* nazwaGrupy(const GrupaSkali& g) { return g.naglowek >= 0 ? STACJE[g.naglowek].nazwa : "Stacje"; }

static LGFX_Sprite sprSkala(&tft);
static bool        skalaGotowa = false;
static int         skGrupaPierwsza = -1, skGrajaca = -2;   // co narysowano w tle
static float       skIglaX = -1, skIglaCel = -1;
static bool        skLed = false;

inline float skalaX(const GrupaSkali& g, int n) { return SK_L + (n - g.pierwsza + 0.5f) * (SK_P - SK_L) / g.ile; }

// Rysuje tło skali (grupa, podziałka, nazwy, sąsiednie grupy) do sprite'a i na ekran
inline void rysujTloSkali(const GrupaSkali& g, int grajaca) {
  if (!skalaGotowa) {
    sprSkala.setPsram(true);
    sprSkala.setColorDepth(16);
    sprSkala.createSprite(SK_W, SK_H);
    skalaGotowa = true;
  }
  auto& s = sprSkala;
  const int oy = SK_Y, ox = SK_X;              // przesunięcie: ekran -> sprite
  s.fillScreen(TFT_BLACK);
  s.fillRoundRect(0, 0, SK_W, SK_H, 8, kolSk::tlo);
  s.drawRoundRect(0, 0, SK_W, SK_H, 8, kolSk::ramka);

  String gn = bezOgonkow(nazwaGrupy(g));
  gn.toUpperCase();
  s.setFont(&fonts::Font2);
  s.setTextDatum(middle_left);
  s.setTextColor(kol::morski);
  s.drawString(gn, 10, 12);

  // Podziałka
  s.drawFastHLine(SK_L - ox, SK_LINIA - oy, SK_P - SK_L, kol::bursztyn);
  for (int x = SK_L; x <= SK_P; x += 6) s.drawFastVLine(x - ox, SK_LINIA - oy - 3, 3, kolSk::drobne);

  // Stacje grupy: na przemian nad i pod linią, w 4 rzędach
  const int rzad[4] = {72, 136, 90, 154};
  float krok = (float)(SK_P - SK_L) / g.ile;
  int szer = min(150, (int)(krok * 4) - 6);
  for (int k = 0; k < g.ile; k++) {
    int n = g.pierwsza + k;
    int x = (int)skalaX(g, n) - ox;
    s.fillRect(x, SK_LINIA - oy - 10, 2, 20, kol::widmoPom);
    int y = rzad[k % 4] - oy;
    bool graj = (n == grajaca);
    s.setTextColor(graj ? kol::zolty : kol::krem);
    s.setTextDatum(middle_center);
    s.setClipRect(max(2, x - szer / 2), y - 9, szer, 18);
    s.drawString(bezOgonkow(nazwaStacji(n)), x, y);
    s.clearClipRect();
  }

  // Sąsiednie grupy
  s.drawFastHLine(10, 176 - oy, SK_W - 20, kol::linia);
  s.setTextColor(kol::szary);
  if (g.pierwsza > 0) {
    GrupaSkali p = grupaStacji(g.pierwsza - 1);
    s.setTextDatum(middle_left);
    s.setClipRect(0, 180 - oy, SK_W / 2 - 4, 20);
    s.drawString(String("< ") + bezOgonkow(nazwaGrupy(p)), 10, 189 - oy);
    s.clearClipRect();
  }
  if (g.pierwsza + g.ile < liczbaStacji) {
    GrupaSkali nx = grupaStacji(g.pierwsza + g.ile);
    s.setTextDatum(middle_right);
    s.setClipRect(SK_W / 2 + 4, 180 - oy, SK_W / 2 - 4, 20);
    s.drawString(bezOgonkow(nazwaGrupy(nx)) + " >", SK_W - 10, 189 - oy);
    s.clearClipRect();
  }

  s.pushSprite(SK_X, SK_Y);
  skGrupaPierwsza = g.pierwsza;
  skGrajaca = grajaca;
  skLed = !skLed;   // wymuś narysowanie lampki
}

inline void rysujIgleSkali(int x) {
  tft.fillRect(x - 1, SK_IGLA_G, 2, SK_IGLA_D - SK_IGLA_G, kolSk::igla);
  tft.fillTriangle(x - 5, SK_IGLA_G - 2, x + 5, SK_IGLA_G - 2, x, SK_IGLA_G + 4, kolSk::igla);
}

inline void zmazIgleSkali(int x) {              // przywróć tło pod wskazówką
  tft.setClipRect(x - 6, SK_IGLA_G - 3, 13, SK_IGLA_D - SK_IGLA_G + 4);
  sprSkala.pushSprite(SK_X, SK_Y);
  tft.clearClipRect();
}

inline void rysujLedSkali(bool dostrojona) {
  if (dostrojona == skLed) return;
  skLed = dostrojona;
  tft.fillCircle(SK_X + SK_W - 14, SK_Y + 12, 4, dostrojona ? kol::wifiOn : kolSk::ledOff);
}

// Pełne narysowanie zakładki
inline void rysujSkale(int pokazywana, int grajaca, bool gra) {
  if (liczbaStacji == 0) { wyczyscTresc(); return; }
  GrupaSkali g = grupaStacji(pokazywana);
  wyczyscTresc();
  rysujTloSkali(g, grajaca);
  skIglaCel = skIglaX = skalaX(g, pokazywana);
  rysujIgleSkali((int)skIglaX);
  rysujLedSkali(pokazywana == grajaca && gra);
}

// Wołać w pętli: przerysowuje tło przy zmianie grupy/grającej stacji, płynnie przesuwa wskazówkę
inline void aktualizujSkale(int pokazywana, int grajaca, bool gra) {
  if (liczbaStacji == 0) return;
  GrupaSkali g = grupaStacji(pokazywana);
  if (g.pierwsza != skGrupaPierwsza || grajaca != skGrajaca) {
    rysujTloSkali(g, grajaca);
    skIglaCel = skIglaX = skalaX(g, pokazywana);
    rysujIgleSkali((int)skIglaX);
  }
  skIglaCel = skalaX(g, pokazywana);
  static uint32_t t0 = 0;
  if (fabsf(skIglaCel - skIglaX) >= 0.5f && millis() - t0 >= 25) {
    t0 = millis();
    float krok = (skIglaCel - skIglaX) * 0.3f;
    if (fabsf(krok) < 1.0f) krok = (krok > 0) ? 1.0f : -1.0f;
    if (fabsf(skIglaCel - skIglaX) < 1.0f) krok = skIglaCel - skIglaX;
    zmazIgleSkali((int)skIglaX);
    skIglaX += krok;
    rysujIgleSkali((int)skIglaX);
  }
  rysujLedSkali(pokazywana == grajaca && gra);
}

// Dotyk: zwraca nr stacji (najbliższa wskazówki), albo -2 = poprzednia grupa, -3 = następna, -1 = nic
inline int skalaDotyk(int x, int y, int pokazywana) {
  if (liczbaStacji == 0) return -1;
  GrupaSkali g = grupaStacji(pokazywana);
  if (y >= 178 && y < Y_ZAKL - 2) {
    if (x < E_W / 2 && g.pierwsza > 0) return -2;
    if (x >= E_W / 2 && g.pierwsza + g.ile < liczbaStacji) return -3;
    return -1;
  }
  if (y < 46 || y > 172) return -1;
  int k = constrain((int)((x - SK_L) * g.ile / (SK_P - SK_L)), 0, g.ile - 1);
  return g.pierwsza + k;
}
