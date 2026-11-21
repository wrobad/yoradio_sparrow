// Konfiguracja wyświetlacza ILI9341 2.8" + dotyk XPT2046 (LovyanGFX)
#pragma once
#include <LovyanGFX.hpp>

#define PIN_SCK      12
#define PIN_MOSI     11
#define PIN_MISO     13
#define PIN_TFT_CS   10
#define PIN_TFT_DC    9
#define PIN_TFT_RST  14
#define PIN_TFT_BL    8
#define PIN_TOUCH_CS  4

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ILI9341 _panel;
  lgfx::Bus_SPI       _bus;
  lgfx::Touch_XPT2046 _touch;

public:
  LGFX() {
    {
      auto cfg = _bus.config();
      cfg.spi_host    = SPI2_HOST;
      cfg.spi_mode    = 0;
      cfg.freq_write  = 40000000;
      cfg.freq_read   = 16000000;
      cfg.spi_3wire   = false;
      cfg.use_lock    = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = PIN_SCK;
      cfg.pin_mosi = PIN_MOSI;
      cfg.pin_miso = PIN_MISO;
      cfg.pin_dc   = PIN_TFT_DC;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    {
      auto cfg = _panel.config();
      cfg.pin_cs   = PIN_TFT_CS;
      cfg.pin_rst  = PIN_TFT_RST;
      cfg.pin_busy = -1;
      cfg.panel_width  = 240;
      cfg.panel_height = 320;
      cfg.readable   = true;
      cfg.invert     = false;  // jeśli w teście kolory były w negatywie → true
      cfg.rgb_order  = false;  // jeśli czerwony/niebieski zamienione → true
      cfg.dlen_16bit = false;
      cfg.bus_shared = true;
      _panel.config(cfg);
    }
    // Podświetlenie NIE jest sterowane przez LovyanGFX (Light_PWM nie działa
    // z rdzeniem ESP32 4.x) - obsługuje je podswietlenie.h.
    {
      auto cfg = _touch.config();
      cfg.x_min = 0;  cfg.x_max = 239;
      cfg.y_min = 0;  cfg.y_max = 319;
      cfg.pin_int    = -1;
      cfg.bus_shared = true;
      cfg.spi_host   = SPI2_HOST;
      cfg.freq       = 1000000;
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

// =====================================================================
// Ekran 2: podłużny 2.25" 76x284 (ST7789P3) na drugiej szynie SPI (SPI3)
// Ustawienia sprawdzone szkicem test_ekranu_2: przesunięcie 82/18, bez inwersji.
// =====================================================================
#define P2_SCK  41
#define P2_MOSI 42
#define P2_RST  39
#define P2_DC   40
#define P2_CS   38
#define P2_BL   47     // podświetlenie świeci przy stanie NISKIM (PNP na module)

class LGFX_Pasek : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel;
  lgfx::Bus_SPI      _bus;

public:
  LGFX_Pasek() {
    {
      auto b = _bus.config();
      b.spi_host    = SPI3_HOST;
      b.spi_mode    = 0;
      b.freq_write  = 40000000;   // jeśli pojawią się zakłócenia obrazu → 20000000
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
    }
    {
      auto p = _panel.config();
      p.pin_cs   = P2_CS;
      p.pin_rst  = P2_RST;
      p.pin_busy = -1;
      p.panel_width   = 76;
      p.panel_height  = 284;
      p.memory_width  = 240;
      p.memory_height = 320;
      p.offset_x = 82;
      p.offset_y = 18;
      p.readable   = false;
      p.invert     = false;
      p.rgb_order  = false;
      p.bus_shared = false;
      _panel.config(p);
    }
    setPanel(&_panel);
  }
};
