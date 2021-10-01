// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// High-level renderer

#include <chrono>
#include <cstring>
#include <memory>
#include <vector>

#include "base/geom.h"
#include "base/scene.h"
#include "base/span.h"
#include "base/util.h" // setExtension
#include "engine/display.h"
#include "engine/graphics_backend.h"
#include "engine/rendermesh.h"
#include "engine/stats.h"
#include "misc/time.h"

#include "matrix4.h"
#include "picture.h"
#include "postprocess.h"
#include "renderpass.h"

using namespace std;

namespace
{
template<typename T>
T blend(T a, T b, float alpha)
{
  return a * (1 - alpha) + b * alpha;
}

struct Camera
{
  Vector3f pos;
  Quaternion dir;
};

struct DrawCommand
{
  SingleRenderMesh* pMesh;
  Rect3f where;
  Quaternion orientation;
  Camera camera;
  bool blinking;
  bool depthtest;
};

struct MeshRenderPass : RenderPass
{
  FrameBuffer getInputFrameBuffer() override { return {}; }

  void execute(FrameBuffer dst) override
  {
    backend->setRenderTarget(dst.fb);

    backend->clear();

    for(auto& cmd : m_drawCommands)
      executeDrawCommand(cmd);
  }

  void executeDrawCommand(const DrawCommand& cmd)
  {
    auto& model = *cmd.pMesh;
    auto& where = cmd.where;

    if(cmd.depthtest)
    {
      backend->useGpuProgram(m_meshShader.get());

      backend->setUniformFloat3(MeshShader::Uniform::ambientLoc, m_ambientLight, m_ambientLight, m_ambientLight);
      backend->setUniformFloat4(MeshShader::Uniform::colorId, 0, 0, 0, 0);

      assert(m_lights.size() < 32);
      backend->setUniformInt(MeshShader::Uniform::LightCountLoc, (int)m_lights.size());

      for(auto& light : m_lights)
      {
        const auto i = int(&light - m_lights.data());
        backend->setUniformFloat3(MeshShader::Uniform::LightPosLoc + i, light.pos.x, light.pos.y, light.pos.z);
        backend->setUniformFloat3(MeshShader::Uniform::LightColorLoc + i, light.color.x, light.color.y, light.color.z);
      }

      if(cmd.blinking)
      {
        if(GetSteadyClockMs() % 100 < 50)
          backend->setUniformFloat4(MeshShader::Uniform::colorId, 0.8, 0.4, 0.4, 0);
      }

      // Texture Unit 0: Diffuse
      model.diffuse->bind(0);
      backend->setUniformInt(MeshShader::Uniform::DiffuseTex, 0);

      // Texture Unit 1: Lightmap
      model.diffuse->bind(1);
      backend->setUniformInt(MeshShader::Uniform::LightmapTex, 1);

      auto const forward = cmd.camera.dir.rotate(Vector3f(1, 0, 0));
      auto const up = cmd.camera.dir.rotate(Vector3f(0, 0, 1));

      auto const target = cmd.camera.pos + forward;
      auto const view = ::lookAt(cmd.camera.pos, target, up);
      auto const pos = ::translate(where.pos);
      auto const scale = ::scale(Vector3f(where.size.cx, where.size.cy, where.size.cz));
      auto const rotate = quaternionToMatrix(cmd.orientation);

      static const float fovy = (float)((60.0f / 180) * PI);
      static const float near_ = 0.1f;
      static const float far_ = 1000.0f;
      const auto perspective = ::perspective(fovy, m_aspectRatio, near_, far_);

      auto MV = pos * rotate * scale;
      auto MVP = perspective * view * MV;

      backend->setUniformMatrixFloat4(MeshShader::Uniform::M, &MV[0][0]);
      backend->setUniformMatrixFloat4(MeshShader::Uniform::MVP, &MVP[0][0]);
      backend->setUniformFloat3(MeshShader::Uniform::CameraPos, cmd.camera.pos.x, cmd.camera.pos.y, cmd.camera.pos.z);

      backend->useVertexBuffer(model.vb);

      backend->enableVertexAttribute(MeshShader::Attribute::positionLoc, 3, sizeof(SingleRenderMesh::Vertex), OFFSET(SingleRenderMesh::Vertex, x));
      backend->enableVertexAttribute(MeshShader::Attribute::normalLoc, 3, sizeof(SingleRenderMesh::Vertex), OFFSET(SingleRenderMesh::Vertex, nx));
      backend->enableVertexAttribute(MeshShader::Attribute::uvDiffuseLoc, 2, sizeof(SingleRenderMesh::Vertex), OFFSET(SingleRenderMesh::Vertex, diffuse_u));
      backend->enableVertexAttribute(MeshShader::Attribute::uvLightmapLoc, 2, sizeof(SingleRenderMesh::Vertex), OFFSET(SingleRenderMesh::Vertex, lightmap_u));

      backend->draw(model.vertices.size());
    }
    else
    {
      backend->useGpuProgram(m_textShader.get());

      // Texture Unit 0: Diffuse
      model.diffuse->bind(0);
      backend->setUniformInt(TextShader::Uniform::DiffuseTex, 0);

      auto const forward = Vector3f(0, 1, 0);
      auto const up = Vector3f(0, 0, 1);

      auto const target = cmd.camera.pos + forward;
      auto const view = ::lookAt(cmd.camera.pos, target, up);
      auto const pos = ::translate(where.pos);
      auto const scale = ::scale(Vector3f(where.size.cx, where.size.cy, where.size.cz));

      static const float fovy = (float)((60.0f / 180) * PI);
      static const float near_ = 0.1f;
      static const float far_ = 100.0f;
      const auto perspective = ::perspective(fovy, m_aspectRatio, near_, far_);

      auto MV = pos * scale;
      auto MVP = perspective * view * MV;

      backend->setUniformMatrixFloat4(TextShader::Uniform::MVP, &MVP[0][0]);

      backend->useVertexBuffer(model.vb);

      backend->enableVertexAttribute(TextShader::Attribute::positionLoc, 3, sizeof(SingleRenderMesh::Vertex), OFFSET(SingleRenderMesh::Vertex, x));
      backend->enableVertexAttribute(TextShader::Attribute::uvDiffuseLoc, 2, sizeof(SingleRenderMesh::Vertex), OFFSET(SingleRenderMesh::Vertex, diffuse_u));

      backend->draw(model.vertices.size());
    }
  }

  struct TextShader
  {
    enum Uniform
    {
      MVP = 0,
      DiffuseTex = 1,
    };

    enum Attribute
    {
      positionLoc = 0,
      uvDiffuseLoc = 1,
    };
  };

  struct MeshShader
  {
    enum Uniform
    {
      M = 0,
      MVP = 1,
      CameraPos = 2,
      DiffuseTex = 4,
      LightmapTex = 5,
      colorId = 3,
      ambientLoc = 6,
      LightCountLoc = 7,
      LightPosLoc = 8,
      LightColorLoc = 40,
    };

    enum Attribute
    {
      positionLoc = 0,
      uvDiffuseLoc = 1,
      uvLightmapLoc = 2,
      normalLoc = 3,
    };
  };

  struct Light
  {
    Vector3f pos;
    Vector3f color;
  };

  IGraphicsBackend* backend {};
  std::unique_ptr<IGpuProgram> m_textShader;
  std::unique_ptr<IGpuProgram> m_meshShader;
  std::vector<DrawCommand> m_drawCommands;
  vector<Light> m_lights;
  float m_ambientLight = 0;
  float m_aspectRatio = 1.0;
};

struct ScreenRenderPass : RenderPass
{
  Size2i screenSize {};

  FrameBuffer getInputFrameBuffer() override { return { nullptr, screenSize }; }
  void execute(FrameBuffer) override { /* nothing to do */ }
};

struct Renderer : Display, IScreenSizeListener
{
  Renderer(IGraphicsBackend* backend_) : backend(backend_)
  {
    m_fontModel = loadFontModels("res/font.png", 16, 16);

    backend->setScreenSizeListener(this);

    m_meshRenderPass.m_textShader = backend->createGpuProgram("text", false);
    m_meshRenderPass.m_meshShader = backend->createGpuProgram("mesh", true);
    m_meshRenderPass.backend = backend;

    m_postprocRenderPass.setup(backend, m_screenSize);
  }

  void onScreenSizeChanged(Size2i screenSize) override
  {
    m_screenSize = screenSize;
    m_postprocRenderPass.setup(backend, screenSize);

    printf("[renderer] Screen size changed to: %dx%d\n", screenSize.width, screenSize.height);
  }

  void setHdr(bool enable) override
  {
    m_enablePostProcessing = enable;
  }

  void setFsaa(bool enable) override
  {
    if(enable != m_enableFsaa)
    {
      Size2i size = m_screenSize;

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
      auto diffuse = loadTexture(setExtension(string(path.data), to_string(i) + ".diffuse.png"));
      auto lightmap = loadTexture(setExtension(string(path.data), to_string(i) + ".lightmap.png"));
      single.diffuse = diffuse.get();
      single.lightmap = lightmap.get();

      m_textures.push_back(std::move(diffuse));
      m_textures.push_back(std::move(lightmap));

      ++i;
    }

    uploadVerticesToGPU(m_Models[modelId]);
  }

  void setCamera(Vector3f pos, Quaternion dir) override
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
    m_meshRenderPass.m_drawCommands.clear();
  }

  void endDraw() override
  {
    m_meshRenderPass.m_aspectRatio = float(m_screenSize.width) / m_screenSize.height;
    m_screenRenderPass.screenSize = m_screenSize;

    RenderPass* passes[16];
    int count = 0;

    passes[count++] = &m_meshRenderPass;

    if(m_enablePostProcessing)
      passes[count++] = &m_postprocRenderPass;

    passes[count++] = &m_screenRenderPass;

    renderPasses(Span<RenderPass*>(passes, count));
  }

  void drawActor(Rect3f where, Quaternion orientation, int modelId, bool blinking, int actionIdx, float ratio) override
  {
    (void)actionIdx;
    (void)ratio;
    auto& model = m_Models.at(modelId);
    pushMesh(where, orientation, m_camera, model, blinking, true);
  }

  void drawText(Vector2f pos, char const* text) override
  {
    Rect3f rect;
    rect.size.cx = 0.5;
    rect.size.cy = 0;
    rect.size.cz = 0.5;
    rect.pos.x = pos.x - strlen(text) * rect.size.cx / 2;
    rect.pos.y = 0;
    rect.pos.z = pos.y;

    auto cam = (Camera { Vector3f(0, -10, 0), Quaternion::fromEuler(PI / 2, 0, 0) });
    auto orientation = Quaternion::fromEuler(0, 0, 0);

    while(*text)
    {
      pushMesh(rect, orientation, cam, m_fontModel[*text], false, false);
      rect.pos.x += rect.size.cx;
      ++text;
    }
  }

  void setAmbientLight(float ambientLight) override
  {
    m_meshRenderPass.m_ambientLight = ambientLight;
  }

  void setLight(int idx, Vector3f pos, Vector3f color) override
  {
    if(idx >= (int)m_meshRenderPass.m_lights.size())
      m_meshRenderPass.m_lights.resize(idx + 1);

    m_meshRenderPass.m_lights[idx] = { pos, color };
  }

private:
  Size2i m_screenSize;
  Camera m_camera;
  bool m_cameraValid = false;
  IGraphicsBackend* const backend;
  vector<RenderMesh> m_Models;
  vector<RenderMesh> m_fontModel;

  bool m_enableFsaa = false;
  bool m_enablePostProcessing = true;

  MeshRenderPass m_meshRenderPass;
  PostProcessRenderPass m_postprocRenderPass;
  ScreenRenderPass m_screenRenderPass;

  std::vector<std::unique_ptr<ITexture>> m_textures;
  std::vector<std::unique_ptr<IVertexBuffer>> m_vbs;

  std::vector<RenderMesh> loadFontModels(String path, int COLS, int ROWS)
  {
    std::vector<RenderMesh> r;

    auto diffuse = backend->createTexture();
    diffuse->upload(addBorderToTiles(loadPicture(path), COLS, ROWS));
    auto lightmap = loadTexture("res/white.png");

    // don't repeat fonts
    diffuse->setNoRepeat();
    lightmap->setNoRepeat();

    for(int i = 0; i < COLS * ROWS; ++i)
    {
      const float col = (i % COLS);
      const float row = (i / COLS);

      const float u0 = (col + 0) / COLS;
      const float u1 = (col + 1) / COLS;

      const float v0 = 1.0f - (row + 1) / ROWS;
      const float v1 = 1.0f - (row + 0) / ROWS;

      const SingleRenderMesh::Vertex vertices[] =
      {
        { 0, 0, 0, /* N */ 0, 1, 0, /* uv diffuse */ u0, v0, /* uv lightmap */ u0, v0, },
        { 1, 0, 1, /* N */ 0, 1, 0, /* uv diffuse */ u1, v1, /* uv lightmap */ u1, v1, },
        { 0, 0, 1, /* N */ 0, 1, 0, /* uv diffuse */ u0, v1, /* uv lightmap */ u0, v1, },

        { 0, 0, 0, /* N */ 0, 1, 0, /* uv diffuse */ u0, v0, /* uv lightmap */ u0, v0, },
        { 1, 0, 0, /* N */ 0, 1, 0, /* uv diffuse */ u1, v0, /* uv lightmap */ u0, v1, },
        { 1, 0, 1, /* N */ 0, 1, 0, /* uv diffuse */ u1, v1, /* uv lightmap */ u1, v1, },
      };

      SingleRenderMesh sm;
      sm.diffuse = diffuse.get();
      sm.lightmap = lightmap.get();

      for(auto& v : vertices)
        sm.vertices.push_back(v);

      RenderMesh glyph {};
      glyph.singleMeshes.push_back(sm);
      uploadVerticesToGPU(glyph);
      r.push_back(glyph);
    }

    m_textures.push_back(std::move(diffuse));
    m_textures.push_back(std::move(lightmap));

    return r;
  }

  void uploadVerticesToGPU(RenderMesh& mesh)
  {
    for(auto& model : mesh.singleMeshes)
    {
      auto vb = backend->createVertexBuffer();
      vb->upload((uint8_t*)model.vertices.data(), sizeof(model.vertices[0]) * model.vertices.size());
      model.vb = vb.get();
      m_vbs.push_back(std::move(vb));
    }
  }

  std::unique_ptr<ITexture> loadTexture(String path)
  {
    auto pic = loadPicture(path);
    auto texture = backend->createTexture();
    texture->upload(pic);
    return texture;
  }

  void pushMesh(Rect3f where, Quaternion orientation, Camera const& camera, RenderMesh& model, bool blinking, bool depthtest)
  {
    for(auto& single : model.singleMeshes)
      m_meshRenderPass.m_drawCommands.push_back({ &single, where, orientation, camera, blinking, depthtest });
  }

  void renderPasses(Span<RenderPass*> passes)
  {
    const auto t0 = chrono::high_resolution_clock::now();

    for(int i = 0; i + 1 < passes.len; ++i)
    {
      auto framebuffer = passes[i + 1]->getInputFrameBuffer();
      passes[i]->execute(framebuffer);
    }

    const auto t1 = chrono::high_resolution_clock::now();

    Stat("Render time (ms)", chrono::duration_cast<chrono::microseconds>(t1 - t0).count() / 1000.0);

    backend->swap();
  }
};
}

Display* createRenderer(IGraphicsBackend* backend)
{
  return new Renderer(backend);
}

