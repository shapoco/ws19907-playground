#pragma once

#include <stdint.h>
#include <math.h>
#include <vector>

#include "lgfx_ili9488.hpp"

namespace shapoco::inochi {

using namespace lgfx;

using real = float;

static constexpr int NUM_INITIAL_BALLS = 15;
static const real VIEW_RADIUS = 8;
static const real CIRCLE_RADIUS = 4;

struct VecR;

struct VecI {
  int x, y;
};

struct VecR {
  real x = 0;
  real y = 0;
  VecR() {}
  VecR(real x, real y) : x(x), y(y) {}
  VecR(const VecI &v) : x(v.x), y(v.y) {}
  VecR operator+(const VecR &v) const { return VecR(x + v.x, y + v.y); }
  VecR operator-(const VecR &v) const { return VecR(x - v.x, y - v.y); }
  VecR operator*(real s) const { return VecR(x * s, y * s); }
  VecR operator/(real s) const { return VecR(x / s, y / s); }
  VecR& operator+=(const VecR &v) { x += v.x; y += v.y; return *this; }
  VecR& operator-=(const VecR &v) { x -= v.x; y -= v.y; return *this; }
  VecR& operator*=(real s) { x *= s; y *= s; return *this; }
  real abs() const { return sqrt(x * x + y * y); }
  VecI roundToInt() const { return VecI{(int)round(x), (int)round(y)}; }
};

struct TouchState {
  VecI pos;
  bool touched = false;
};

enum Palette {
  BLACK = 0,
  RED = 1,
  BLUE = 2,
  WHITE = 3,
};

struct HostAPI {
  uint64_t (*getTimeMs)();
  VecI (*getScreenSize)();
  void (*clearScreen)();
  void (*drawCircle)(VecI pos, int r, Palette col);
  void (*getTouchState)(TouchState *state);
};

static inline real max(real a, real b) {
  return a > b ? a : b;
}

static inline real min(real a, real b) {
  return a < b ? a : b;
}

static inline real sign(real x) {
  return x < 0 ? -1 : 1;
}

static real randFloat() {
  return (real)rand() / RAND_MAX;
}

class Ball;
class InochiNoKakera;

class Context {
public:
  int nextId = 1;
  uint64_t nowMs;
  real deltaMs;
  VecI screenSize;
  
  std::vector<Ball*> ball;
  std::vector<InochiNoKakera*> fragments;
  VecR touchDownPos;
  VecR touchMovePos;
  VecR touchMoveVel;
  bool touching = false;
  bool fragging = false;
  int dragTargetBallId = -1;
  HostAPI intf;

  void getViewPort(VecR *origin, real *radius) {
    origin->x = (real)screenSize.x / 2;
    origin->y = (real)screenSize.y / 2;
    *radius = min(origin->x, origin->y);
  }

  VecR screenToWorld(VecI pos) {
    VecR viewOrigin;
    real viewRadius;
    getViewPort(&viewOrigin, &viewRadius);
    return (((VecR)pos) - viewOrigin) * VIEW_RADIUS / viewRadius;
  }
    
  void fillCircle(VecR pos, real r, Palette col) {
    VecR viewOrigin;
    real viewRadius;
    getViewPort(&viewOrigin, &viewRadius);
    VecI posInt = (pos * viewRadius / VIEW_RADIUS + viewOrigin).roundToInt();
    int rInt = min(viewRadius / 2, max(1, r * viewRadius / VIEW_RADIUS));
    intf.drawCircle(posInt, rInt, col);
  }

};

class InochiNoKakera {
public:
  VecR pos;
  VecR vec;
  real r;

  InochiNoKakera(VecR pos) : pos(pos) {
    vec.x = 1.0 * (randFloat() - 0.5);
    vec.y = 1.0 * (randFloat() - 0.5);
    r = 1.0;
  }

  void odoru(Context &ctx) {
    real aCoeff = pow(0.0018, ctx.deltaMs);
    real vCoeff = 60 * ctx.deltaMs;
    vec *= aCoeff;
    pos += vec * vCoeff;
    r -= 3 * ctx.deltaMs;
  }

  void kagayaku(Context &ctx) {
    if (r > 0.0) {
      ctx.fillCircle(pos, r, Palette::RED);
    }
  }
};
  
class Ball {
public:
  const int id;
  VecR bodyPos;
  real bodySize = 0;
  VecR bodyPosVel;
  real bodySizeVel = 0;
  real orbitGoal = CIRCLE_RADIUS;
  real bodySizeGoal = 1;
  bool eyeOpened = false;
  VecR eyePos;
  VecR eyePosGoal;
  VecR irisPos;
  VecR irisPosGoal;
  bool alive = true;

  struct Nearest {
    Ball *ball = nullptr;
    VecR vec;
    real dist = 1e10;
  };

  Ball(Context &ctx, VecR pos) : id(ctx.nextId++), bodyPos(pos) {
    bounce();
  }

  void interact(Context &ctx) {
    if (!alive) return;
    if (ctx.dragTargetBallId == id) return;
    
    Nearest nearest[2];
    int nearCount = 0;

    for (Ball *ball : ctx.ball) {
      if (ball->id == id) continue;
      if (!ball->alive) continue;

      VecR vec = ball->bodyPos - bodyPos;
      real dist = vec.abs();

      if (dist < nearest[0].dist) {
        nearest[1] = nearest[0];
        nearest[0].ball = ball;
        nearest[0].vec = vec;
        nearest[0].dist = dist;
        nearCount += 1;
        if (nearCount > 2) nearCount = 2;
      }
      else if (dist < nearest[1].dist) {
        nearest[1].ball = ball;
        nearest[1].vec = vec;
        nearest[1].dist = dist;
        nearCount += 1;
        if (nearCount > 2) nearCount = 2;
      }
    }
    
    for (int i = 0; i < nearCount; i++) {
      Nearest &near = nearest[i];
      real d = nearest[i].dist;
      if (d < 0.3) {
        die(ctx);
        if (near.ball->id != ctx.dragTargetBallId) {
          near.ball->die(ctx);
        }
        continue;
      }
      real dd = d * d;
      real ddd = dd * d;
      real vCoeff = 60 * ctx.deltaMs;
      this->bodyPosVel += (near.vec * 0.08 / dd - near.vec * 0.15 / ddd) * vCoeff;
    }
  }
  
  void move(Context &ctx) {
    if (!alive) return;
    if (ctx.dragTargetBallId == id) {
      bodyPos = ctx.touchMovePos;
      bodyPosVel = ctx.touchMoveVel;
      return;
    }
    else {
      {
        real posAcc = pow(0.0018, ctx.deltaMs);
        real sizeAcc = pow(0.0000015, ctx.deltaMs);
        bodyPosVel *= posAcc;
        bodySizeVel *= sizeAcc;
      }
      {
        real orbit = max(0.001, bodyPos.abs());
        real orbitErr = orbitGoal - orbit;
        real orbitAcc = 0.3 * sign(orbitErr) * orbitErr * orbitErr;
        VecR bodyAcc = bodyPos * orbitAcc / orbit;
        real sizeAcc = (bodySizeGoal - bodySize) * 0.2;
        real accCoeff = 60 * ctx.deltaMs;
        bodyPosVel += bodyAcc * accCoeff;
        bodySizeVel += sizeAcc * accCoeff;
      }
    }
    
    const real MAX_VEL = 0.5;
    real absVel = bodyPosVel.abs();
    if (absVel > MAX_VEL) {
      real velCoeff = pow(MAX_VEL / absVel, 60 * ctx.deltaMs);
      bodyPosVel *= velCoeff;
    }
    
    {
      real velCoeff = 60 * ctx.deltaMs;
      bodyPos += bodyPosVel * velCoeff;
      bodySize += bodySizeVel * velCoeff;
    }
    
    {
      real a = 0.1;
      eyePos = (eyePos * (1.0 - a)) + ((eyePosGoal - eyePos) * a);
      irisPos = (irisPos * (1.0 - a)) + ((irisPosGoal - irisPos) * a);
    }
  }
  
  void bounce(real eyeRatio = 0.5) {
    if (!alive) return;
    bodyPosVel.x = 0.5 * (randFloat() - 0.5);
    bodyPosVel.y = 0.5 * (randFloat() - 0.5);
    bodySizeVel = 0.5 * randFloat();
    bodySizeGoal = 1.0 + 0.5 * randFloat();
    orbitGoal = CIRCLE_RADIUS + 1.2 * (randFloat() - 0.5);
    eyeOpened = randFloat() < eyeRatio;
    eyePosGoal.x = 0.8 * (randFloat() - 0.5);
    eyePosGoal.y = 0.8 * (randFloat() - 0.5);
  }
  
  void startSaccade() {
    if (!alive) return;
    irisPosGoal.x = 0.8 * (randFloat() - 0.5);
    irisPosGoal.y = 0.8 * (randFloat() - 0.5);
  }
  
  void paintBody(Context &ctx) {
    if (!alive) return;
    if (bodySize <= 0.0) return;
    ctx.fillCircle(bodyPos, bodySize, Palette::RED);
  }
  
  void paintEye(Context &ctx) {
    if (!alive) return;
    if (bodySize <= 0.0) return;
    if (!eyeOpened) return;
    VecR pos = this->bodyPos;
    pos += eyePos * bodySize;
    ctx.fillCircle(pos, bodySize * 0.5, Palette::WHITE);
    pos += irisPos * bodySize * 1.25;
    ctx.fillCircle(pos, bodySize * 0.2, Palette::BLUE);
  }
  
  void die(Context &ctx) {
    alive = false;
    for (int i = 0; i < 8; i++) {
      ctx.fragments.push_back(new InochiNoKakera(bodyPos));
    }
  }
};

class World {
public:
  int paintIndex = 0;
  Context ctx;

  void init(HostAPI &intf) {
    ctx.intf = intf;
    ctx.screenSize = ctx.intf.getScreenSize();

    for (int i = 0; i < NUM_INITIAL_BALLS; i++) {
      real a = 2 * M_PI * i / NUM_INITIAL_BALLS;
      ctx.ball.push_back(new Ball(ctx, VecR(CIRCLE_RADIUS * cos(a), CIRCLE_RADIUS * sin(a))));
    }
  }

  void update() {
    uint64_t lastMs = ctx.nowMs;
    ctx.nowMs = ctx.intf.getTimeMs();
    ctx.deltaMs = (real)(ctx.nowMs - lastMs) / 1000;
    
    bool lastTouched = ctx.touching;
    TouchState touch;
    ctx.intf.getTouchState(&touch);
    ctx.touchMovePos = ctx.screenToWorld(touch.pos);
    ctx.touching = touch.touched;
    if (ctx.touching && !lastTouched) {
      ctx.touchDownPos = ctx.touchMovePos;
    }

    //if (ctx.inochis.size() < NUM_INITIAL_INOCHIS) {
    //  if (randFloat() < 0.01) {
    //    real a = 2 * M_PI * randFloat();
    //    ctx.inochis.push_back(new Inochi(ctx, Vec2d(2 * VIEW_RADIUS * cos(a), 2 * VIEW_RADIUS * sin(a))));
    //  }
    //}
      
    int numInochis = ctx.ball.size();
    if (numInochis > 0) {
      int numOpenEye = 0;
      for(auto ball : ctx.ball) {
        if (ball->eyeOpened) numOpenEye++;
      }
      if (randFloat() < 0.005 * numInochis) {
        int i = floor(randFloat() * (int)ctx.ball.size());
        ctx.ball[i]->bounce(numOpenEye < 0.3 * numInochis ? 0.8 : 0.2);
      }
      if (randFloat() < 0.01 * numInochis) {
        int i = floor(randFloat() * numInochis);
        ctx.ball[i]->startSaccade();
      }
    }
    
    for(auto ball : ctx.ball) {
      ball->interact(ctx);
      ball->move(ctx);
    }
    
    for(auto kakera : ctx.fragments) {
      kakera->odoru(ctx);
    }
    
    paintIndex = 0;
  }

  void servicePaint() {
    if (idle()) return;
    
    if (paintIndex == 0) {
      ctx.intf.clearScreen();
    }

    int n = ctx.ball.size();

    if (paintIndex < n) {
      ctx.ball[paintIndex]->paintBody(ctx);
    }
    else if (paintIndex < 2 * n) {
      ctx.ball[paintIndex - n]->paintEye(ctx);
    }
    else {
      ctx.fragments[paintIndex - 2 * n]->kagayaku(ctx);
    }

    for (int i = 0; i < (int)ctx.ball.size(); ) {
      if (ctx.ball[i]->alive) i++;
      else {
        delete ctx.ball[i];
        ctx.ball.erase(ctx.ball.begin() + i);
      }
    }
    
    for (int i = 0; i < (int)ctx.fragments.size(); ) {
      if (ctx.fragments[i]->r > 0.0) i++;
      else {
        delete ctx.fragments[i];
        ctx.fragments.erase(ctx.fragments.begin() + i);
      }
    }
    
    paintIndex += 1;
  }

  bool idle() {
    int n = (int)(ctx.ball.size()) * 2 + (int)(ctx.fragments.size());
    return paintIndex >= n;
  }

};

}
