// Test wyświetlacza ILI9341 2.8" + dotyk XPT2046 na ESP32-S3-N8R2
// Biblioteka: LovyanGFX (Menedżer bibliotek Arduino → "LovyanGFX")
// Płytka: ESP32S3 Dev Module, PSRAM: QSPI PSRAM, Flash: 8MB

#include <LovyanGFX.hpp>

// ---- Piny ----
#define PIN_SCK   12
#define PIN_MOSI  11
#define PIN_MISO  13
#define PIN_TFT_CS 10
#define PIN_TFT_DC  9
#define PIN_TFT_RST 14
#define PIN_TFT_BL  8
#define PIN_TOUCH_CS 4

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ILI9341 _panel;
  lgfx::Bus_SPI       _bus;
  lgfx::Touch_XPT2046 _touch;

public:
  LGFX() {
    { // Szyna SPI
      auto cfg = _bus.config();
      cfg.spi_host   = SPI2_HOST;
      cfg.spi_mode   = 0;
      cfg.freq_write = 40000000;   // jeśli są artefakty, zmniejsz do 27000000
      cfg.freq_read  = 16000000;
      cfg.spi_3wire  = false;
      cfg.use_lock   = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = PIN_SCK;
      cfg.pin_mosi = PIN_MOSI;
      cfg.pin_miso = PIN_MISO;
      cfg.pin_dc   = PIN_TFT_DC;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    { // Panel
      auto cfg = _panel.config();
      cfg.pin_cs   = PIN_TFT_CS;
      cfg.pin_rst  = PIN_TFT_RST;
      cfg.pin_busy = -1;
      cfg.panel_width  = 240;
      cfg.panel_height = 320;
      cfg.offset_rotation = 0;
      cfg.readable   = true;
      cfg.invert     = false;  // jeśli kolory są "w negatywie" → true
      cfg.rgb_order  = false;  // jeśli czerwony i niebieski są zamienione → true
      cfg.dlen_16bit = false;
      cfg.bus_shared = true;   // szyna współdzielona z dotykiem i SD
      _panel.config(cfg);
    }
    { // Dotyk
      auto cfg = _touch.config();
      cfg.x_min = 0;   cfg.x_max = 239;   // nadpisze to kalibracja
      cfg.y_min = 0;   cfg.y_max = 319;
      cfg.pin_int = -1;
      cfg.bus_shared = true;
      cfg.offset_rotation = 0;
      cfg.spi_host = SPI2_HOST;
      cfg.freq     = 1000000;
      cfg.pin_sclk = PIN_SCK;
      cfg.pin_mosi = PIN_MOSI;
      cfg.pin_miso = PIN_MISO;
      cfg.pin_cs   = PIN_TOUCH_CS;
      _touch.config(cfg);
      _panel.setTouch(&_touch);
    }
    setPanel(&_panel);
  }
};

LGFX tft;

void setup() {
  Serial.begin(115200);
  delay(500);

  tft.init();
  tft.setRotation(1);      // poziomo 320x240
  pinMode(PIN_TFT_BL, OUTPUT);       // podświetlenie wł. (bez PWM)
  digitalWrite(PIN_TFT_BL, HIGH);

  // --- Kalibracja dotyku: dotknij kolejno wskazanych rogów ---
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(middle_center);
  tft.setFont(&fonts::Font2);
  tft.drawString("Dotknij strzalek w rogach", tft.width() / 2, tft.height() / 2);

  uint16_t cal[8];
  tft.calibrateTouch(cal, TFT_WHITE, TFT_BLACK, 20);
  tft.setTouchCalibrate(cal);

  Serial.print("Kalibracja: {");
  for (int i = 0; i < 8; i++) { Serial.print(cal[i]); if (i < 7) Serial.print(", "); }
  Serial.println("}");

  // --- Ekran testowy ---
  tft.fillScreen(TFT_BLACK);
  int w = tft.width() / 4;
  tft.fillRect(0,     0, w, 40, TFT_RED);
  tft.fillRect(w,     0, w, 40, TFT_GREEN);
  tft.fillRect(w * 2, 0, w, 40, TFT_BLUE);
  tft.fillRect(w * 3, 0, w, 40, TFT_WHITE);

  tft.setTextDatum(top_left);
  tft.setFont(&fonts::Font4);
  tft.setTextColor(TFT_CYAN);
  tft.drawString("yoRadio - test", 10, 55);
  tft.setFont(&fonts::Font2);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Paski: czerwony, zielony, niebieski, bialy", 10, 90);
  tft.drawString("Rysuj palcem / rysikiem po ekranie", 10, 110);
}

void loop() {
  int32_t x, y;
  if (tft.getTouch(&x, &y)) {
    tft.fillCircle(x, y, 3, TFT_YELLOW);
    Serial.printf("Dotyk: x=%d y=%d\n", x, y);
  }
}
