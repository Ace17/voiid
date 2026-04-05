#pragma once

#include "renderpass.h"

struct QuadToDraw
{
  SingleRenderMesh* pMesh;
  Rect3f where;
};

struct QuadsRenderPass : RenderPass
{
  void execute(FrameBuffer dst) override
  {
    backend->setRenderTarget(dst.fb);

    for(auto& cmd : m_drawCommands)
      executeDrawCommand(cmd);
  }

  void executeDrawCommand(const QuadToDraw& cmd)
  {
    auto& model = *cmd.pMesh;
    auto& where = cmd.where;

    backend->useGpuProgram(m_textShader.get());

    // Texture Unit 0: Diffuse
    model.diffuse->bind(1);

    auto const forward = Vec3f(0, 1, 0);
    auto const up = Vec3f(0, 0, 1);

    Vec3f camPos(0, -10, 0);

    auto const target = camPos + forward;
    auto const view = ::lookAt(camPos, target, up);
    auto const pos = ::translate(where.pos);
    auto const scale = ::scale(where.size);

    static const float fovy = (float)((60.0f / 180) * PI);
    static const float near_ = 0.1f;
    static const float far_ = 100.0f;
    const auto perspective = ::perspective(fovy, m_aspectRatio, near_, far_);

    auto MV = pos * scale;

    // Must match the uniform block in text.vert and text.frag
    struct MyUniformBlock
    {
      Matrix4f MVP;
    };

    MyUniformBlock ub {};
    ub.MVP = perspective * view * MV;
    ub.MVP = transpose(ub.MVP);

    backend->setUniformBlock(&ub, sizeof ub);

    backend->useVertexBuffer(model.vb.get());

    backend->enableVertexAttribute(TextShader::Attribute::positionLoc, 3, sizeof(SingleRenderMesh::Vertex), OFFSET(SingleRenderMesh::Vertex, x));
    backend->enableVertexAttribute(TextShader::Attribute::uvDiffuseLoc, 2, sizeof(SingleRenderMesh::Vertex), OFFSET(SingleRenderMesh::Vertex, diffuse_u));

    backend->draw(model.vertices.size());
  }

  struct TextShader
  {
    enum Attribute
    {
      positionLoc = 0,
      uvDiffuseLoc = 1,
    };
  };

  IGraphicsBackend* backend {};
  std::unique_ptr<IGpuProgram> m_textShader;
  std::vector<QuadToDraw> m_drawCommands;
  float m_aspectRatio = 1.0;
};

