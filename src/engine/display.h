// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// interface to a high-level renderer

#pragma once

#include "base/box.h"
#include "base/geom.h"
#include "base/quaternion.h"
#include "base/string.h"

struct Display
{
  virtual ~Display() = default;

  virtual void setHdr(bool enable) = 0;
  virtual void setFsaa(bool enable) = 0;
  virtual void loadModel(int modelId, String path) = 0;
  virtual void setCamera(Vector3f pos, Quaternion dir) = 0;
  virtual void setAmbientLight(float ambientLight) = 0;
  virtual void setLight(int idx, Vector3f pos, Vector3f color) = 0;

  // draw functions
  virtual void beginDraw() = 0;
  virtual void endDraw() = 0;
  virtual void drawActor(Rect3f where, Quaternion orientation, int modelId, bool blinking, int actionIdx, float frame) = 0;
  virtual void drawText(Vector2f pos, String text) = 0;
};

