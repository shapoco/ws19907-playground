#pragma once

#include <stdint.h>

#include "lgfx_ili9488.hpp"

namespace shapoco::shapopad {

static constexpr int BPP = 2;
static constexpr int PIXELS_PER_BYTE = (8 + BPP - 1) / BPP;

static constexpr uint8_t PALETTE_BLACK = 0;
static constexpr uint8_t PALETTE_RED = 1;
static constexpr uint8_t PALETTE_BLUE = 2;
static constexpr uint8_t PALETTE_WHITE = 3;
static constexpr uint8_t PALETTE_MASK = (1 << BPP) - 1;

static constexpr int MAX_INOCHIS = 16;

using namespace lgfx;

class Screen {
public:
  const int width;
  const int height;
  const int stride;
  const int rotation;
  LGFX_ILI9488 lcd;
  LGFX_Sprite buff1;
  LGFX_Sprite buff2;
  uint16_t *lineBuff;
  bool flip = false;
  bool firstTrans = true;

  uint64_t fpsStartTimeMs = 0;
  int fpsFrameCount = 0;
  float fps = 0;

  struct Inochi {
    int x;
    int y;
    int dx;
    int dy;
  };
  
  Inochi inochis[MAX_INOCHIS];
  
  Screen(int width, int height, int rotation) :
    width(width),
    height(height),
    stride((width * BPP + 7) / 8),
    rotation(rotation),
    lcd(width, height, rotation),
    buff1(),
    buff2(),
    lineBuff(new uint16_t[width])
  { }

  ~Screen() {
    delete[] lineBuff;
  }

  void init(uint64_t nowMs) {
    lcd.init();
    lcd.setRotation(rotation);
    lcd.setColorDepth(16);
    for (int iBuff = 0; iBuff < 2; iBuff++) {
      LGFX_Sprite &sp = iBuff == 0 ? buff1 : buff2;
      sp.setColorDepth(BPP);
      sp.createSprite(width, height);
      sp.clear(PALETTE_WHITE);
    }
    for (int iInochi = 0; iInochi < MAX_INOCHIS; iInochi++) {
      Inochi &inochi = inochis[iInochi];
      inochi.x = (rand() % width) << 8;
      inochi.y = (rand() % height) << 8;
      inochi.dx = rand() % 512 - 256;
      inochi.dy = rand() % 512 - 256;
    }
    firstTrans = true;
    fpsStartTimeMs = nowMs;
    fpsFrameCount = 0;
    fps = 0;
  }

  void update(uint64_t nowMs) {
    LGFX_Sprite &front = flip ? front : buff1;
    LGFX_Sprite &back = flip ? back : front;
    lcd.endWrite();

    front.clear(PALETTE_WHITE);
    for (int i = 0; i < MAX_INOCHIS; i++) {
      Inochi &inochi = inochis[i];
      front.fillCircle(inochi.x >> 8, inochi.y >> 8, 32, 1 + i % 2);
      inochi.x += inochi.dx;
      inochi.y += inochi.dy;
      if (inochi.x < 0 || inochi.x >= (width << 8)) {
        inochi.dx = -inochi.dx;
        inochi.x += inochi.dx;
      }
      if (inochi.y < 0 || inochi.y >= (height << 8)) {
        inochi.dy = -inochi.dy;
        inochi.y += inochi.dy;
      }
    }

    {
      char buf[64];
      snprintf(buf, sizeof(buf), "FPS:%.1f", fps);
      front.setTextColor(PALETTE_BLACK, PALETTE_WHITE);
      front.drawString(buf, 4, 4, 1);
    }

    commit(back, front);
    flip = !flip;

    {
      fpsFrameCount++;
      uint32_t elapsedMs = nowMs - fpsStartTimeMs;
      if (elapsedMs >= 1000) {
        fps = (float)fpsFrameCount * 1000 / elapsedMs;
        fpsStartTimeMs = nowMs;
        fpsFrameCount = 0;
      }
    }
  }

  void commit(const LGFX_Sprite &sp0, const LGFX_Sprite &sp1) {
    const uint8_t* line0 = (const uint8_t*)sp0.getBuffer();
    const uint8_t* line1 = (const uint8_t*)sp1.getBuffer();

    lcd.startWrite();
    for(int y = 0; y < height; y++) {
      const uint8_t* rdPtr0 = line0;
      const uint8_t* rdPtr1 = line1;
      uint16_t* wrPtr = nullptr;
      int iStartByte = -1;
      for(int ix = 0; ix < stride; ix++) {
        uint8_t byte0 = *(rdPtr0++);
        uint8_t byte1 = *(rdPtr1++);

        bool change = (byte0 != byte1 || firstTrans);
        if (change) {
          if (iStartByte < 0) {
            iStartByte = ix;
            wrPtr = lineBuff;
          }

          uint8_t sreg = byte1;
          for (int ipix = 0; ipix < PIXELS_PER_BYTE; ipix++) {
            uint8_t colIndex = (sreg >> (8 - BPP)) & PALETTE_MASK;
            switch (colIndex) {
            case PALETTE_RED: *wrPtr = 0x00f8; break;
            case PALETTE_BLACK: *wrPtr = 0x0000; break;
            case PALETTE_BLUE: *wrPtr = 0x1f00; break;
            default: *wrPtr = 0xffff; break;
            }
            wrPtr++;
            sreg <<= BPP;            
          }
        }

        bool endOfLine = ix >= stride - 1;
        if (iStartByte >= 0 && (!change || endOfLine)) {
          int iEndByte = endOfLine ? stride : ix;
          int pixStart = iStartByte * 8 / BPP;
          int pixEnd = iEndByte * 8 / BPP;
          lcd.pushImageDMA(pixStart, y, pixEnd - pixStart, 1, lineBuff);
          iStartByte = -1;
        }
      }

      line0 += stride;
      line1 += stride;
    }

    firstTrans = false;
  }

};

}

