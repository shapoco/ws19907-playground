#pragma once

#include <stdint.h>

#define LGFX_USE_V1

// https://x.com/lovyan03/status/1504298966577725441
// #define LV_COLOR_16_SWAP (1)

#include <LovyanGFX.hpp>

#include <hardware/pwm.h>

// https://www.waveshare.com/pico-restouch-lcd-3.5.htm

namespace shapoco::shapopad {

class LGFX_ILI9488 : public lgfx::LGFX_Device {
  lgfx::Panel_ILI9488 _panel_instance;
  lgfx::Bus_SPI _bus_instance;
  lgfx::Light_PWM _light_instance;

 public:
  LGFX_ILI9488(int width, int height, int rotation = 0) {
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host = 1;
      cfg.spi_mode = 0;
      cfg.freq_write = 40 * 1000 * 1000;
      cfg.freq_read = 20 * 1000 * 1000;
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
      cfg.panel_width = (rotation & 1) ? height : width;
      cfg.panel_height = (rotation & 1) ? width : height;
      cfg.memory_width = cfg.panel_width;
      cfg.memory_height = cfg.panel_height;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits = 1;
      cfg.readable = true;
      cfg.invert = true;
      cfg.dlen_16bit = true;
      _panel_instance.config(cfg);
    }

    {
      auto cfg = _light_instance.config();
      cfg.pin_bl = 13;
      cfg.invert = false;
      cfg.freq = 44100;
      cfg.pwm_channel = pwm_gpio_to_channel(cfg.pin_bl);

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }

    setPanel(&_panel_instance);
  }
};

}  // namespace shapoco::shapopad
