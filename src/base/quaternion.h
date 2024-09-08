// Copyright (C) 2022 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

#pragma once

struct Quaternion
{
  Vec3f v = Vec3f(0, 0, 0);
  float s = 0;

  static Quaternion identity()
  {
    return Quaternion { { 0, 0, 0 }, 1 };
  }

  static Quaternion rotation(Vec3f axis, float angle);
  static Quaternion fromEuler(float yaw, float pitch, float roll);

  Quaternion operator * (Quaternion q) const
  {
    auto const& p = *this;
    Quaternion r;
    // p * q = [( pS qV + qS pV + pV x qV ), ( pS qS - pV qV )]
    r.v = p.s * q.v + q.s * p.v + crossProduct(p.v, q.v);
    r.s = p.s * q.s - dotProduct(p.v, q.v);
    return r;
  }

  Quaternion operator * (float f) const
  {
    return Quaternion { v* f, s* f };
  }

  void operator *= (float f)
  {
    *this = *this * f;
  }

  float magnitude() const;

  Quaternion conjugate() const
  {
    return Quaternion { v* -1.0f, s };
  }

  Quaternion normalized() const
  {
    return *this * (1.0 / magnitude());
  }

  Vec3f rotate(Vec3f v) const
  {
    return (*this * Quaternion { v, 0 } *this->conjugate()).v;
  }
};

