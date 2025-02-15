#pragma once

#include <stdint.h>
#include <math.h>
#include <vector>

#include "lgfx_ili9488.hpp"

namespace shapoco::shapopad {

using namespace lgfx;

using real = float;

static constexpr int NUM_INITIAL_INOCHIS = 15;
static const real VIEW_RADIUS = 8;
static const real CIRCLE_RADIUS = 4;

enum Palette {
  BLACK = 0,
  RED = 1,
  BLUE = 2,
  WHITE = 3,
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

struct Vec2d {
  real x = 0;
  real y = 0;
  Vec2d() {}
  Vec2d(real x, real y) : x(x), y(y) {}
  Vec2d operator+(const Vec2d &v) const { return Vec2d(x + v.x, y + v.y); }
  Vec2d operator-(const Vec2d &v) const { return Vec2d(x - v.x, y - v.y); }
  Vec2d operator*(real s) const { return Vec2d(x * s, y * s); }
  Vec2d operator/(real s) const { return Vec2d(x / s, y / s); }
  Vec2d& operator+=(const Vec2d &v) { x += v.x; y += v.y; return *this; }
  Vec2d& operator-=(const Vec2d &v) { x -= v.x; y -= v.y; return *this; }
  Vec2d& operator*=(real s) { x *= s; y *= s; return *this; }
  real abs() const { return sqrt(x * x + y * y); }
};

static void drawCircle(LGFX_Sprite &g, Vec2d pos, real r, Palette col) {
  int cx = g.width() / 2;
  int cy = g.height() / 2;
  int posR = min(cx, cy);
  int x = cx + pos.x * posR / VIEW_RADIUS;
  int y = cy + pos.y * posR / VIEW_RADIUS;
  int intR = min(cx / 2, max(1, r * cx * 0.85 / VIEW_RADIUS));
  g.fillCircle(x, y, intR, col);
}

class Inochi;
class InochiNoKakera;

class Context {
public:
  int nextId = 1;
  real dt;
  std::vector<Inochi*> inochis;
  std::vector<InochiNoKakera*> kakeras;
  Vec2d mouseDownPos;
  Vec2d mouseMovePos;
  Vec2d mouseMoveVel;
  bool mousePressing = false;
  bool mouseDragging = false;
  int draggingTargetId = -1;
};

class InochiNoKakera {
public:
  Vec2d pos;
  Vec2d vec;
  real r;

  InochiNoKakera(Vec2d pos) : pos(pos) {
    vec.x = 1.0 * (randFloat() - 0.5);
    vec.y = 1.0 * (randFloat() - 0.5);
    r = 1.0;
  }

  void odoru(Context &ctx) {
    real aCoeff = pow(0.0018, ctx.dt);
    real vCoeff = 60 * ctx.dt;
    vec *= aCoeff;
    pos += vec * vCoeff;
    r -= 3 * ctx.dt;
  }

  void kagayaku(LGFX_Sprite &g) {
    if (r > 0.0) {
      drawCircle(g, pos, r, Palette::RED);
    }
  }
};
  
class Inochi {
public:
  const int id;
  Vec2d pos;
  real r = 0;
  Vec2d vel;
  real velR = 0;
  real tgtOrb = CIRCLE_RADIUS;
  real tgtR = 1;
  bool eyeOpen = false;
  Vec2d eyePos0;
  Vec2d eyePos1;
  Vec2d eyeTgt0;
  Vec2d eyeTgt1;
  bool ikiteru = true;

  struct NearItem {
    Inochi *inochi = nullptr;
    Vec2d vec;
    real dist = 1e10;
  };

  Inochi(Context &ctx, Vec2d pos) : id(ctx.nextId++), pos(pos) {
    haneruBody();
  }

  void odoru(Context &ctx) {
    if (!ikiteru) return;
    if (ctx.draggingTargetId == id) return;
    
    NearItem nears[2];
    int nearCount = 0;

    for (Inochi *inochi : ctx.inochis) {
      if (inochi->id == id) continue;
      if (!inochi->ikiteru) continue;

      Vec2d vec = inochi->pos - pos;
      real dist = vec.abs();

      if (dist < nears[0].dist) {
        nears[1] = nears[0];
        nears[0].inochi = inochi;
        nears[0].vec = vec;
        nears[0].dist = dist;
        nearCount += 1;
        if (nearCount > 2) nearCount = 2;
      }
      else if (dist < nears[1].dist) {
        nears[1].inochi = inochi;
        nears[1].vec = vec;
        nears[1].dist = dist;
        nearCount += 1;
        if (nearCount > 2) nearCount = 2;
      }
    }
    
    for (int i = 0; i < nearCount; i++) {
      NearItem &near = nears[i];
      real d = nears[i].dist;
      if (d < 0.3) {
        shinu(ctx);
        if (near.inochi->id != ctx.draggingTargetId) {
          near.inochi->shinu(ctx);
        }
        continue;
      }
      real dd = d * d;
      real ddd = dd * d;
      real vCoeff = 60 * ctx.dt;
      this->vel += (near.vec * 0.08 / dd - near.vec * 0.15 / ddd) * vCoeff;
    }
  }
  
  void hazumu(Context &ctx) {
    if (!ikiteru) return;
    if (ctx.draggingTargetId == id) {
      pos = ctx.mouseMovePos;
      vel = ctx.mouseMoveVel;
      return;
    }
    else {
      real apCoeff = pow(0.0018, ctx.dt);
      real arCoeff = pow(0.0000015, ctx.dt);
      vel *= apCoeff;
      velR *= arCoeff;
      real orb = max(0.001, pos.abs());
      real dOrb = tgtOrb - orb;
      real accOrb = 0.3 * sign(dOrb) * dOrb * dOrb;
      Vec2d acc = pos * accOrb / orb;
      real accR = (tgtR - r) * 0.2;
      real vCoeff = 60 * ctx.dt;
      vel += acc * vCoeff;
      velR += accR * vCoeff;
    }
    
    const real MAX_VEL_SIZE = 0.5;
    real velSize = vel.abs();
    if (velSize > MAX_VEL_SIZE) {
      real aCoeff = pow(MAX_VEL_SIZE / velSize, 60 * ctx.dt);
      vel *= aCoeff;
    }
    
    {
      real vCoeff = 60 * ctx.dt;
      pos += vel * vCoeff;
      r += velR * vCoeff;
    }
    
    real eyeA = 0.1;
    eyePos0 = (eyePos0 * (1.0 - eyeA)) + ((eyeTgt0 - eyePos0) * eyeA);
    eyePos1 = (eyePos1 * (1.0 - eyeA)) + ((eyeTgt1 - eyePos1) * eyeA);
  }
  
  void haneruBody(real eyeRatio = 0.5) {
    if (!ikiteru) return;
    vel.x = 0.5 * (randFloat() - 0.5);
    vel.y = 0.5 * (randFloat() - 0.5);
    velR = 0.5 * randFloat();
    tgtR = 1.0 + 0.5 * randFloat();
    tgtOrb = CIRCLE_RADIUS + 1.2 * (randFloat() - 0.5);
    eyeOpen = randFloat() < eyeRatio;
    eyeTgt0.x = 0.8 * (randFloat() - 0.5);
    eyeTgt0.y = 0.8 * (randFloat() - 0.5);
  }
  
  void haneruEye() {
    if (!ikiteru) return;
    eyeTgt1.x = 0.8 * (randFloat() - 0.5);
    eyeTgt1.y = 0.8 * (randFloat() - 0.5);
  }
  
  void kagayakuKarada(LGFX_Sprite &g) {
    if (!ikiteru) return;
    if (r <= 0.0) return;
    drawCircle(g, pos, r, Palette::RED);
  }
  
  void kagayakuHitomi(LGFX_Sprite &g) {
    if (!ikiteru) return;
    if (r <= 0.0) return;
    if (!eyeOpen) return;
    Vec2d pos = this->pos;
    pos += eyePos0 * r;
    drawCircle(g, pos, r * 0.5, Palette::WHITE);
    pos += eyePos1 * r * 1.25;
    drawCircle(g, pos, r * 0.2, Palette::BLUE);
  }
  
  void shinu(Context &ctx) {
    ikiteru = false;
    for (int i = 0; i < 8; i++) {
      ctx.kakeras.push_back(new InochiNoKakera(pos));
    }
  }
};

class InochiNoKagayaki {
public:
  int paintIndex = 0;
  uint64_t lastTimeMs = 0;
  Context ctx;

  void init(uint64_t nowMs) {
    for (int i = 0; i < NUM_INITIAL_INOCHIS; i++) {
      real a = 2 * M_PI * i / NUM_INITIAL_INOCHIS;
      ctx.inochis.push_back(new Inochi(ctx, Vec2d(CIRCLE_RADIUS * cos(a), CIRCLE_RADIUS * sin(a))));
    }
  }

  void update(uint64_t nowMs) {
    uint32_t elapsedMs = nowMs - lastTimeMs;
    lastTimeMs = nowMs;
    ctx.dt = (float)elapsedMs / 1000;
    
    //if (ctx.inochis.size() < NUM_INITIAL_INOCHIS) {
    //  if (randFloat() < 0.01) {
    //    float a = 2 * M_PI * randFloat();
    //    ctx.inochis.push_back(new Inochi(ctx, Vec2d(2 * VIEW_RADIUS * cos(a), 2 * VIEW_RADIUS * sin(a))));
    //  }
    //}
      
    if (ctx.inochis.size() > 0) {
      int numOpenEye = 0;
      for(auto inochi : ctx.inochis) {
        if (inochi->eyeOpen) numOpenEye++;
      }
      if (randFloat() < 0.005 * ctx.inochis.size()) {
        int i = floor(randFloat() * ctx.inochis.size());
        ctx.inochis[i]->haneruBody(numOpenEye < 0.3 * ctx.inochis.size() ? 0.8 : 0.2);
      }
      if (randFloat() < 0.01 * ctx.inochis.size()) {
        int i = floor(randFloat() * ctx.inochis.size());
        ctx.inochis[i]->haneruEye();
      }
    }
    
    for(auto inochi : ctx.inochis) {
      inochi->odoru(ctx);
      inochi->hazumu(ctx);
    }
    
    for(auto kakera : ctx.kakeras) {
      kakera->odoru(ctx);
    }
    
    paintIndex = 0;
  }

  void servicePaint(uint64_t nowMs, LGFX_Sprite &g) {
    if (idle()) return;
    
    if (paintIndex == 0) {
      g.clear(Palette::WHITE);
    }

    int n = ctx.inochis.size();

    if (paintIndex < n) {
      ctx.inochis[paintIndex]->kagayakuKarada(g);
    }
    else if (paintIndex < 2 * n) {
      ctx.inochis[paintIndex - n]->kagayakuHitomi(g);
    }
    else {
      ctx.kakeras[paintIndex - 2 * n]->kagayaku(g);
    }

    //for (int i = 0; i < ctx.inochis.size(); ) {
    //  if (ctx.inochis[i]->ikiteru) i++;
    //  else inochis.splice(i, 1);
    //}
    //
    //for (var i = 0; i < kakeras.length; ) {
    //  if (kakeras[i].r > 0.0) i++;
    //  else kakeras.splice(i, 1);
    //}
    
    paintIndex += 1;
  }

  bool idle() {
    int n = (int)(ctx.inochis.size()) * 2 + (int)(ctx.kakeras.size());
    return paintIndex >= n;
  }

};

}
