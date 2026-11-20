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
  lgfx::Light_PWM     _light;
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
    {
      auto cfg = _light.config();
      cfg.pin_bl = PIN_TFT_BL;
      cfg.invert = false;
      cfg.freq   = 44100;
      cfg.pwm_channel = 7;
      _light.config(cfg);
      _panel.setLight(&_light);
    }
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
