// Copyright (C) 2018 - Sebastien Alaiwan <sebastien.alaiwan@gmail.com>
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

  struct col
  {
    float elements[4];

    float const & operator [] (int i) const
    {
      return elements[i];
    }

    float & operator [] (int i)
    {
      return elements[i];
    }
  };

  const col & operator [] (int i) const
  {
    return data[i];
  }

  col & operator [] (int i)
  {
    return data[i];
  }

  col data[4];
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
        sum += A[k][row] * B[col][k];

      r[col][row] = sum;
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
  r[3][0] = v.x;
  r[3][1] = v.y;
  r[3][2] = v.z;
  r[3][3] = 1;
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
  r[1][0] = s.y;
  r[2][0] = s.z;
  r[0][1] = u.x;
  r[1][1] = u.y;
  r[2][1] = u.z;
  r[0][2] = -f.x;
  r[1][2] = -f.y;
  r[2][2] = -f.z;
  r[3][0] = -dotProduct(s, eye);
  r[3][1] = -dotProduct(u, eye);
  r[3][2] = dotProduct(f, eye);
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
  r[2][3] = -1.0;
  r[3][2] = -(2.0 * zFar * zNear) / (zFar - zNear);
  return r;
}

