#pragma once

#include <stdint.h>
#include <math.h>
#include <vector>

#include "inochi/real.hpp"
#include "inochi/vec.hpp"

namespace shapoco::inochi {

static constexpr int NUM_INITIAL_BALLS = 15;
static const real VIEW_RADIUS = 8;
static const real CIRCLE_RADIUS = 4;

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

static real randR() {
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
  
  std::vector<Ball*> balls;
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
    vec.x = 1.0 * (randR() - 0.5);
    vec.y = 1.0 * (randR() - 0.5);
    r = 1.0;
  }

  void move(Context &ctx) {
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

    for (Ball *ball : ctx.balls) {
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
        kill(ctx);
        if (near.ball->id != ctx.dragTargetBallId) {
          near.ball->kill(ctx);
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
    bodyPosVel.x = 0.5 * (randR() - 0.5);
    bodyPosVel.y = 0.5 * (randR() - 0.5);
    bodySizeVel = 0.5 * randR();
    bodySizeGoal = 1.0 + 0.5 * randR();
    orbitGoal = CIRCLE_RADIUS + 1.2 * (randR() - 0.5);
    eyeOpened = randR() < eyeRatio;
    eyePosGoal.x = 0.8 * (randR() - 0.5);
    eyePosGoal.y = 0.8 * (randR() - 0.5);
  }
  
  void startSaccade() {
    if (!alive) return;
    irisPosGoal.x = 0.8 * (randR() - 0.5);
    irisPosGoal.y = 0.8 * (randR() - 0.5);
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
  
  void kill(Context &ctx) {
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
      ctx.balls.push_back(new Ball(ctx, VecR(CIRCLE_RADIUS * cos(a), CIRCLE_RADIUS * sin(a))));
    }
  }

  void update() {
    uint64_t lastMs = ctx.nowMs;
    ctx.nowMs = ctx.intf.getTimeMs();
    ctx.deltaMs = (real)(ctx.nowMs - lastMs) / 1000;
    
    bool lastTouched = ctx.touching;
    VecR lastMovePos = ctx.touchMovePos;
    TouchState touch;
    ctx.intf.getTouchState(&touch);
    ctx.touchMovePos = ctx.screenToWorld(touch.pos);
    ctx.touchMoveVel = ctx.touchMovePos - lastMovePos;
    ctx.touching = touch.touched;
    if (ctx.touching && !lastTouched) {
      ctx.touchDownPos = ctx.touchMovePos;
      ctx.dragTargetBallId = findByWorldPos(ctx.touchDownPos);
    }
    else if (!ctx.touching && lastTouched) {
      VecR d = ctx.touchMovePos - ctx.touchDownPos;
      real dManhattan = abs(d.x) + abs(d.y);
      if (dManhattan < VIEW_RADIUS / 10) {
        if (ctx.dragTargetBallId >= 0) {
          Ball *ball = findById(ctx.dragTargetBallId);
          if (ball) ball->kill(ctx);
        }
        else {
          ctx.balls.push_back(new Ball(ctx, ctx.touchMovePos));
        }
      }
      ctx.dragTargetBallId = -1;
    }

    if (ctx.balls.size() < NUM_INITIAL_BALLS) {
      if (randR() < 0.01) {
        real a = 2 * M_PI * randR();
        ctx.balls.push_back(new Ball(ctx, VecR(2 * VIEW_RADIUS * cos(a), 2 * VIEW_RADIUS * sin(a))));
      }
    }
      
    int numInochis = ctx.balls.size();
    if (numInochis > 0) {
      int numOpenEye = 0;
      for(auto ball : ctx.balls) {
        if (ball->eyeOpened) numOpenEye++;
      }
      if (randR() < 0.005 * numInochis) {
        int i = floor(randR() * (int)ctx.balls.size());
        ctx.balls[i]->bounce(numOpenEye < 0.3 * numInochis ? 0.8 : 0.2);
      }
      if (randR() < 0.01 * numInochis) {
        int i = floor(randR() * numInochis);
        ctx.balls[i]->startSaccade();
      }
    }
    
    for(auto ball : ctx.balls) {
      ball->interact(ctx);
      ball->move(ctx);
    }
    
    for(auto kakera : ctx.fragments) {
      kakera->move(ctx);
    }
    
    paintIndex = 0;
  }

  void servicePaint() {
    if (idle()) return;
    
    if (paintIndex == 0) {
      ctx.intf.clearScreen();
    }

    int n = ctx.balls.size();

    if (paintIndex < n) {
      ctx.balls[paintIndex]->paintBody(ctx);
    }
    else if (paintIndex < 2 * n) {
      ctx.balls[paintIndex - n]->paintEye(ctx);
    }
    else {
      ctx.fragments[paintIndex - 2 * n]->kagayaku(ctx);
    }

    for (int i = 0; i < (int)ctx.balls.size(); ) {
      if (ctx.balls[i]->alive) i++;
      else {
        delete ctx.balls[i];
        ctx.balls.erase(ctx.balls.begin() + i);
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

  int findByWorldPos(VecR pos) {
    for (Ball *ball : ctx.balls) {
      real dd = (ball->bodyPos - pos).absPow2();
      if (dd < ball->bodySize * ball->bodySize) {
        return ball->id;
      }
    }
    return -1;
  }
  
  Ball *findById(int id) {
    for (Ball *ball : ctx.balls) {
      if (ball->id == id) return ball;
    }
    return nullptr;
  }
  
  bool idle() {
    int n = (int)(ctx.balls.size()) * 2 + (int)(ctx.fragments.size());
    return paintIndex >= n;
  }

};

}
