#pragma once

#include "renderpass.h"

struct QuadToDraw
{
  ITexture* tex;
  IVertexBuffer* vb;
  Rect2f where;
  int vertexCount;
};

struct QuadsRenderPass : RenderPass
{
  QuadsRenderPass(IGraphicsBackend * backend_) : backend(backend_)
    , m_textShader(backend->createGpuProgram("text", false))
  {
  }

  void execute(FrameBuffer dst) override
  {
    backend->setRenderTarget(dst.fb);

    for(auto& cmd : m_quadsToDraw)
      executeDrawCommand(cmd);
  }

  void executeDrawCommand(const QuadToDraw& cmd)
  {
    const Vec2f size = cmd.where.size;
    const Vec2f pos = cmd.where.pos;

    backend->useGpuProgram(m_textShader.get());

    // Texture Unit 0: Diffuse
    cmd.tex->bind(1);

    auto const forward = Vec3f(0, 0, -1);
    auto const up = Vec3f(0, 1, 0);

    Vec3f camPos(0, 0, 10);

    auto const target = camPos + forward;
    auto const view = ::lookAt(camPos, target, up);
    auto const trans = ::translate(Vec3f(pos.x, pos.y, 0));
    auto const scale = ::scale(Vec3f(size.x, size.y, 1));

    static const float fovy = (float)((60.0f / 180) * PI);
    static const float near_ = 0;
    static const float far_ = 1;
    const auto perspective = ::perspective(fovy, m_aspectRatio, near_, far_);

    auto MV = trans * scale;

    // Must match the uniform block in text.vert and text.frag
    struct MyUniformBlock
    {
      Matrix4f MVP;
    };

    MyUniformBlock ub {};
    ub.MVP = perspective * view * MV;
    ub.MVP = transpose(ub.MVP);

    backend->setUniformBlock(&ub, sizeof ub);

    backend->useVertexBuffer(cmd.vb);

    backend->enableVertexAttribute(TextShader::Attribute::positionLoc, 2, sizeof(SingleRenderMesh::Vertex), OFFSET(SingleRenderMesh::Vertex, x));
    backend->enableVertexAttribute(TextShader::Attribute::uvDiffuseLoc, 2, sizeof(SingleRenderMesh::Vertex), OFFSET(SingleRenderMesh::Vertex, diffuse_u));

    backend->draw(cmd.vertexCount);
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

