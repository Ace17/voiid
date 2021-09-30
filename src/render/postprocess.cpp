// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// Full-screen post-processing (hdr, bloom)

#include "postprocess.h"

namespace
{
struct QuadVertex
{
  float x, y, u, v;
};

const QuadVertex screenQuad[] =
{
  { -1, -1, 0, 0 },
  { +1, +1, 1, 1 },
  { -1, +1, 0, 1 },

  { -1, -1, 0, 0 },
  { +1, -1, 1, 0 },
  { +1, +1, 1, 1 },
};

namespace HdrShader
{
enum Uniform
{
  InputTex1 = 0,
  InputTex2 = 1,
};

enum Attribute
{
  positionLoc = 0,
  uvLoc = 1,
};
}

namespace BloomShader
{
enum Uniform
{
  InputTex = 0,
  IsThreshold = 1,
};

enum Attribute
{
  positionLoc = 0,
  uvLoc = 1,
};
}
}

struct PostProcessing
{
  PostProcessing(IGraphicsBackend* backend, Size2i resolution)
    : m_resolution(resolution), backend(backend)
  {
    m_hdrShader = backend->createGpuProgram("hdr", false);
    m_bloomShader = backend->createGpuProgram("bloom", false);

    m_quadVbo = backend->createVertexBuffer();
    m_quadVbo->upload(screenQuad, sizeof screenQuad);

    m_hdrFramebuffer = backend->createFrameBuffer(resolution, true);

    for(int k = 0; k < 2; ++k)
      m_bloomFramebuffer[k] = backend->createFrameBuffer(resolution, false);
  }

  void applyBloomFilter()
  {
    backend->useGpuProgram(m_bloomShader.get());

    backend->useVertexBuffer(m_quadVbo.get());

    backend->enableVertexAttribute(BloomShader::Attribute::positionLoc, 2, sizeof(QuadVertex), OFFSET(QuadVertex, x));
    backend->enableVertexAttribute(BloomShader::Attribute::uvLoc, 2, sizeof(QuadVertex), OFFSET(QuadVertex, u));

    auto oneBlurringPass = [&] (ITexture* inputTex, IFrameBuffer* outputFramebuffer, bool isThreshold = false)
      {
        backend->setUniformInt(BloomShader::Uniform::IsThreshold, isThreshold);

        // Texture Unit 0
        inputTex->bind(0);
        backend->setUniformInt(BloomShader::Uniform::InputTex, 0);

        backend->setRenderTarget(outputFramebuffer);
        backend->draw(6);
      };

    oneBlurringPass(m_hdrFramebuffer->getColorTexture(), m_bloomFramebuffer[0].get(), true);

    auto srcFb = m_bloomFramebuffer[0].get();
    auto dstFb = m_bloomFramebuffer[1].get();

    for(int k = 0; k < 6; ++k)
    {
      oneBlurringPass(srcFb->getColorTexture(), dstFb);
      std::swap(srcFb, dstFb);
    }
  }

  void drawHdrBuffer()
  {
    backend->useGpuProgram(m_hdrShader.get());

    // Texture Unit 0
    m_hdrFramebuffer->getColorTexture()->bind(0);
    backend->setUniformInt(HdrShader::Uniform::InputTex1, 0);

    // Texture Unit 1
    m_bloomFramebuffer[0]->getColorTexture()->bind(1);
    backend->setUniformInt(HdrShader::Uniform::InputTex2, 1);

    backend->useVertexBuffer(m_quadVbo.get());

    backend->enableVertexAttribute(HdrShader::Attribute::positionLoc, 2, sizeof(QuadVertex), OFFSET(QuadVertex, x));
    backend->enableVertexAttribute(HdrShader::Attribute::uvLoc, 2, sizeof(QuadVertex), OFFSET(QuadVertex, u));

    backend->draw(6);
  }

  const Size2i m_resolution;

  IGraphicsBackend* const backend;

  std::unique_ptr<IGpuProgram> m_hdrShader;
  std::unique_ptr<IGpuProgram> m_bloomShader;

  std::unique_ptr<IVertexBuffer> m_quadVbo;
  std::unique_ptr<IFrameBuffer> m_hdrFramebuffer;
  std::unique_ptr<IFrameBuffer> m_bloomFramebuffer[2];
};

RenderPass::FrameBuffer PostProcessRenderPass::getInputFrameBuffer()
{
  return { postproc->m_hdrFramebuffer.get(), postproc->m_resolution };
}

void PostProcessRenderPass::execute(RenderPass::FrameBuffer dst)
{
  postproc->applyBloomFilter();
  postproc->backend->setRenderTarget(dst.fb);
  postproc->drawHdrBuffer();
}

void PostProcessRenderPass::setup(IGraphicsBackend* backend, Size2i resolution)
{
  postproc = std::make_unique<PostProcessing>(backend, resolution);
}

PostProcessRenderPass::PostProcessRenderPass() = default;
PostProcessRenderPass::~PostProcessRenderPass() = default;

