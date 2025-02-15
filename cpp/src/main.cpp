#include <stdio.h>

#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "pico/stdlib.h"

#include "lgfx_ili9488.hpp"
#include "lcd_service.hpp"
#include "inochi_no_kagayaki.hpp"

namespace shapoco::shapopad {

using namespace lgfx;

uint64_t nextUpdateTimeUs = 0;

LcdService screen(480, 320, 3);
InochiNoKagayaki world;

void setup(void) {
  set_sys_clock_khz(250000, true);
  sleep_ms(100);

  stdio_init_all();
  sleep_ms(500);

  uint64_t nowMs = time_us_64() / 1000;
  screen.init(nowMs);
  world.init(nowMs);

#ifdef BOARD_PICO_W
#else
  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#endif

  gpio_init(13);
  gpio_set_dir(13, GPIO_OUT);
  gpio_put(13, true);
}

void loop(void) {
  uint64_t nowUs = time_us_64();
  uint64_t nowMs = nowUs / 1000;

  if (nowUs >= nextUpdateTimeUs && world.idle()) {
    nextUpdateTimeUs += 1000 * 1000 / 60;
    nextUpdateTimeUs = nowUs > nextUpdateTimeUs ? nowUs : nextUpdateTimeUs;
    screen.paintFps(nowMs);
    screen.flip();
    world.update(nowMs);
  }
  screen.serviceStart(nowMs);
  world.servicePaint(nowMs, screen.getBackBuffer());
  screen.serviceEnd(nowMs);
} 

int main(void) {
  setup();
  while (true) {
    loop();
  }
}
}  // namespace shapoco::shapopad

int main(void) {
  return shapoco::shapopad::main();
}

