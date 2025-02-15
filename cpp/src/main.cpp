#include <stdio.h>

#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "pico/stdlib.h"

#include "lgfx_ili9488.hpp"
#include "lcd_service.hpp"
#include "inochi/inochi.hpp"

namespace shapoco {

using namespace lgfx;
using namespace shapoco::inochi;

uint64_t nextUpdateTimeUs = 0;

static constexpr int SCREEN_WIDTH = 480;
static constexpr int SCREEN_HEIGHT = 320;

LcdService screen(SCREEN_WIDTH, SCREEN_HEIGHT, 3);
World world;

HostAPI touch;

uint64_t getTimeMs() {
  return time_us_64() / 1000;
}

VecI getScreenSize() {
  return VecI{SCREEN_WIDTH, SCREEN_HEIGHT};
}

void clearScreen() {
  LGFX_Sprite &g = screen.getBackBuffer();
  g.clear(Palette::WHITE);
}

void drawCircle(VecI pos, int r, Palette col) {
  LGFX_Sprite &g = screen.getBackBuffer();
  g.fillCircle(pos.x, pos.y, r, col);
}

void getTouchState(TouchState *state) {
#if TOUCH_ENABLED
  state->touched = screen.lcd.getTouch(&state->pos.x, &state->pos.y);
#else
  state->touched = false;
#endif
}

void setup(void) {
  set_sys_clock_khz(250000, true);
  sleep_ms(100);

  stdio_init_all();
  sleep_ms(500);

  uint64_t nowMs = time_us_64() / 1000;
  screen.init(nowMs);
  
  HostAPI intf;
  intf.getTimeMs = getTimeMs;
  intf.getScreenSize = getScreenSize;
  intf.clearScreen = clearScreen;
  intf.drawCircle = drawCircle;
  intf.getTouchState = getTouchState;
  world.init(intf);

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

    world.update();
  }
  screen.serviceStart(nowMs);
  world.servicePaint();
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
  return shapoco::main();
}

