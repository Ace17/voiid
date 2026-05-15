#pragma once

#include "base/geom.h"
#include "base/quaternion.h"
#include <cstdint>
#include <cstring> // memcmp

#define OFFSET(VertexType, Attribute) \
        (uintptr_t)(&(((VertexType*)nullptr)->Attribute))

struct IFrameBuffer;

struct RenderPass
{
  virtual ~RenderPass() = default;

  struct FrameBuffer
  {
    IFrameBuffer* fb;
  };

  virtual void execute(FrameBuffer dst) = 0;
};

struct Camera
{
  Vec3f pos;
  Quaternion dir;

  bool operator < (const Camera& other) const { return memcmp(this, &other, sizeof(Camera)) < 0; }
  bool operator == (const Camera& other) const { return memcmp(this, &other, sizeof(Camera)) == 0; }
};

static_assert(sizeof(Camera) == 7 * sizeof(float));

