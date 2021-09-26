#pragma once

#include <cstdint>

struct Shader
{
  operator uintptr_t () { return program; }
  uintptr_t program;
};

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

