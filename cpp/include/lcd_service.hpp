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

using namespace lgfx;

class LcdService {
public:
  static constexpr int NUM_BUFFERS = 3;

  const int width;
  const int height;
  const int stride;
  const int rotation;
  LGFX_ILI9488 lcd;
  LGFX_Sprite buffers[NUM_BUFFERS];
  uint16_t *lineBuff;

  int phase = 0;
  int scanY = 0;
  int scanRemaining = 0;
  bool dmaStarted = false;
  bool firstTrans = true;

  uint64_t fpsStartTimeMs = 0;
  int fpsFrameCount = 0;
  float fps = 0;

  LcdService(int width, int height, int rotation) :
    width(width),
    height(height),
    stride((width * BPP + 7) / 8),
    rotation(rotation),
    lcd(width, height, rotation),
    lineBuff(new uint16_t[width])
  { }

  ~LcdService() {
    delete[] lineBuff;
  }

  void init(uint64_t nowMs) {
    lcd.init();
    lcd.setRotation(rotation);
    lcd.setColorDepth(16);
    for (int i = 0; i < NUM_BUFFERS; i++) {
      buffers[i].setColorDepth(BPP);
      buffers[i].createSprite(width, height);
      buffers[i].clear(PALETTE_WHITE);
    }
    firstTrans = true;
    fpsStartTimeMs = nowMs;
    fpsFrameCount = 0;
    fps = 0;
  }

  LGFX_Sprite &getBackBuffer() {
    return buffers[phase & 1];
  }

  LGFX_Sprite &getFrontBuffer() {
    return buffers[(phase + 1) & 1];
  }

  void flip() {
    phase = (phase + 1) & 1;
    if (idle()) {
      scanY = 0;
    }
    scanRemaining = height;
  }

  void serviceStart(uint64_t nowMs) {
    if (idle()) return;

    LGFX_Sprite &spOld = buffers[2];
    LGFX_Sprite &spNew = getFrontBuffer();

    while (true) {
      uint8_t* oldLine = ((uint8_t*)spOld.getBuffer()) + stride * scanY;
      const uint8_t* newLine = ((const uint8_t*)spNew.getBuffer()) + stride * scanY;
      uint16_t* wrPtr = nullptr;
      int startByte = -1;
      for(int ix = 0; ix < stride; ix++) {
        uint8_t oldByte = oldLine[ix];
        uint8_t newByte = newLine[ix];

        bool change = (oldByte != newByte || firstTrans);
        if (change) {
          if (!dmaStarted) {
            lcd.startWrite();   
            dmaStarted = true;     
          }

          if (startByte < 0) {
            startByte = ix;
            wrPtr = lineBuff;
          }

          uint8_t sreg = newByte;
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
        if (startByte >= 0 && (!change || endOfLine)) {
          int endByte = endOfLine ? stride : ix;
          int numBytes = endByte - startByte;
          int startPix = startByte * PIXELS_PER_BYTE;
          int numPixs = numBytes * PIXELS_PER_BYTE;
          lcd.pushImageDMA(startPix, scanY, numPixs, 1, lineBuff);
          memcpy(oldLine + startByte, newLine + startByte, numBytes);
          startByte = -1;
        }
      }

      if (dmaStarted) break;
      stepScanLine(nowMs);
      if (scanRemaining <= 0) break;
    }
  }

  void serviceEnd(uint64_t nowMs) {
    if (idle()) return;

    if (dmaStarted) {
      lcd.endWrite();
      dmaStarted = false;
    }

    stepScanLine(nowMs);
  }

  void stepScanLine(uint64_t nowMs) {
    scanRemaining -= 1;
    if (scanY + 1 < height) {
      scanY += 1;
    }
    else {
      scanY = 0;
      firstTrans = false;
      fpsFrameCount++;
      uint32_t elapsedMs = nowMs - fpsStartTimeMs;
      if (elapsedMs >= 1000) {
        fps = (float)fpsFrameCount * 1000 / elapsedMs;
        fpsStartTimeMs = nowMs;
        fpsFrameCount = 0;
      }
    }
  }

  void paintFps(uint64_t nowMs) {
    LGFX_Sprite &g = getBackBuffer();
    char buf[64];
    snprintf(buf, sizeof(buf), "FPS:%.1f", fps);
    g.setTextColor(PALETTE_BLACK, PALETTE_WHITE);
    g.drawString(buf, 4, 4, 1);
  }

  bool idle() {
    return scanRemaining <= 0;
  }

};

}

