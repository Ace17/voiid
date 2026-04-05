#pragma once

#include "renderpass.h"

struct QuadToDraw
{
  ITexture* tex;
  Rect2f where;
  Vec2f uv[2];
};

struct QuadsRenderPass : RenderPass
{
  QuadsRenderPass(IGraphicsBackend * backend_) : backend(backend_)
    , m_textShader(backend->createGpuProgram("text", false))
    , m_vb(backend->createVertexBuffer(true))
  {
  }

  void execute(FrameBuffer dst) override
  {
    backend->setRenderTarget(dst.fb);

    auto byLexico = [](const QuadToDraw& a, const QuadToDraw& b)
      {
        if(a.tex != b.tex)
          return a.tex < b.tex;

        return false;
      };

    backend->useGpuProgram(m_textShader.get());

    std::sort(m_quadsToDraw.begin(), m_quadsToDraw.end(), byLexico);

    ITexture* currTexture = nullptr;

    std::vector<SingleRenderMesh::Vertex> pendingVertices;

    auto flushQuads = [&]()
      {
        if(pendingVertices.empty())
          return;

        m_vb->upload(pendingVertices.data(), pendingVertices.size() * sizeof(pendingVertices[0]));
        backend->useVertexBuffer(m_vb.get());

        backend->enableVertexAttribute(TextShader::Attribute::positionLoc, 2, sizeof(SingleRenderMesh::Vertex), OFFSET(SingleRenderMesh::Vertex, x));
        backend->enableVertexAttribute(TextShader::Attribute::uvDiffuseLoc, 2, sizeof(SingleRenderMesh::Vertex), OFFSET(SingleRenderMesh::Vertex, diffuse_u));

        backend->draw(pendingVertices.size());

        pendingVertices.clear();
      };

    for(auto& cmd : m_quadsToDraw)
    {
      if(cmd.tex != currTexture)
      {
        flushQuads();
        cmd.tex->bind(1);
        currTexture = cmd.tex;
      }

      const Vec2f size = cmd.where.size * 0.1;
      const Vec2f pos = cmd.where.pos * 0.1;

      const auto x0 = pos.x;
      const auto x1 = pos.x + size.x;
      const auto y0 = pos.y;
      const auto y1 = pos.y + size.y * m_aspectRatio;

      const auto u0 = cmd.uv[0].x;
      const auto u1 = cmd.uv[1].x;
      const auto v0 = cmd.uv[0].y;
      const auto v1 = cmd.uv[1].y;

      pendingVertices.push_back({ x0, y0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*UV*/ u0, v0 });
      pendingVertices.push_back({ x1, y0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*UV*/ u1, v0 });
      pendingVertices.push_back({ x1, y1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*UV*/ u1, v1 });

      pendingVertices.push_back({ x0, y0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*UV*/ u0, v0 });
      pendingVertices.push_back({ x1, y1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*UV*/ u1, v1 });
      pendingVertices.push_back({ x0, y1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*UV*/ u0, v1 });
    }

    flushQuads();
  }

  struct TextShader
  {
    enum Attribute
    {
      positionLoc = 0,
      uvDiffuseLoc = 1,
    };
  };

  IGraphicsBackend* const backend;
  std::unique_ptr<IGpuProgram> m_textShader;
  std::unique_ptr<IVertexBuffer> m_vb;
  std::vector<QuadToDraw> m_quadsToDraw;
  float m_aspectRatio = 1.0;
};

