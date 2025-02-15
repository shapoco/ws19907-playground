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

#if 1
static constexpr int SCREEN_WIDTH = 480;
static constexpr int SCREEN_HEIGHT = 320;
LcdService screen(SCREEN_WIDTH, SCREEN_HEIGHT, 3);
#else
static constexpr int SCREEN_WIDTH = 320;
static constexpr int SCREEN_HEIGHT = 480;
LcdService screen(SCREEN_WIDTH, SCREEN_HEIGHT, 0);
#endif

World world;

HostAPI apis;
TouchState touchState;

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

void getTouchState(TouchState *touch) {
#if TOUCH_ENABLED
  touchState.touched = screen.lcd.getTouch(&touchState.pos.x, &touchState.pos.y);
#else
  touchState.touched = false;
#endif
  *touch = touchState;
}

void setup(void) {
  set_sys_clock_khz(250000, true);
  sleep_ms(100);

  stdio_init_all();
  sleep_ms(500);

  uint64_t nowMs = time_us_64() / 1000;
  screen.init(nowMs);

#if TOUCH_ENABLED
#if 1
  if (screen.lcd.touch()) {
    uint16_t calibData[] = {
      0x0f45, 0x0f6b, 0x0f64, 0x00ed,
      0x00ea, 0x0f76, 0x00cd, 0x00f6,
    };
    screen.lcd.setTouchCalibrate(calibData);
  }
#else
  if (screen.lcd.touch()) {
    auto &display = screen.lcd;
    
    if (display.width() < display.height()) display.setRotation(display.getRotation() ^ 1);

    // 画面に案内文章を描画します。
    display.setTextDatum(textdatum_t::middle_center);
    display.drawString("touch the arrow marker.", display.width()>>1, display.height() >> 1);
    display.setTextDatum(textdatum_t::top_left);

    // タッチを使用する場合、キャリブレーションを行います。画面の四隅に表示される矢印の先端を順にタッチしてください。
    uint16_t fg = TFT_WHITE;
    uint16_t bg = TFT_BLACK;
    if (display.isEPD()) std::swap(fg, bg);
    uint16_t calibData[8];
    display.calibrateTouch(calibData, fg, bg, std::max(display.width(), display.height()) >> 3);
    printf("uint16_t calibData[] = {\n");
    for (int i = 0; i < 8; i++) {
      printf("calibData[%d] = 0x%04x,\n", i, calibData[i]);
    }
    printf("};\n");
  }
#endif

#if 0
  bool lastTouched = false;
  int lastX = 0, lastY = 0;
  while(true) {
    if (lastTouched) {
      screen.lcd.startWrite();
      screen.lcd.fillCircle(lastX, lastY, 5, TFT_BLACK);
      screen.lcd.endWrite();
    }

    int x, y;
    bool touched = screen.lcd.getTouch(&x, &y);
    if (touched) {
      screen.lcd.startWrite();
      screen.lcd.fillCircle(x, y, 5, TFT_WHITE);
      screen.lcd.endWrite();
    }
    sleep_ms(10);
    lastTouched = touched;
    lastX = x;
    lastY = y;
  }
#endif
#endif

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

