#pragma once

#include <stdint.h>

#define LGFX_USE_V1
#define USE_PICO_SDK (1)
#include <LovyanGFX.hpp>

// https://www.waveshare.com/pico-restouch-lcd-3.5.htm

class LGFX_ILI9488 : public lgfx::LGFX_Device {
  lgfx::Panel_ILI9488 _panel_instance;
  lgfx::Bus_SPI _bus_instance;

public:
  LGFX_ILI9488(int width, int height, int rotation = 0) {
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host = 1;
      cfg.spi_mode = 0;
      cfg.freq_write = 80 * 1000 * 1000;
      cfg.freq_read = 80 * 1000 * 1000;
      cfg.pin_sclk = 10;
      cfg.pin_miso = 12;
      cfg.pin_mosi = 11;
      cfg.pin_dc = 8;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs = 9;
      cfg.pin_rst = 15;
      cfg.pin_busy = -1;
      cfg.panel_width = width;
      cfg.panel_height = height;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = rotation;
      cfg.invert = true;
      cfg.dlen_16bit = true;
      _panel_instance.config(cfg);
    }

    setPanel(&_panel_instance);
  }
};
