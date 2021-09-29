#pragma once

#include "base/geom.h"
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
    Size2i dim;
  };

  virtual FrameBuffer getInputFrameBuffer() = 0;
  virtual void execute(FrameBuffer dst) = 0;
};

