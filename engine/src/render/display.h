// Copyright (C) 2018 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

#pragma once

#include <stdint.h>

#include "base/geom.h"
#include "base/span.h"

struct Display
{
  virtual ~Display() = default;

  virtual void setFullscreen(bool fs) = 0;
  virtual void setHdr(bool enable) = 0;
  virtual void setFsaa(bool enable) = 0;
  virtual void setCaption(const char* caption) = 0;
  virtual void loadModel(int modelId, const char* path) = 0;
  virtual void setCamera(Vector3f pos, Quaternion dir) = 0;
  virtual void setAmbientLight(float ambientLight) = 0;
  virtual void readPixels(Span<uint8_t> dstRgbPixels) = 0;
  virtual void enableGrab(bool enable) = 0;

  // draw functions
  virtual void beginDraw() = 0;
  virtual void endDraw() = 0;
  virtual void drawActor(Rect3f where, Quaternion orientation, int modelId, bool blinking, int actionIdx, float frame) = 0;
  virtual void drawText(Vector2f pos, char const* text) = 0;
};

