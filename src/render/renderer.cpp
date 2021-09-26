// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

///////////////////////////////////////////////////////////////////////////////
// High-level renderer

#include "engine/display.h"

#include <cstring>
#include <memory>
#include <vector>

#include "base/geom.h"
#include "base/scene.h"
#include "base/span.h"
#include "base/util.h" // setExtension
#include "engine/graphics_backend.h"
#include "engine/rendermesh.h"
#include "matrix4.h"

#include "picture.h"

using namespace std;

namespace
{
template<typename T>
T blend(T a, T b, float alpha)
{
  return a * (1 - alpha) + b * alpha;
}

struct Renderer : Display
{
  Renderer(IGraphicsBackend* backend_) : backend(backend_)
  {
    m_fontModel = loadFontModels("res/font.png", 16, 16);
  }

  void setFullscreen(bool fs) { backend->setFullscreen(fs); }
  void setHdr(bool enable) { backend->setHdr(enable); }
  void setFsaa(bool enable) { backend->setFsaa(enable); }
  void setCaption(String caption) { backend->setCaption(caption); }

  void loadModel(int modelId, String path) override
  {
    if((int)m_Models.size() <= modelId)
      m_Models.resize(modelId + 1);

    m_Models[modelId] = loadRenderMesh(path);

    int i = 0;

    for(auto& single : m_Models[modelId].singleMeshes)
    {
      single.diffuse = loadTexture(setExtension(string(path.data), to_string(i) + ".diffuse.png"));
      single.lightmap = loadTexture(setExtension(string(path.data), to_string(i) + ".lightmap.png"));
      ++i;
    }

    backend->uploadVerticesToGPU(m_Models[modelId]);
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

  void setAmbientLight(float ambientLight) { backend->setAmbientLight(ambientLight); }
  void setLight(int idx, Vector3f pos, Vector3f color) { backend->setLight(idx, pos, color); }
  void beginDraw() { backend->beginDraw(); }
  void endDraw() { backend->endDraw(); }

  void drawActor(Rect3f where, Quaternion orientation, int modelId, bool blinking, int actionIdx, float ratio) override
  {
    (void)actionIdx;
    (void)ratio;
    auto& model = m_Models.at(modelId);
    backend->pushMesh(where, orientation, m_camera, model, blinking, true);
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
      backend->pushMesh(rect, orientation, cam, m_fontModel[*text], false, false);
      rect.pos.x += rect.size.cx;
      ++text;
    }
  }

private:
  Camera m_camera;
  bool m_cameraValid = false;
  IGraphicsBackend* const backend;
  vector<RenderMesh> m_Models;
  vector<RenderMesh> m_fontModel;

  std::vector<RenderMesh> loadFontModels(String path, int COLS, int ROWS)
  {
    std::vector<RenderMesh> r;

    const int diffuse = backend->uploadTextureToGPU(addBorderToTiles(loadPicture(path), COLS, ROWS));
    const int lightmap = loadTexture("res/white.png");

    // don't GL_REPEAT fonts
    backend->setTextureNoRepeat(diffuse);
    backend->setTextureNoRepeat(lightmap);

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
      sm.diffuse = diffuse;
      sm.lightmap = lightmap;

      for(auto& v : vertices)
        sm.vertices.push_back(v);

      RenderMesh glyph {};
      glyph.singleMeshes.push_back(sm);
      backend->uploadVerticesToGPU(glyph);
      r.push_back(glyph);
    }

    return r;
  }

  int loadTexture(String path)
  {
    auto pic = loadPicture(path);
    return backend->uploadTextureToGPU(pic);
  }
};
}

Display* createRenderer(IGraphicsBackend* backend)
{
  return new Renderer(backend);
}

