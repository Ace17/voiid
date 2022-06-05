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
#include "base/matrix4.h"
#include "base/scene.h"
#include "base/span.h"
#include "base/util.h" // setExtension
#include "engine/graphics_backend.h"
#include "engine/renderer.h"
#include "engine/rendermesh.h"
#include "engine/stats.h"
#include "misc/time.h"

#include "picture.h"
#include "postprocess.h"
#include "renderpass.h"
#include "weakcache.h"

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
  Vec3f pos;
  Quaternion dir;
};

struct DrawCommand
{
  SingleRenderMesh* pMesh;
  Rect3f where;
  Quaternion orientation;
  Camera camera;
  bool blinking;
};

struct CubeVertex
{
  float x, y, z;
};

const CubeVertex UnitCube[] =
{
  { -1, -1, -1, },
  { -1, -1, 1, },
  { -1, 1, 1, },
  { 1, 1, -1, },
  { -1, -1, -1, },
  { -1, 1, -1, },
  { 1, -1, 1, },
  { -1, -1, -1, },
  { 1, -1, -1, },
  { 1, 1, -1, },
  { 1, -1, -1, },
  { -1, -1, -1, },
  { -1, -1, -1, },
  { -1, 1, 1, },
  { -1, 1, -1, },
  { 1, -1, 1, },
  { -1, -1, 1, },
  { -1, -1, -1, },
  { -1, 1, 1, },
  { -1, -1, 1, },
  { 1, -1, 1, },
  { 1, 1, 1, },
  { 1, -1, -1, },
  { 1, 1, -1, },
  { 1, -1, -1, },
  { 1, 1, 1, },
  { 1, -1, 1, },
  { 1, 1, 1, },
  { 1, 1, -1, },
  { -1, 1, -1, },
  { 1, 1, 1, },
  { -1, 1, -1, },
  { -1, 1, 1, },
  { 1, 1, 1, },
  { -1, 1, 1, },
  { 1, -1, 1 },
};

struct SkyboxPass : RenderPass
{
  SkyboxPass(IGraphicsBackend* backend) : backend(backend)
  {
    m_shader = backend->createGpuProgram("skybox", false);
    m_vb = backend->createVertexBuffer();
    m_vb->upload(UnitCube, sizeof UnitCube);
  }

  void execute(FrameBuffer dst) override
  {
    backend->setRenderTarget(dst.fb);

    backend->useGpuProgram(m_shader.get());

    auto const forward = camera.dir.rotate(Vec3f(1, 0, 0));
    auto const up = camera.dir.rotate(Vec3f(0, 0, 1));

    auto const target = forward;
    auto const view = ::lookAt(Vec3f(0, 0, 0), target, up);
    auto const pos = ::translate({});

    static const float fovy = (float)((60.0f / 180) * PI);
    static const float near_ = 0.1f;
    static const float far_ = 1000.0f;
    const auto perspective = ::perspective(fovy, 16.0 / 9.0, near_, far_);

    auto M = pos;
    auto MVP = perspective * view * M;

    backend->setUniformMatrixFloat4(0, &M[0][0]);
    backend->setUniformMatrixFloat4(1, &MVP[0][0]);

    backend->useVertexBuffer(m_vb.get());
    backend->enableVertexAttribute(0, 3, sizeof(CubeVertex), OFFSET(CubeVertex, x));

    backend->draw(std::size(UnitCube));
  }

  IGraphicsBackend* const backend;
  std::unique_ptr<IGpuProgram> m_shader;
  std::unique_ptr<IVertexBuffer> m_vb;
  Camera camera;
};

struct MeshRenderPass
{
  void execute(RenderPass::FrameBuffer dst)
  {
    backend->setRenderTarget(dst.fb);

    for(auto& cmd : m_drawCommands)
      executeDrawCommand(cmd);
  }

  void executeDrawCommand(const DrawCommand& cmd)
  {
    auto& model = *cmd.pMesh;
    auto& where = cmd.where;

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
    model.lightmap->bind(1);
    backend->setUniformInt(MeshShader::Uniform::LightmapTex, 1);

    // Texture Unit 2: Normalmap
    model.normal->bind(2);
    backend->setUniformInt(MeshShader::Uniform::NormalTex, 2);

    auto const forward = cmd.camera.dir.rotate(Vec3f(1, 0, 0));
    auto const up = cmd.camera.dir.rotate(Vec3f(0, 0, 1));

    auto const target = cmd.camera.pos + forward;
    auto const view = ::lookAt(cmd.camera.pos, target, up);
    auto const pos = ::translate(where.pos);
    auto const scale = ::scale(Vec3f(where.size.cx, where.size.cy, where.size.cz));
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
    backend->enableVertexAttribute(MeshShader::Attribute::binormalLoc, 3, sizeof(SingleRenderMesh::Vertex), OFFSET(SingleRenderMesh::Vertex, bx));
    backend->enableVertexAttribute(MeshShader::Attribute::tangentLoc, 3, sizeof(SingleRenderMesh::Vertex), OFFSET(SingleRenderMesh::Vertex, tx));
    backend->enableVertexAttribute(MeshShader::Attribute::uvDiffuseLoc, 2, sizeof(SingleRenderMesh::Vertex), OFFSET(SingleRenderMesh::Vertex, diffuse_u));
    backend->enableVertexAttribute(MeshShader::Attribute::uvLightmapLoc, 2, sizeof(SingleRenderMesh::Vertex), OFFSET(SingleRenderMesh::Vertex, lightmap_u));

    backend->draw(model.vertices.size());
  }

  struct MeshShader
  {
    enum Uniform
    {
      M = 0,
      MVP = 1,
      CameraPos = 2,
      colorId = 3,
      DiffuseTex = 4,
      LightmapTex = 5,
      NormalTex = 6,
      ambientLoc = 7,
      LightCountLoc = 8,
      LightPosLoc = 9,
      LightColorLoc = 41,
    };

    enum Attribute
    {
      positionLoc = 0,
      uvDiffuseLoc = 1,
      uvLightmapLoc = 2,
      normalLoc = 3,
      binormalLoc = 4,
      tangentLoc = 5,
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
  vector<Light> m_lights;
  float m_ambientLight = 0;
  float m_aspectRatio = 1.0;
};

struct UiRenderPass : RenderPass
{
  void execute(FrameBuffer dst) override
  {
    backend->setRenderTarget(dst.fb);

    for(auto& cmd : m_drawCommands)
      executeDrawCommand(cmd);
  }

  void executeDrawCommand(const DrawCommand& cmd)
  {
    auto& model = *cmd.pMesh;
    auto& where = cmd.where;

    backend->useGpuProgram(m_textShader.get());

    // Texture Unit 0: Diffuse
    model.diffuse->bind(0);
    backend->setUniformInt(TextShader::Uniform::DiffuseTex, 0);

    auto const forward = Vec3f(0, 1, 0);
    auto const up = Vec3f(0, 0, 1);

    auto const target = cmd.camera.pos + forward;
    auto const view = ::lookAt(cmd.camera.pos, target, up);
    auto const pos = ::translate(where.pos);
    auto const scale = ::scale(Vec3f(where.size.cx, where.size.cy, where.size.cz));

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

  IGraphicsBackend* backend {};
  std::unique_ptr<IGpuProgram> m_textShader;
  std::vector<DrawCommand> m_drawCommands;
  float m_aspectRatio = 1.0;
};

struct Renderer : IRenderer, IScreenSizeListener
{
  Renderer(IGraphicsBackend* backend_) : backend(backend_), m_skyboxPass(backend_)
  {
    m_textureCache.onCacheMiss = [this] (String path) { return loadTexture(path); };

    m_fontModel = loadFontModels("res/font.png", 16, 16);

    backend->setScreenSizeListener(this);

    m_meshRenderPass.m_meshShader = backend->createGpuProgram("mesh", true);
    m_meshRenderPass.backend = backend;

    m_uiRenderPass.m_textShader = backend->createGpuProgram("text", false);
    m_uiRenderPass.backend = backend;

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
      single.diffuse = m_textureCache.fetch(setExtension(string(path.data), to_string(i) + ".diffuse.png"));
      single.lightmap = m_textureCache.fetch(setExtension(string(path.data), to_string(i) + ".lightmap.png"));
      single.normal = m_textureCache.fetch(setExtension(string(path.data), to_string(i) + ".normal.png"));

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
    m_uiRenderPass.m_drawCommands.clear();
    m_meshRenderPass.m_drawCommands.clear();
    m_meshRenderPass.m_lights.clear();
  }

  void endDraw() override
  {
    const auto t0 = chrono::high_resolution_clock::now();

    doRender();

    const auto t1 = chrono::high_resolution_clock::now();

    Stat("Render time (ms)", chrono::duration_cast<chrono::microseconds>(t1 - t0).count() / 1000.0);

    backend->swap();
  }

  void doRender()
  {
    const auto aspectRatio = float(m_screenSize.width) / m_screenSize.height;
    auto screen = RenderPass::FrameBuffer{ nullptr, m_screenSize };
    auto meshRenderTarget = m_enablePostProcessing ? m_postprocRenderPass.getInputFrameBuffer() : screen;

    backend->setRenderTarget(meshRenderTarget.fb);
    backend->clear();

    m_skyboxPass.camera = m_camera;
    m_skyboxPass.execute(meshRenderTarget);

    m_meshRenderPass.m_aspectRatio = aspectRatio;
    m_meshRenderPass.execute(meshRenderTarget);

    if(m_enablePostProcessing)
      m_postprocRenderPass.execute(screen);

    m_uiRenderPass.m_aspectRatio = aspectRatio;
    m_uiRenderPass.execute(screen);
  }

  void drawActor(Rect3f where, Quaternion orientation, int modelId, bool blinking) override
  {
    auto& model = m_Models.at(modelId);

    for(auto& single : model.singleMeshes)
      m_meshRenderPass.m_drawCommands.push_back({& single, where, orientation, m_camera, blinking });
  }

  void drawText(Vec2f pos, String text) override
  {
    Rect3f rect;
    rect.size.cx = 0.25;
    rect.size.cy = 0;
    rect.size.cz = 0.25;
    rect.pos.x = pos.x - text.len * rect.size.cx / 2;
    rect.pos.y = 0;
    rect.pos.z = pos.y;

    auto cam = (Camera { Vec3f(0, -10, 0), Quaternion::fromEuler(PI / 2, 0, 0) });
    auto orientation = Quaternion::identity();

    for(auto c : text)
    {
      for(auto& single : m_fontModel[c].singleMeshes)
        m_uiRenderPass.m_drawCommands.push_back({& single, rect, orientation, cam, false });

      rect.pos.x += rect.size.cx;
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
  Size2i m_screenSize;
  Camera m_camera;
  bool m_cameraValid = false;
  IGraphicsBackend* const backend;
  vector<RenderMesh> m_Models;
  vector<RenderMesh> m_fontModel;

  bool m_enableFsaa = false;
  bool m_enablePostProcessing = true;

  SkyboxPass m_skyboxPass;
  MeshRenderPass m_meshRenderPass;
  UiRenderPass m_uiRenderPass;
  PostProcessRenderPass m_postprocRenderPass;

  WeakCache<std::string, ITexture> m_textureCache;
  std::vector<std::unique_ptr<IVertexBuffer>> m_vbs;
  std::shared_ptr<ITexture> m_fontTexture;

  std::vector<RenderMesh> loadFontModels(String path, int COLS, int ROWS)
  {
    std::vector<RenderMesh> r;

    m_fontTexture = backend->createTexture();
    m_fontTexture->upload(addBorderToTiles(loadPicture(path), COLS, ROWS));
    auto lightmap = m_textureCache.fetch("res/white.png");

    // don't repeat fonts
    m_fontTexture->setNoRepeat();
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
        { 0, 0, 0, /* N */ 0, 1, 0, /* BN */ 1, 0, 0, /* T */ 0, 0, 1, /* uv diffuse */ u0, v0, /* uv lightmap */ u0, v0, },
        { 1, 0, 1, /* N */ 0, 1, 0, /* BN */ 1, 0, 0, /* T */ 0, 0, 1, /* uv diffuse */ u1, v1, /* uv lightmap */ u1, v1, },
        { 0, 0, 1, /* N */ 0, 1, 0, /* BN */ 1, 0, 0, /* T */ 0, 0, 1, /* uv diffuse */ u0, v1, /* uv lightmap */ u0, v1, },

        { 0, 0, 0, /* N */ 0, 1, 0, /* BN */ 1, 0, 0, /* T */ 0, 0, 1, /* uv diffuse */ u0, v0, /* uv lightmap */ u0, v0, },
        { 1, 0, 0, /* N */ 0, 1, 0, /* BN */ 1, 0, 0, /* T */ 0, 0, 1, /* uv diffuse */ u1, v0, /* uv lightmap */ u0, v1, },
        { 1, 0, 1, /* N */ 0, 1, 0, /* BN */ 1, 0, 0, /* T */ 0, 0, 1, /* uv diffuse */ u1, v1, /* uv lightmap */ u1, v1, },
      };

      SingleRenderMesh sm;
      sm.diffuse = m_fontTexture;
      sm.lightmap = lightmap;

      for(auto& v : vertices)
        sm.vertices.push_back(v);

      RenderMesh glyph {};
      glyph.singleMeshes.push_back(sm);
      uploadVerticesToGPU(glyph);
      r.push_back(glyph);
    }

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
};
}

IRenderer* createRenderer(IGraphicsBackend* backend)
{
  return new Renderer(backend);
}

