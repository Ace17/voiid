// Copyright (C) 2022 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

#include "geom.h"
#include "quaternion.h"

#include <cmath>

template<typename T>
double magnitude(GenericVector3<T> v)
{
  return sqrt(dotProduct(v, v));
}

template double magnitude(GenericVector3<double> v);
template double magnitude(GenericVector3<float> v);

float Quaternion::magnitude() const
{
  return sqrt(v.x * v.x + v.y * v.y + v.z * v.z + s * s);
}

Quaternion Quaternion::rotation(Vec3f axis, float angle)
{
  Quaternion r;
  r.v = axis * sin(angle / 2);
  r.s = cos(angle / 2);
  return r.normalized();
}

Quaternion Quaternion::fromEuler(float yaw, float pitch, float roll)
{
  // Abbreviations for the various angular functions
  auto const cy = cos(yaw * 0.5);
  auto const sy = sin(yaw * 0.5);
  auto const cp = cos(pitch * 0.5);
  auto const sp = sin(pitch * 0.5);
  auto const cr = cos(roll * 0.5);
  auto const sr = sin(roll * 0.5);

  Quaternion q;
  q.s = cy * cp * cr + sy * sp * sr;
  q.v.x = cy * cp * sr - sy * sp * cr;
  q.v.y = sy * cp * sr + cy * sp * cr;
  q.v.z = sy * cp * cr - cy * sp * sr;

  return q;
}

