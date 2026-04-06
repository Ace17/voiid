// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// High-level renderer

#include <algorithm> // sort
#include <chrono>
#include <cstring>
#include <memory>
#include <vector>

#include "base/geom.h"
#include "base/matrix4.h"
#include "base/scene.h"
#include "base/span.h"
#include "base/util.h" // setExtension
#include "engine/graphics_backend.h"
#include "engine/renderer.h"
#include "engine/rendermesh.h"
#include "misc/stats.h"
#include "misc/time.h"

#include "picture.h"
#include "postprocess.h"
#include "renderer_quads.h"
#include "renderpass.h"
#include "weakcache.h"

std::unique_ptr<RenderPass> CreateSkyboxPass(IGraphicsBackend* backend, const Camera* camera);

namespace
{
Gauge ggRenderTime("Render time (ms)");

template<typename T>
T blend(T a, T b, float alpha)
{
  return a * (1 - alpha) + b * alpha;
}

const int COLS = 16;
const int ROWS = 16;

struct DrawCommand
{
  SingleRenderMesh* pMesh;
  Rect3f where;
  Quaternion orientation;
  Camera camera;
  bool blinking;
};

struct MeshRenderPass
{
  void execute(RenderPass::FrameBuffer dst)
  {
    backend->setRenderTarget(dst.fb);

    auto byMaterial = [] (const DrawCommand& a, const DrawCommand& b)
      {
        if(a.pMesh->transparency != b.pMesh->transparency)
          return a.pMesh->transparency < b.pMesh->transparency;

        return false;
      };

    std::sort(m_drawCommands.begin(), m_drawCommands.end(), byMaterial);

    for(auto& cmd : m_drawCommands)
      executeDrawCommand(cmd);

    backend->enableVertexAttribute(MeshShader::Attribute::positionLoc, 0, 0, 0);
    backend->enableVertexAttribute(MeshShader::Attribute::normalLoc, 0, 0, 0);
    backend->enableVertexAttribute(MeshShader::Attribute::binormalLoc, 0, 0, 0);
    backend->enableVertexAttribute(MeshShader::Attribute::tangentLoc, 0, 0, 0);
    backend->enableVertexAttribute(MeshShader::Attribute::uvDiffuseLoc, 0, 0, 0);
  }

  void executeDrawCommand(const DrawCommand& cmd)
  {
    auto& model = *cmd.pMesh;
    auto& where = cmd.where;

    backend->useGpuProgram(m_meshShader.get());

    // Must match the uniform block in mesh.frag and mesh.vert
    struct MyUniformBlock
    {
      Matrix4f M;
      Matrix4f MVP;
      Vec4f fragOffset;
      Vec4f cameraPos;
      Vec4f ambientLight;
      Vec4f lightPos[32];
      Vec4f lightColor[32];
      int lightCount;
    };

    // Binding #1: Diffuse
    model.diffuse->bind(1);

    // Binding #2: Normalmap
    model.normal->bind(2);

    auto const forward = cmd.camera.dir.rotate(Vec3f(1, 0, 0));
    auto const up = cmd.camera.dir.rotate(Vec3f(0, 0, 1));

    auto const target = cmd.camera.pos + forward;
    auto const view = ::lookAt(cmd.camera.pos, target, up);
    auto const pos = ::translate(where.pos);
    auto const scale = ::scale(where.size);
    auto const rotate = quaternionToMatrix(cmd.orientation);

    static const float fovy = (float)((60.0f / 180) * PI);
    static const float near_ = 0.1f;
    static const float far_ = 1000.0f;
    const auto perspective = ::perspective(fovy, m_aspectRatio, near_, far_);

    const auto MV = pos * rotate * scale;
    const auto MVP = perspective * view * MV;

    {
      MyUniformBlock ub {};
      ub.ambientLight = { m_ambientLight, m_ambientLight, m_ambientLight, 0 };
      ub.lightCount = m_lights.size();

      assert(m_lights.size() < 32);

      for(auto& light : m_lights)
      {
        const auto i = int(&light - m_lights.data());
        ub.lightPos[i] = { light.pos.x, light.pos.y, light.pos.z, 1 };
        ub.lightColor[i] = { light.color.x, light.color.y, light.color.z, 1 };
      }

      if(cmd.blinking)
      {
        if(GetSteadyClockMs() % 100 < 50)
        {
          ub.fragOffset = { 0.8, 0.4, 0.4, 0.0 };
        }
      }

      ub.M = transpose(MV);
      ub.MVP = transpose(MVP);
      ub.cameraPos = { cmd.camera.pos.x, cmd.camera.pos.y, cmd.camera.pos.z, 1 };

      backend->setUniformBlock(&ub, sizeof ub);
    }

    backend->useVertexBuffer(model.vb.get());

    backend->enableVertexAttribute(MeshShader::Attribute::positionLoc, 3, sizeof(SingleRenderMesh::Vertex), offsetof(SingleRenderMesh::Vertex, x));
    backend->enableVertexAttribute(MeshShader::Attribute::normalLoc, 3, sizeof(SingleRenderMesh::Vertex), offsetof(SingleRenderMesh::Vertex, nx));
    backend->enableVertexAttribute(MeshShader::Attribute::binormalLoc, 3, sizeof(SingleRenderMesh::Vertex), offsetof(SingleRenderMesh::Vertex, bx));
    backend->enableVertexAttribute(MeshShader::Attribute::tangentLoc, 3, sizeof(SingleRenderMesh::Vertex), offsetof(SingleRenderMesh::Vertex, tx));
    backend->enableVertexAttribute(MeshShader::Attribute::uvDiffuseLoc, 2, sizeof(SingleRenderMesh::Vertex), offsetof(SingleRenderMesh::Vertex, diffuse_u));

    backend->draw(model.vertices.size());
  }

  struct MeshShader
  {
    enum Attribute
    {
      positionLoc = 0,
      uvDiffuseLoc = 1,
      normalLoc = 2,
      binormalLoc = 3,
      tangentLoc = 4,
    };
  };

  struct Light
  {
    Vec3f pos;
    Vec3f color;
  };

  IGraphicsBackend* backend {};
  std::unique_ptr<IGpuProgram> m_meshShader;
  std::vector<DrawCommand> m_drawCommands;
  std::vector<Light> m_lights;
  float m_ambientLight = 0;
  float m_aspectRatio = 1.0;
};

struct Renderer : IRenderer, IScreenSizeListener
{
  Renderer(IGraphicsBackend* backend_) : backend(backend_), m_skyboxPass(CreateSkyboxPass(backend_, &m_camera)), m_quadsRenderPass(backend_)
  {
    m_textureCache.onCacheMiss = [this] (String path) { return loadTexture(path); };

    loadFontTexture("res/font.png", COLS, ROWS);

    backend->setScreenSizeListener(this);

    m_meshRenderPass.m_meshShader = backend->createGpuProgram("mesh", true);
    m_meshRenderPass.backend = backend;

    m_postprocRenderPass.setup(backend, m_screenSize);
  }

  void onScreenSizeChanged(Vec2i screenSize) override
  {
    m_screenSize = screenSize;
    m_postprocRenderPass.setup(backend, screenSize);

    printf("[renderer] Screen size changed to: %dx%d\n", screenSize.x, screenSize.y);
  }

  void setHdr(bool enable) override
  {
    m_enablePostProcessing = enable;
  }

  void setFsaa(bool enable) override
  {
    if(enable != m_enableFsaa)
    {
      Vec2i size = m_screenSize;

      if(enable)
        size = size * 2;

      m_postprocRenderPass.setup(backend, size);
    }

    m_enableFsaa = enable;
  }

  void loadModel(int modelId, String path) override
  {
    if((int)m_Models.size() <= modelId)
      m_Models.resize(modelId + 1);

    m_Models[modelId] = loadRenderMesh(path);

    int i = 0;

    for(auto& single : m_Models[modelId].singleMeshes)
    {
      single.diffuse = m_textureCache.fetch(setExtension(std::string(path.data), std::to_string(i) + ".diffuse.png"));
      single.normal = m_textureCache.fetch(setExtension(std::string(path.data), std::to_string(i) + ".normal.png"));

      ++i;
    }

    uploadVerticesToGPU(m_Models[modelId]);
  }

  void setCamera(Vec3f pos, Quaternion dir) override
  {
    auto cam = (Camera { pos, dir });

    if(!m_cameraValid)
    {
      m_camera = cam;
      m_cameraValid = true;
    }

    // avoid big camera jumps
    {
      auto delta = m_camera.pos - pos;

      if(dotProduct(delta, delta) > 10)
        m_camera = cam;
    }

    m_camera.pos = blend(m_camera.pos, cam.pos, 0.3);
    m_camera.dir = cam.dir;
  }

  void beginDraw() override
  {
    m_quadsRenderPass.m_quadsToDraw.clear();
    m_meshRenderPass.m_drawCommands.clear();
    m_meshRenderPass.m_lights.clear();
  }

  void endDraw() override
  {
    const auto t0 = std::chrono::high_resolution_clock::now();

    doRender();

    const auto t1 = std::chrono::high_resolution_clock::now();

    ggRenderTime = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count() / 1000.0;

    backend->swap();
  }

  void doRender()
  {
    const auto aspectRatio = float(m_screenSize.x) / m_screenSize.y;
    auto screen = RenderPass::FrameBuffer{ nullptr };
    auto meshRenderTarget = m_enablePostProcessing ? m_postprocRenderPass.getInputFrameBuffer() : screen;

    backend->setRenderTarget(meshRenderTarget.fb);
    backend->clear();

    m_skyboxPass->execute(meshRenderTarget);

    m_meshRenderPass.m_aspectRatio = aspectRatio;
    m_meshRenderPass.execute(meshRenderTarget);

    if(m_enablePostProcessing)
      m_postprocRenderPass.execute(screen);

    m_quadsRenderPass.m_aspectRatio = aspectRatio;
    m_quadsRenderPass.execute(screen);
  }

  void drawActor(Rect3f where, Quaternion orientation, int modelId, bool blinking) override
  {
    auto& model = m_Models.at(modelId);

    for(auto& single : model.singleMeshes)
      m_meshRenderPass.m_drawCommands.push_back({ &single, where, orientation, m_camera, blinking });
  }

  void drawText(Vec2f pos, String text) override
  {
    Rect2f rect;
    rect.size.x = 0.25;
    rect.size.y = 0.25;
    rect.pos.x = pos.x - text.len * rect.size.x / 2;
    rect.pos.y = pos.y;

    for(auto c : text)
    {
      const float col = (c % COLS);
      const float row = (c / COLS);

      const float u0 = (col + 0) / COLS;
      const float u1 = (col + 1) / COLS;

      const float v0 = 1.0f - (row + 1) / ROWS;
      const float v1 = 1.0f - (row + 0) / ROWS;

      m_quadsRenderPass.m_quadsToDraw.push_back({ m_fontTexture.get(), rect, { { u0, v0 }, { u1, v1 } } });

      rect.pos.x += rect.size.x;
    }
  }

  void setAmbientLight(float ambientLight) override
  {
    m_meshRenderPass.m_ambientLight = ambientLight;
  }

  void drawLight(Vec3f pos, Vec3f color) override
  {
    m_meshRenderPass.m_lights.push_back({ pos, color });
  }

private:
  Vec2i m_screenSize;
  Camera m_camera;
  bool m_cameraValid = false;
  IGraphicsBackend* const backend;
  std::vector<RenderMesh> m_Models;

  bool m_enableFsaa = false;
  bool m_enablePostProcessing = true;

  std::unique_ptr<RenderPass> m_skyboxPass;
  MeshRenderPass m_meshRenderPass;
  QuadsRenderPass m_quadsRenderPass;
  PostProcessRenderPass m_postprocRenderPass;

  WeakCache<std::string, ITexture> m_textureCache;
  std::shared_ptr<ITexture> m_fontTexture;

  void loadFontTexture(String path, int COLS, int ROWS)
  {
    m_fontTexture = backend->createTexture();
    m_fontTexture->upload(addBorderToTiles(loadPicture(path), COLS, ROWS));
    m_fontTexture->setNoRepeat();
  }

  void uploadVerticesToGPU(RenderMesh& mesh)
  {
    for(auto& model : mesh.singleMeshes)
    {
      model.vb = backend->createVertexBuffer();
      model.vb->upload((uint8_t*)model.vertices.data(), sizeof(model.vertices[0]) * model.vertices.size());
    }
  }

  std::unique_ptr<ITexture> loadTexture(String path)
  {
    auto pic = loadPicture(path);
    auto texture = backend->createTexture();
    texture->upload(pic);
    return texture;
  }
};
}

IRenderer* createRenderer(IGraphicsBackend* backend)
{
  return new Renderer(backend);
}

