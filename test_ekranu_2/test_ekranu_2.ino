// Test podłużnego wyświetlacza 2.25" 76x284 (ST7789P3) na drugiej szynie SPI (SPI3)
// Biblioteka: LovyanGFX
//
// Podłączenie:
//   GND -> GND      VCC -> 3.3
//   SCL -> 41       SDA -> 42
//   RST -> 39       DC  -> 40
//   CS  -> 38       BL  -> 47  (świeci przy stanie NISKIM)
//
// Co powinno być widać: biała ramka dokładnie na krawędziach ekranu,
// w rogach kolorowe kwadraty (lewy górny CZERWONY, prawy górny ZIELONY,
// lewy dolny NIEBIESKI, prawy dolny ŻÓŁTY) i napis na środku.
//
// Korekta w Serial Monitorze (115200), wpisz znak i Enter:
//   x / X  - przesunięcie w poziomie -1 / +1
//   y / Y  - przesunięcie w pionie   -1 / +1
//   i      - odwrócenie kolorów (gdy tło jest białe zamiast czarnego)
//   r      - zamiana czerwony <-> niebieski
// Gdy obraz jest dobry, przepisz wypisane wartości - wpiszę je do programu radia.

#include <LovyanGFX.hpp>

#define P2_SCK  41
#define P2_MOSI 42
#define P2_RST  39
#define P2_DC   40
#define P2_CS   38
#define P2_BL   47
#define BL_WLACZ LOW    // ten moduł (tranzystor PNP "2TY") świeci przy stanie NISKIM

int  offX = 82, offY = 18;      // typowe wartości dla 76x284
bool odwroc = true, zamienRGB = false;

class LGFX_Pasek : public lgfx::LGFX_Device {
public:
  lgfx::Panel_ST7789 _panel;
  lgfx::Bus_SPI      _bus;

  LGFX_Pasek() {
    auto b = _bus.config();
    b.spi_host    = SPI3_HOST;
    b.spi_mode    = 0;
    b.freq_write  = 20000000;      // wolniej = bezpieczniej na przewodach
    b.freq_read   = 16000000;
    b.spi_3wire   = true;
    b.use_lock    = true;
    b.dma_channel = SPI_DMA_CH_AUTO;
    b.pin_sclk = P2_SCK;
    b.pin_mosi = P2_MOSI;
    b.pin_miso = -1;
    b.pin_dc   = P2_DC;
    _bus.config(b);
    _panel.setBus(&_bus);
    ustawPanel();
    setPanel(&_panel);
  }

  void ustawPanel() {
    auto p = _panel.config();
    p.pin_cs   = P2_CS;
    p.pin_rst  = P2_RST;
    p.pin_busy = -1;
    p.panel_width   = 76;
    p.panel_height  = 284;
    p.memory_width  = 240;
    p.memory_height = 320;
    p.offset_x = offX;
    p.offset_y = offY;
    p.readable   = false;
    p.invert     = odwroc;
    p.rgb_order  = zamienRGB;
    p.bus_shared = false;
    _panel.config(p);
  }
};

LGFX_Pasek ekran2;

void rysujTest() {
  ekran2.fillScreen(TFT_BLACK);
  int w = ekran2.width(), h = ekran2.height();
  ekran2.drawRect(0, 0, w, h, TFT_WHITE);
  ekran2.drawRect(1, 1, w - 2, h - 2, TFT_WHITE);
  ekran2.fillRect(3, 3, 12, 12, TFT_RED);
  ekran2.fillRect(w - 15, 3, 12, 12, TFT_GREEN);
  ekran2.fillRect(3, h - 15, 12, 12, TFT_BLUE);
  ekran2.fillRect(w - 15, h - 15, 12, 12, TFT_YELLOW);
  ekran2.setFont(&fonts::FreeSansBold9pt7b);
  ekran2.setTextColor(0x6E9F);   // jasnoniebieski
  ekran2.setTextDatum(middle_center);
  ekran2.drawString("SPARROW 284x76", w / 2, h / 2 - 8);
  char buf[40];
  snprintf(buf, sizeof(buf), "x=%d y=%d inv=%d rgb=%d", offX, offY, odwroc, zamienRGB);
  ekran2.setFont(&fonts::Font2);
  ekran2.setTextColor(TFT_WHITE);
  ekran2.drawString(buf, w / 2, h / 2 + 14);
  Serial.printf("offset_x=%d offset_y=%d invert=%d rgb_order=%d\n", offX, offY, odwroc, zamienRGB);
}

void zastosuj() {
  ekran2.ustawPanel();
  ekran2.init();
  ekran2.setRotation(1);       // poziomo: 284 x 76
  rysujTest();
}

void setup() {
  Serial.begin(115200);
  delay(1500);                 // czas na otwarcie Serial Monitora
  Serial.println("\n=== Test ekranu 2.25\" 76x284 ===");
  pinMode(P2_BL, OUTPUT);
  digitalWrite(P2_BL, BL_WLACZ);   // podświetlenie
  Serial.println("Podswietlenie (BL, GPIO 47) wlaczone stanem niskim");

  bool ok = ekran2.init();
  Serial.printf("init(): %s\n", ok ? "OK" : "BLAD");
  ekran2.setRotation(1);

  // Miganie całego ekranu - widać, że żyje, nawet przy złym przesunięciu
  const uint16_t kolory[] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_WHITE, TFT_BLACK};
  const char* nazwy[]     = {"CZERWONY", "ZIELONY", "NIEBIESKI", "BIALY", "CZARNY"};
  for (int i = 0; i < 5; i++) {
    Serial.printf("Caly ekran: %s\n", nazwy[i]);
    ekran2.fillScreen(kolory[i]);
    delay(800);
  }
  rysujTest();
  Serial.println("Gotowe - teraz powinna byc ramka i napis.");
}

void loop() {
  if (!Serial.available()) return;
  char c = Serial.read();
  switch (c) {
    case 'x': offX--; break;
    case 'X': offX++; break;
    case 'y': offY--; break;
    case 'Y': offY++; break;
    case 'i': odwroc = !odwroc; break;
    case 'r': zamienRGB = !zamienRGB; break;
    default: return;
  }
  zastosuj();
}
