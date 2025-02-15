#pragma once

#include <math.h>
#include "real.hpp"

namespace shapoco::inochi {

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
  real absPow2() const { return  x * x + y * y; }
  real abs() const { return sqrt(absPow2()); }
  VecI roundToInt() const { return VecI{(int)round(x), (int)round(y)}; }
};

}
