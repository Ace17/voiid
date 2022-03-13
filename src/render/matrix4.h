// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Matrix 4x4 class for display

#pragma once

struct Matrix4f
{
  Matrix4f(float init)
  {
    for(int row = 0; row < 4; ++row)
      for(int col = 0; col < 4; ++col)
        (*this)[col][row] = init;
  }

  struct row
  {
    float elements[4];

    float const& operator [] (int i) const
    {
      return elements[i];
    }

    float& operator [] (int i)
    {
      return elements[i];
    }
  };

  const row& operator [] (int i) const
  {
    return data[i];
  }

  row& operator [] (int i)
  {
    return data[i];
  }

  row data[4];
};

inline
Matrix4f operator * (Matrix4f const& A, Matrix4f const& B)
{
  Matrix4f r(0);

  for(int row = 0; row < 4; ++row)
    for(int col = 0; col < 4; ++col)
    {
      double sum = 0;

      for(int k = 0; k < 4; ++k)
        sum += A[row][k] * B[k][col];

      r[row][col] = sum;
    }

  return r;
}

inline
Matrix4f translate(Vector3f v)
{
  Matrix4f r(0);

  r[0][0] = 1;
  r[1][1] = 1;
  r[2][2] = 1;
  r[3][3] = 1;

  r[0][3] = v.x;
  r[1][3] = v.y;
  r[2][3] = v.z;

  return r;
}

inline
Matrix4f scale(Vector3f v)
{
  Matrix4f r(0);
  r[0][0] = v.x;
  r[1][1] = v.y;
  r[2][2] = v.z;
  r[3][3] = 1;
  return r;
}

inline
Matrix4f lookAt(Vector3f eye, Vector3f center, Vector3f up)
{
  auto f = normalize(center - eye);
  auto s = normalize(crossProduct(f, up));
  auto u = crossProduct(s, f);

  Matrix4f r(0);
  r[0][0] = s.x;
  r[0][1] = s.y;
  r[0][2] = s.z;
  r[1][0] = u.x;
  r[1][1] = u.y;
  r[1][2] = u.z;
  r[2][0] = -f.x;
  r[2][1] = -f.y;
  r[2][2] = -f.z;
  r[0][3] = -dotProduct(s, eye);
  r[1][3] = -dotProduct(u, eye);
  r[2][3] = dotProduct(f, eye);
  r[3][3] = 1;
  return r;
}

inline
Matrix4f perspective(float fovy, float aspect, float zNear, float zFar)
{
  assert(aspect != 0.0);
  assert(zFar != zNear);

  auto const rad = fovy;
  auto const tanHalfFovy = tan(rad / 2.0);

  Matrix4f r(0);
  r[0][0] = 1.0 / (aspect * tanHalfFovy);
  r[1][1] = 1.0 / (tanHalfFovy);
  r[2][2] = -(zFar + zNear) / (zFar - zNear);
  r[3][2] = -1.0;
  r[2][3] = -(2.0 * zFar * zNear) / (zFar - zNear);
  return r;
}

inline
Matrix4f quaternionToMatrix(const Quaternion& q)
{
  const auto qx = q.v.x;
  const auto qy = q.v.y;
  const auto qz = q.v.z;
  const auto qw = q.s;

  Matrix4f r(0);

  r[0][0] = 1.0f - 2.0f * qy * qy - 2.0f * qz * qz;
  r[1][0] = 2.0f * qx * qy - 2.0f * qz * qw;
  r[2][0] = 2.0f * qx * qz + 2.0f * qy * qw;
  r[3][0] = 0;

  r[0][1] = 2.0f * qx * qy + 2.0f * qz * qw;
  r[1][1] = 1.0f - 2.0f * qx * qx - 2.0f * qz * qz;
  r[2][1] = 2.0f * qy * qz - 2.0f * qx * qw;
  r[3][1] = 0;

  r[0][2] = 2.0f * qx * qz - 2.0f * qy * qw;
  r[1][2] = 2.0f * qy * qz + 2.0f * qx * qw;
  r[2][2] = 1.0f - 2.0f * qx * qx - 2.0f * qy * qy;
  r[3][2] = 0;

  r[0][3] = 0;
  r[1][3] = 0;
  r[2][3] = 0;
  r[3][3] = 1;

  return r;
}

