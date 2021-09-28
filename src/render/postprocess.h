// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Full-screen post-processing (hdr, bloom)

#pragma once

#include "engine/graphics_backend.h"
#include "renderpass.h"
#include <memory>

struct IGraphicsBackend;
struct PostProcessing;

struct PostProcessRenderPass : RenderPass
{
  PostProcessRenderPass();
  ~PostProcessRenderPass();

  void setup(IGraphicsBackend* backend, Size2i resolution);
  FrameBuffer getInputFrameBuffer() override;
  void execute(FrameBuffer dst) override;

  std::unique_ptr<PostProcessing> postproc;
};

