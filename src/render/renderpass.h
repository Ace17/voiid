#pragma once

#include "base/geom.h"
#include "base/quaternion.h"
#include <cstdint>

#define OFFSET(VertexType, Attribute) \
        (uintptr_t)(&(((VertexType*)nullptr)->Attribute))

struct IFrameBuffer;

struct RenderPass
{
  virtual ~RenderPass() = default;

  struct FrameBuffer
  {
    IFrameBuffer* fb;
    Vec2i dim;
  };

  virtual void execute(FrameBuffer dst) = 0;
};

struct Camera
{
  Vec3f pos;
  Quaternion dir;
};

