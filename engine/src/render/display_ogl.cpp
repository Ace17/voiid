// Copyright (C) 2018 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// OpenGL stuff

#include "display.h"

#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <vector>
using namespace std;

#include "glad.h"
#include "png.h"
#include "SDL.h" // SDL_INIT_VIDEO

#include "base/geom.h"
#include "base/scene.h"
#include "base/span.h"
#include "base/util.h"
#include "matrix4.h"
#include "misc/file.h"
#include "rendermesh.h"

extern const Span<uint8_t> VertexShaderCode;
extern const Span<uint8_t> FragmentShaderCode;
extern const Span<uint8_t> HdrVertexShaderCode;
extern const Span<uint8_t> HdrFragmentShaderCode;
extern RenderMesh boxModel();

#ifdef NDEBUG
#define SAFE_GL(a) a
#else
#define SAFE_GL(a) \
  do { a; ensureGl(# a, __LINE__); } while (0)
#endif

namespace
{
void ensureGl(char const* expr, int line)
{
  auto const errorCode = glGetError();

  if(errorCode == GL_NO_ERROR)
    return;

  string ss;
  ss += "OpenGL error\n";
  ss += "Expr: " + string(expr) + "\n";
  ss += "Line: " + to_string(line) + "\n";
  ss += "Code: " + to_string(errorCode) + "\n";
  throw runtime_error(ss);
}

GLuint safeGetUniformLocation(GLuint programId, const char* name)
{
  auto uniformLocation = glGetUniformLocation(programId, name);

  if(uniformLocation < 0)
    throw runtime_error("Can't get location for uniform '" + string(name) + "'");

  return uniformLocation;
}

GLuint safeGetAttributeLocation(GLuint programId, const char* name)
{
  auto attribLocation = glGetAttribLocation(programId, name);

  if(attribLocation < 0)
    throw runtime_error("Can't get location for attribute '" + string(name) + "'");

  return attribLocation;
}

int compileShader(Span<uint8_t> code, int type)
{
  auto shaderId = glCreateShader(type);

  if(!shaderId)
    throw runtime_error("Can't create shader");

  printf("[display] compiling %s shader ... ", (type == GL_VERTEX_SHADER ? "vertex" : "fragment"));
  auto srcPtr = (const char*)code.data;
  auto length = (GLint)code.len;
  SAFE_GL(glShaderSource(shaderId, 1, &srcPtr, &length));
  SAFE_GL(glCompileShader(shaderId));

  // Check compile result
  GLint Result;
  SAFE_GL(glGetShaderiv(shaderId, GL_COMPILE_STATUS, &Result));

  if(!Result)
  {
    int logLength;
    glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);
    vector<char> msg(logLength);
    glGetShaderInfoLog(shaderId, logLength, nullptr, msg.data());
    fprintf(stderr, "%s\n", msg.data());

    throw runtime_error("Can't compile shader");
  }

  printf("OK\n");

  return shaderId;
}

int linkShaders(vector<int> ids)
{
  // Link the program
  printf("[display] Linking shaders ... ");
  auto ProgramID = glCreateProgram();

  for(auto id : ids)
    glAttachShader(ProgramID, id);

  glLinkProgram(ProgramID);

  // Check the program
  GLint Result = GL_FALSE;
  glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);

  if(!Result)
  {
    int logLength;
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &logLength);
    vector<char> msg(logLength);
    glGetProgramInfoLog(ProgramID, logLength, nullptr, msg.data());
    fprintf(stderr, "%s\n", msg.data());

    throw runtime_error("Can't link shader");
  }

  printf("OK\n");

  return ProgramID;
}

struct Picture
{
  Size2i dim;
  int stride;
  vector<uint8_t> pixels;
};

Picture loadPng(string path)
{
  Picture pic;
  auto pngDataBuf = File::read(path);
  auto pngData = Span<const uint8_t>((uint8_t*)pngDataBuf.data(), (int)pngDataBuf.size());
  pic.pixels = decodePng(pngData, pic.dim.width, pic.dim.height);
  pic.stride = pic.dim.width * 4;

  return pic;
}

Picture loadPicture(const char* path, Rect2f frect)
{
  try
  {
    auto surface = loadPng(path);

    if(frect.size.width == 0 && frect.size.height == 0)
      frect = Rect2f(0, 0, 1, 1);

    if(frect.pos.x < 0 || frect.pos.y < 0 || frect.pos.x + frect.size.width > 1 || frect.pos.y + frect.size.height > 1)
      throw runtime_error("Invalid boundaries for '" + string(path) + "'");

    auto const bpp = 4;

    Rect2i rect;
    rect.pos.x = frect.pos.x * surface.dim.width;
    rect.pos.y = frect.pos.y * surface.dim.height;
    rect.size.width = frect.size.width * surface.dim.width;
    rect.size.height = frect.size.height * surface.dim.height;

    vector<uint8_t> img(rect.size.width * rect.size.height * bpp);

    auto src = (Uint8*)surface.pixels.data() + rect.pos.x * bpp + rect.pos.y * surface.stride;
    auto dst = (Uint8*)img.data() + bpp * rect.size.width * rect.size.height;

    // from glTexImage2D doc:
    // "The first element corresponds to the lower left corner of the texture image",
    // (e.g (u,v) = (0,0))
    for(int y = 0; y < rect.size.height; ++y)
    {
      dst -= bpp * rect.size.width;
      memcpy(dst, src, bpp * rect.size.width);
      src += surface.stride;
    }

    Picture r;
    r.dim = rect.size;
    r.stride = rect.size.width;
    r.pixels = std::move(img);

    return r;
  }
  catch(std::exception const& e)
  {
    printf("[display] can't load texture: %s\n", e.what());
    printf("[display] falling back on generated texture\n");

    Picture r;
    r.dim = Size2i(32, 32);
    r.pixels.resize(r.dim.width * r.dim.height * 4);

    for(int y = 0; y < r.dim.height; ++y)
    {
      for(int x = 0; x < r.dim.width; ++x)
      {
        r.pixels[(x + y * r.dim.width) * 4 + 0] = 0xff;
        r.pixels[(x + y * r.dim.width) * 4 + 1] = x < 16 ? 0xff : 0x00;
        r.pixels[(x + y * r.dim.width) * 4 + 2] = y < 16 ? 0xff : 0x00;
        r.pixels[(x + y * r.dim.width) * 4 + 3] = 0xff;
      }
    }

    return r;
  }
}

GLuint uploadTextureToGPU(const Picture& pic)
{
  GLuint texture;

  glGenTextures(1, &texture);

  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pic.dim.width, pic.dim.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pic.pixels.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glBindTexture(GL_TEXTURE_2D, 0);

  return texture;
}

int loadTexture(const char* path, Rect2f frect)
{
  const auto pic = loadPicture(path, frect);
  return uploadTextureToGPU(pic);
}

GLuint loadShaders(Span<uint8_t> vsCode, Span<uint8_t> fsCode)
{
  auto const vertexId = compileShader(vsCode, GL_VERTEX_SHADER);
  auto const fragmentId = compileShader(fsCode, GL_FRAGMENT_SHADER);

  auto const progId = linkShaders(vector<int>({ vertexId, fragmentId }));

  SAFE_GL(glDeleteShader(vertexId));
  SAFE_GL(glDeleteShader(fragmentId));

  return progId;
}

struct Camera
{
  Vector3f pos;
  Vector3f dir;
  bool valid = false;
};

void uploadVerticesToGPU(RenderMesh& mesh)
{
  for(auto& model : mesh.singleMeshes)
  {
    SAFE_GL(glGenBuffers(1, &model.buffer));
    SAFE_GL(glBindBuffer(GL_ARRAY_BUFFER, model.buffer));
    SAFE_GL(glBufferData(GL_ARRAY_BUFFER, sizeof(model.vertices[0]) * model.vertices.size(), model.vertices.data(), GL_STATIC_DRAW));
    SAFE_GL(glBindBuffer(GL_ARRAY_BUFFER, 0));
  }
}

std::vector<RenderMesh> loadTiledAnimation(const char* path, int COLS, int ROWS)
{
  std::vector<RenderMesh> r;

  for(int i = 0; i < COLS * ROWS; ++i)
  {
    auto m = boxModel();

    auto col = i % COLS;
    auto row = i / COLS;

    Rect2f rect;
    rect.size.width = 1.0 / COLS;
    rect.size.height = 1.0 / ROWS;
    rect.pos.x = col * rect.size.width;
    rect.pos.y = row * rect.size.height;

    m.singleMeshes[0].diffuse = loadTexture(path, rect);
    m.singleMeshes[0].lightmap = loadTexture("res/white.png", {});
    r.push_back(m);
  }

  return r;
}

void printOpenGlVersion()
{
  auto sVersion = (char const*)glGetString(GL_VERSION);
  auto sLangVersion = (char const*)glGetString(GL_SHADING_LANGUAGE_VERSION);

  auto notNull = [] (char const* s) -> char const*
    {
      return s ? s : "<null>";
    };

  printf("[display] OpenGL version: %s (shading version: %s)\n",
         notNull(sVersion),
         notNull(sLangVersion));
}

template<typename T>
T blend(T a, T b, float alpha)
{
  return a * (1 - alpha) + b * alpha;
}

struct OpenglDisplay : Display
{
  OpenglDisplay(Size2i resolution)
  {
    if(SDL_InitSubSystem(SDL_INIT_VIDEO))
      throw runtime_error(string("Can't init SDL video: ") + SDL_GetError());

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    // require OpenGL 2.0, ES or Core. No compatibility mode.
    {
      // SDL_GL_CONTEXT_PROFILE_ES: works in both browser and native
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    }

    m_window = SDL_CreateWindow(
      "",
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      resolution.width, resolution.height,
      SDL_WINDOW_OPENGL
      );

    if(!m_window)
      throw runtime_error(string("Can't create SDL window: ") + SDL_GetError());

    // Create our opengl context and attach it to our window
    m_context = SDL_GL_CreateContext(m_window);

    if(!m_context)
      throw runtime_error(string("Can't create OpenGL context: ") + SDL_GetError());

    if(!gladLoadGLES2Loader(&SDL_GL_GetProcAddress))
      throw runtime_error("Can't load OpenGL");

    printOpenGlVersion();

    // This makes our buffer swap syncronized with the monitor's vertical refresh
    SDL_GL_SetSwapInterval(1);

    // Create our unique vertex array
    GLuint VertexArrayID;
    SAFE_GL(glGenVertexArrays(1, &VertexArrayID));
    SAFE_GL(glBindVertexArray(VertexArrayID));

    glEnable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_fontModel = loadTiledAnimation("res/font.png", 16, 16);

    // don't GL_REPEAT fonts
    for(auto& glyph : m_fontModel)
    {
      for(auto& single : glyph.singleMeshes)
      {
        glBindTexture(GL_TEXTURE_2D, single.diffuse);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      }
    }

    for(auto& glyph : m_fontModel)
      uploadVerticesToGPU(glyph);

    {
      m_shader.programId = loadShaders(VertexShaderCode, FragmentShaderCode);

      m_shader.M = safeGetUniformLocation(m_shader.programId, "M");
      m_shader.MVP = safeGetUniformLocation(m_shader.programId, "MVP");
      m_shader.DiffuseTex = safeGetUniformLocation(m_shader.programId, "DiffuseTex");
      m_shader.LightmapTex = safeGetUniformLocation(m_shader.programId, "LightmapTex");
      m_shader.colorId = safeGetUniformLocation(m_shader.programId, "fragOffset");
      m_shader.ambientLoc = safeGetUniformLocation(m_shader.programId, "ambientLight");
      m_shader.positionLoc = safeGetAttributeLocation(m_shader.programId, "vertexPos_model");
      m_shader.uvDiffuseLoc = safeGetAttributeLocation(m_shader.programId, "vertexUV");
      m_shader.uvLightmapLoc = safeGetAttributeLocation(m_shader.programId, "vertexUV_lightmap");
      m_shader.normalLoc = safeGetAttributeLocation(m_shader.programId, "a_normal");
    }

    {
      m_hdrShader.programId = loadShaders(HdrVertexShaderCode, HdrFragmentShaderCode);

      m_hdrShader.HdrTex = safeGetUniformLocation(m_hdrShader.programId, "HdrTex");
      m_hdrShader.positionLoc = safeGetAttributeLocation(m_hdrShader.programId, "vertexPos_model");
      m_hdrShader.uvLoc = safeGetAttributeLocation(m_hdrShader.programId, "vertexUV");
    }

    {
      SAFE_GL(glGenFramebuffers(1, &m_hdrFramebuffer));
      SAFE_GL(glBindFramebuffer(GL_FRAMEBUFFER, m_hdrFramebuffer));

      // color buffer
      {
        SAFE_GL(glGenTextures(1, &m_hdrTexture));
        SAFE_GL(glBindTexture(GL_TEXTURE_2D, m_hdrTexture));
        SAFE_GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, resolution.width, resolution.height, 0, GL_RGBA, GL_FLOAT, nullptr));
        SAFE_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        SAFE_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        SAFE_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_hdrTexture, 0));
      }

      // depth buffer
      {
        SAFE_GL(glGenTextures(1, &m_hdrDepthTexture));
        SAFE_GL(glBindTexture(GL_TEXTURE_2D, m_hdrDepthTexture));
        SAFE_GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, resolution.width, resolution.height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL));
        SAFE_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_hdrDepthTexture, 0));
      }

      SAFE_GL(glGenBuffers(1, &m_hdrVbo));
    }

    printf("[display] init OK\n");
  }

  ~OpenglDisplay()
  {
    SAFE_GL(glDeleteBuffers(1, &m_hdrVbo));
    SAFE_GL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    SAFE_GL(glDeleteFramebuffers(1, &m_hdrFramebuffer));

    SDL_GL_DeleteContext(m_context);
    SDL_DestroyWindow(m_window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);

    printf("[display] shutdown OK\n");
  }

  void setFullscreen(bool fs) override
  {
    auto flags = fs ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
    SDL_SetWindowFullscreen(m_window, flags);
  }

  void setCaption(const char* caption) override
  {
    SDL_SetWindowTitle(m_window, caption);
  }

  void loadModel(int id, const char* path) override
  {
    if((int)m_Models.size() <= id)
      m_Models.resize(id + 1);

    m_Models[id] = ::loadModel(path);

    int i = 0;

    for(auto& single : m_Models[id].singleMeshes)
    {
      single.diffuse = loadTexture(setExtension(path, to_string(i) + ".diffuse.png").c_str(), {});
      single.lightmap = loadTexture(setExtension(path, to_string(i) + ".lightmap.png").c_str(), {});
      ++i;
    }

    uploadVerticesToGPU(m_Models[id]);
  }

  void setCamera(Vector3f pos, Quaternion dir) override
  {
    Vector3f v(1, 0, 0);
    auto cam = (Camera { pos, dir.rotate(v) });

    if(!m_camera.valid)
    {
      m_camera = cam;
      m_camera.valid = true;
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

  void setAmbientLight(float ambientLight) override
  {
    m_ambientLight = ambientLight;
  }

  void beginDraw() override
  {
    m_frameCount++;

    {
      int w, h;
      SDL_GL_GetDrawableSize(m_window, &w, &h);
      SAFE_GL(glViewport(0, 0, w, h));
    }

    // draw to the HDR buffer
    SAFE_GL(glBindFramebuffer(GL_FRAMEBUFFER, m_hdrFramebuffer));

    SAFE_GL(glUseProgram(m_shader.programId));

    glEnable(GL_DEPTH_TEST);
    SAFE_GL(glClearColor(0, 0, 0, 1));
    SAFE_GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    SAFE_GL(glUniform3f(m_shader.ambientLoc, m_ambientLight, m_ambientLight, m_ambientLight));
  }

  void endDraw() override
  {
    drawHdrBufferToScreen();

    SDL_GL_SwapWindow(m_window);
  }

  // end-of public API
  void drawHdrBufferToScreen()
  {
    // draw to screen
    SAFE_GL(glBindFramebuffer(GL_FRAMEBUFFER, 0));

    SAFE_GL(glUseProgram(m_hdrShader.programId));
    SAFE_GL(glDisable(GL_DEPTH_TEST));

    // Texture Unit 0
    SAFE_GL(glActiveTexture(GL_TEXTURE0));
    SAFE_GL(glBindTexture(GL_TEXTURE_2D, m_hdrTexture));
    SAFE_GL(glUniform1i(m_hdrShader.HdrTex, 0));

    struct QuadVertex
    {
      float x, y, u, v;
    };

    static const QuadVertex screenQuad[] =
    {
      { -1, -1, 0, 0 },
      { +1, +1, 1, 1 },
      { -1, +1, 0, 1 },

      { -1, -1, 0, 0 },
      { +1, -1, 1, 0 },
      { +1, +1, 1, 1 },
    };

    SAFE_GL(glBindBuffer(GL_ARRAY_BUFFER, m_hdrVbo));
    SAFE_GL(glBufferData(GL_ARRAY_BUFFER, sizeof screenQuad, screenQuad, GL_STATIC_DRAW));

    SAFE_GL(glEnableVertexAttribArray(m_hdrShader.positionLoc));
    SAFE_GL(glEnableVertexAttribArray(m_hdrShader.uvLoc));

#define OFFSET(a) (void*)(&(((QuadVertex*)nullptr)->a))
    SAFE_GL(glVertexAttribPointer(m_hdrShader.positionLoc, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), OFFSET(x)));
    SAFE_GL(glVertexAttribPointer(m_hdrShader.uvLoc, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), OFFSET(u)));
#undef OFFSET

    SAFE_GL(glDrawArrays(GL_TRIANGLES, 0, 6));
    SAFE_GL(glBindBuffer(GL_ARRAY_BUFFER, 0));
  }

  void renderMesh(Rect3f where, Camera const& camera, RenderMesh& model, bool blinking)
  {
    for(auto& single : model.singleMeshes)
      renderSingleMesh(where, camera, single, blinking);
  }

  void renderSingleMesh(Rect3f where, Camera const& camera, SingleRenderMesh& model, bool blinking)
  {
    SAFE_GL(glUniform4f(m_shader.colorId, 0, 0, 0, 0));

    if(blinking)
    {
      if((m_frameCount / 4) % 2)
        SAFE_GL(glUniform4f(m_shader.colorId, 0.8, 0.4, 0.4, 0));
    }

    // Texture Unit 0: Diffuse
    SAFE_GL(glActiveTexture(GL_TEXTURE0));
    SAFE_GL(glBindTexture(GL_TEXTURE_2D, model.diffuse));
    SAFE_GL(glUniform1i(m_shader.DiffuseTex, 0));

    // Texture Unit 1: Lightmap
    SAFE_GL(glActiveTexture(GL_TEXTURE1));
    SAFE_GL(glBindTexture(GL_TEXTURE_2D, model.lightmap));
    SAFE_GL(glUniform1i(m_shader.LightmapTex, 1));

    auto const target = camera.pos + camera.dir;
    auto const view = ::lookAt(camera.pos, target, Vector3f(0, 0, 1));
    auto const pos = ::translate(where.pos);
    auto const scale = ::scale(Vector3f(where.size.cx, where.size.cy, where.size.cz));

    static const float fovy = (float)((60.0f / 180) * PI);
    static const float aspect = 16.0f / 9.0;
    static const float near_ = 0.1f;
    static const float far_ = 100.0f;
    static const auto perspective = ::perspective(fovy, aspect, near_, far_);

    auto MV = pos * scale;
    auto MVP = perspective * view * MV;

    SAFE_GL(glUniformMatrix4fv(m_shader.M, 1, GL_FALSE, &MV[0][0]));
    SAFE_GL(glUniformMatrix4fv(m_shader.MVP, 1, GL_FALSE, &MVP[0][0]));

    SAFE_GL(glBindBuffer(GL_ARRAY_BUFFER, model.buffer));

    SAFE_GL(glEnableVertexAttribArray(m_shader.positionLoc));
    SAFE_GL(glEnableVertexAttribArray(m_shader.normalLoc));
    SAFE_GL(glEnableVertexAttribArray(m_shader.uvDiffuseLoc));
    SAFE_GL(glEnableVertexAttribArray(m_shader.uvLightmapLoc));

#define OFFSET(a) (void*)(&(((SingleRenderMesh::Vertex*)nullptr)->a))
    SAFE_GL(glVertexAttribPointer(m_shader.positionLoc, 3, GL_FLOAT, GL_FALSE, sizeof(SingleRenderMesh::Vertex), OFFSET(x)));
    SAFE_GL(glVertexAttribPointer(m_shader.normalLoc, 3, GL_FLOAT, GL_FALSE, sizeof(SingleRenderMesh::Vertex), OFFSET(nx)));
    SAFE_GL(glVertexAttribPointer(m_shader.uvDiffuseLoc, 2, GL_FLOAT, GL_FALSE, sizeof(SingleRenderMesh::Vertex), OFFSET(diffuse_u)));
    SAFE_GL(glVertexAttribPointer(m_shader.uvLightmapLoc, 2, GL_FLOAT, GL_FALSE, sizeof(SingleRenderMesh::Vertex), OFFSET(lightmap_u)));
#undef OFFSET

    SAFE_GL(glDrawArrays(GL_TRIANGLES, 0, model.vertices.size()));
  }

  void readPixels(Span<uint8_t> dstRgbPixels) override
  {
    int width, height;
    SDL_GetWindowSize(m_window, &width, &height);
    SAFE_GL(glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, dstRgbPixels.data));

    // reverse upside down
    const auto rowSize = width * 4;
    vector<uint8_t> rowBuf(rowSize);

    for(int row = 0; row < height / 2; ++row)
    {
      const auto rowLo = row;
      const auto rowHi = height - 1 - row;
      auto pRowLo = dstRgbPixels.data + rowLo * rowSize;
      auto pRowHi = dstRgbPixels.data + rowHi * rowSize;
      memcpy(rowBuf.data(), pRowLo, rowSize);
      memcpy(pRowLo, pRowHi, rowSize);
      memcpy(pRowHi, rowBuf.data(), rowSize);
    }
  }

  void enableGrab(bool enable) override
  {
    SDL_SetRelativeMouseMode(enable ? SDL_TRUE : SDL_FALSE);
    SDL_SetWindowGrab(m_window, enable ? SDL_TRUE : SDL_FALSE);
    SDL_ShowCursor(enable ? 0 : 1);
  }

  void drawActor(Rect3f where, int modelId, bool blinking, int actionIdx, float ratio) override
  {
    (void)actionIdx;
    (void)ratio;
    auto& model = m_Models.at(modelId);
    renderMesh(where, m_camera, model, blinking);
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

    auto cam = (Camera { Vector3f(0, -10, 0), Vector3f(0, 1, 0) });

    glDisable(GL_DEPTH_TEST);

    while(*text)
    {
      renderMesh(rect, cam, m_fontModel[*text], false);
      rect.pos.x += rect.size.cx;
      ++text;
    }
  }

private:
  SDL_Window* m_window;
  SDL_GLContext m_context;

  Camera m_camera;

  // shader attribute/uniform locations
  struct Shader
  {
    GLuint programId;
    GLint M;
    GLint MVP;
    GLint colorId;
    GLint ambientLoc;
    GLint DiffuseTex;
    GLint LightmapTex;
    GLint positionLoc;
    GLint uvDiffuseLoc;
    GLint uvLightmapLoc;
    GLint normalLoc;
  };

  struct HdrShader
  {
    GLuint programId;
    GLint HdrTex;
    GLint positionLoc;
    GLint uvLoc;
  };

  Shader m_shader;
  HdrShader m_hdrShader;

  vector<RenderMesh> m_Models;
  vector<RenderMesh> m_fontModel;

  float m_ambientLight = 0;
  int m_frameCount = 0;
  GLuint m_hdrFramebuffer = 0;
  GLuint m_hdrTexture = 0;
  GLuint m_hdrDepthTexture = 0;
  GLuint m_hdrVbo = 0;
};
}

Display* createDisplay(Size2i resolution)
{
  return new OpenglDisplay(resolution);
}

