#include <stdio.h>

#include "hardware/clocks.h"
#include "lgfx_ili9488.hpp"
#include "pico/stdlib.h"

namespace shapoco::shapopad {
LGFX_ILI9488 lcd(320, 480, 1);

int main() {
  stdio_init_all();
  sleep_ms(500);

  // gpio_init(PICO_DEFAULT_LED_PIN);
  // gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

  gpio_init(13);
  gpio_set_dir(13, GPIO_OUT);
  gpio_put(13, true);

  lcd.init();
  lcd.setRotation(1);
  // lcd.setPivot(lcd.width() >> 1, lcd.height() >> 1);

  while (true) {
    // gpio_put(PICO_DEFAULT_LED_PIN, true);

    // gpio_put(13, true);
    // lcd.fillScreen(TFT_BLACK);
    lcd.clear(TFT_BLACK);
    sleep_ms(500);
    // lcd.fillScreen(TFT_WHITE);
    lcd.clear(TFT_WHITE);
    sleep_ms(500);

    // gpio_put(PICO_DEFAULT_LED_PIN, false);

    // gpio_put(13, false);
    // lcd.fillScreen(TFT_RED);
    lcd.clear(TFT_RED);
    sleep_ms(500);
    // lcd.fillScreen(TFT_GREEN);
    lcd.clear(TFT_GREEN);
    sleep_ms(500);
    // lcd.fillScreen(TFT_BLUE);
    lcd.clear(TFT_BLUE);
    sleep_ms(500);
  }
  return 0;
}
}  // namespace shapoco::shapopad

int main(void) {
  return shapoco::shapopad::main();
}

