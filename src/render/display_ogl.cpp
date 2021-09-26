// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// OpenGL stuff

#include "engine/display.h"

#include <cassert>
#include <chrono>
#include <cstdio>
#include <stdexcept>
#include <vector>
using namespace std;

#include "glad.h"
#include "SDL.h" // SDL_INIT_VIDEO

#include "base/geom.h"
#include "base/scene.h"
#include "base/span.h"
#include "base/util.h"
#include "engine/rendermesh.h"
#include "engine/stats.h"
#include "matrix4.h"
#include "misc/file.h"

#include "picture.h"

#ifdef NDEBUG
#define SAFE_GL(a) a
#else
#define SAFE_GL(a) \
  do { a; ensureGl(# a, __FILE__, __LINE__); } while (0)
#endif

void ensureGl(char const* expr, const char* file, int line)
{
  auto const errorCode = glGetError();

  if(errorCode == GL_NO_ERROR)
    return;

  string ss;
  ss += "OpenGL error\n";
  ss += string(file) + "(" + to_string(line) + "): " + string(expr) + "\n";
  ss += "Error code: " + to_string(errorCode) + "\n";
  throw runtime_error(ss);
}

#define OFFSET(VertexType, Attribute) \
  (void*)(&(((VertexType*)nullptr)->Attribute))

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

GLuint compileShader(Span<const uint8_t> code, int type)
{
  auto shaderId = glCreateShader(type);

  if(!shaderId)
    throw runtime_error("Can't create shader");

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

  return shaderId;
}

GLuint linkShaders(vector<GLuint> ids)
{
  // Link the program
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

  return ProgramID;
}

GLuint uploadTextureToGPU(PictureView pic)
{
  GLuint texture;

  glGenTextures(1, &texture);

  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pic.dim.width, pic.dim.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pic.pixels);
  SAFE_GL(glGenerateMipmap(GL_TEXTURE_2D));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glBindTexture(GL_TEXTURE_2D, 0);

  return texture;
}

int loadTexture(String path)
{
  auto pic = loadPicture(path);
  return uploadTextureToGPU(pic);
}

GLuint loadShaders(Span<const uint8_t> vsCode, Span<const uint8_t> fsCode)
{
  auto const vertexId = compileShader(vsCode, GL_VERTEX_SHADER);
  auto const fragmentId = compileShader(fsCode, GL_FRAGMENT_SHADER);

  auto const progId = linkShaders({ vertexId, fragmentId });

  SAFE_GL(glDeleteShader(vertexId));
  SAFE_GL(glDeleteShader(fragmentId));

  return progId;
}

GLuint loadShader(std::string name)
{
  printf("[display] loading shader '%s'\n", name.c_str());
  auto vsCode = File::read("res/shaders/" + name + ".vert");
  auto fsCode = File::read("res/shaders/" + name + ".frag");

  auto toSpan = [] (const std::string& s)
    {
      return Span<const uint8_t>((const uint8_t*)s.c_str(), s.size());
    };

  return loadShaders(toSpan(vsCode), toSpan(fsCode));
}

struct Camera
{
  Vector3f pos;
  Quaternion dir;
  bool valid = false;
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

std::vector<RenderMesh> loadFontModels(String path, int COLS, int ROWS)
{
  std::vector<RenderMesh> r;

  const int diffuse = uploadTextureToGPU(addBorderToTiles(loadPicture(path), COLS, ROWS));
  const int lightmap = loadTexture("res/white.png");

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

    RenderMesh rm {};
    rm.singleMeshes.push_back(sm);
    r.push_back(rm);
  }

  return r;
}

void printOpenGlVersion()
{
  auto notNull = [] (const void* s) -> char const*
    {
      return s ? (const char*)s : "<null>";
    };

  printf("[display] %s [%s]\n",
         notNull(glGetString(GL_VERSION)),
         notNull(glGetString(GL_SHADING_LANGUAGE_VERSION)));

  printf("[display] GPU: %s [%s]\n",
         notNull(glGetString(GL_RENDERER)),
         notNull(glGetString(GL_VENDOR)));
}

template<typename T>
T blend(T a, T b, float alpha)
{
  return a * (1 - alpha) + b * alpha;
}

struct Shader
{
  operator GLuint () { return program; }
  GLuint program;
};

struct RenderPass
{
  virtual ~RenderPass() = default;

  struct FrameBuffer
  {
    GLuint id;
    Size2i dim;
  };

  virtual FrameBuffer getInputFrameBuffer() = 0;
  virtual void execute(FrameBuffer dst) = 0;
};

struct ScreenRenderPass : RenderPass
{
  Size2i screenSize {};

  FrameBuffer getInputFrameBuffer() override { return { 0, screenSize }; }
  void execute(FrameBuffer) override { /* nothing to do */ }
};

struct PostProcessing
{
  PostProcessing(Size2i resolution)
    : m_resolution(resolution)
  {
    m_hdrShader.program = loadShader("hdr");
    m_bloomShader.program = loadShader("bloom");

    SAFE_GL(glGenBuffers(1, &m_hdrQuadVbo));

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
        SAFE_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        SAFE_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
        SAFE_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_hdrTexture, 0));
      }

      // depth buffer
      {
        SAFE_GL(glGenTextures(1, &m_hdrDepthTexture));
        SAFE_GL(glBindTexture(GL_TEXTURE_2D, m_hdrDepthTexture));
        SAFE_GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, resolution.width, resolution.height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL));
        SAFE_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_hdrDepthTexture, 0));
      }
    }

    for(int k = 0; k < 2; ++k)
    {
      SAFE_GL(glGenFramebuffers(1, &m_bloomFramebuffer[k]));
      SAFE_GL(glBindFramebuffer(GL_FRAMEBUFFER, m_bloomFramebuffer[k]));

      // color buffer
      {
        SAFE_GL(glGenTextures(1, &m_bloomTexture[k]));
        SAFE_GL(glBindTexture(GL_TEXTURE_2D, m_bloomTexture[k]));
        SAFE_GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, resolution.width, resolution.height, 0, GL_RGBA, GL_FLOAT, nullptr));
        SAFE_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        SAFE_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        SAFE_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        SAFE_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
        SAFE_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_bloomTexture[k], 0));
      }
    }
  }

  ~PostProcessing()
  {
    SAFE_GL(glDeleteBuffers(1, &m_hdrQuadVbo));
    SAFE_GL(glDeleteFramebuffers(1, &m_hdrFramebuffer));
  }

  void applyBloomFilter()
  {
    SAFE_GL(glViewport(0, 0, m_resolution.width, m_resolution.height));

    SAFE_GL(glUseProgram(m_bloomShader));
    SAFE_GL(glDisable(GL_DEPTH_TEST));

    SAFE_GL(glBindBuffer(GL_ARRAY_BUFFER, m_hdrQuadVbo));
    SAFE_GL(glBufferData(GL_ARRAY_BUFFER, sizeof screenQuad, screenQuad, GL_STATIC_DRAW));

    SAFE_GL(glEnableVertexAttribArray(BloomShader::Attribute::positionLoc));
    SAFE_GL(glEnableVertexAttribArray(BloomShader::Attribute::uvLoc));

    SAFE_GL(glVertexAttribPointer(BloomShader::Attribute::positionLoc, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), OFFSET(QuadVertex, x)));
    SAFE_GL(glVertexAttribPointer(BloomShader::Attribute::uvLoc, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), OFFSET(QuadVertex, u)));

    auto oneBlurringPass = [&] (GLuint inputTex, GLuint outputFramebuffer, bool isThreshold = false)
      {
        SAFE_GL(glUniform1i(BloomShader::Uniform::IsThreshold, isThreshold));

        // Texture Unit 0
        SAFE_GL(glActiveTexture(GL_TEXTURE0));
        SAFE_GL(glBindTexture(GL_TEXTURE_2D, inputTex));
        SAFE_GL(glUniform1i(BloomShader::Uniform::InputTex, 0));

        SAFE_GL(glBindFramebuffer(GL_FRAMEBUFFER, outputFramebuffer));
        SAFE_GL(glDrawArrays(GL_TRIANGLES, 0, 6));
      };

    oneBlurringPass(m_hdrTexture, m_bloomFramebuffer[0], true);
    oneBlurringPass(m_bloomTexture[0], m_bloomFramebuffer[1]);
    oneBlurringPass(m_bloomTexture[1], m_bloomFramebuffer[0]);
    oneBlurringPass(m_bloomTexture[0], m_bloomFramebuffer[1]);
    oneBlurringPass(m_bloomTexture[1], m_bloomFramebuffer[0]);
    oneBlurringPass(m_bloomTexture[0], m_bloomFramebuffer[1]);
    oneBlurringPass(m_bloomTexture[1], m_bloomFramebuffer[0]);

    SAFE_GL(glBindBuffer(GL_ARRAY_BUFFER, 0));
  }

  void drawHdrBuffer(Size2i screenSize)
  {
    SAFE_GL(glViewport(0, 0, screenSize.width, screenSize.height));

    SAFE_GL(glUseProgram(m_hdrShader));

    SAFE_GL(glDisable(GL_DEPTH_TEST));

    // Texture Unit 0
    SAFE_GL(glActiveTexture(GL_TEXTURE0));
    SAFE_GL(glBindTexture(GL_TEXTURE_2D, m_hdrTexture));
    SAFE_GL(glUniform1i(HdrShader::Uniform::InputTex1, 0));

    // Texture Unit 1
    SAFE_GL(glActiveTexture(GL_TEXTURE1));
    SAFE_GL(glBindTexture(GL_TEXTURE_2D, m_bloomTexture[0]));
    SAFE_GL(glUniform1i(HdrShader::Uniform::InputTex2, 1));

    SAFE_GL(glBindBuffer(GL_ARRAY_BUFFER, m_hdrQuadVbo));
    SAFE_GL(glBufferData(GL_ARRAY_BUFFER, sizeof screenQuad, screenQuad, GL_STATIC_DRAW));

    SAFE_GL(glEnableVertexAttribArray(HdrShader::Attribute::positionLoc));
    SAFE_GL(glVertexAttribPointer(HdrShader::Attribute::positionLoc, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), OFFSET(QuadVertex, x)));

    SAFE_GL(glEnableVertexAttribArray(HdrShader::Attribute::uvLoc));
    SAFE_GL(glVertexAttribPointer(HdrShader::Attribute::uvLoc, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), OFFSET(QuadVertex, u)));

    SAFE_GL(glDrawArrays(GL_TRIANGLES, 0, 6));
    SAFE_GL(glBindBuffer(GL_ARRAY_BUFFER, 0));
  }

  struct HdrShader : Shader
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
  };

  struct BloomShader : Shader
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
  };

  const Size2i m_resolution;

  HdrShader m_hdrShader;
  BloomShader m_bloomShader;
  GLuint m_hdrFramebuffer = 0;
  GLuint m_hdrTexture = 0;
  GLuint m_hdrDepthTexture = 0;

  GLuint m_bloomFramebuffer[2] {};
  GLuint m_bloomTexture[2] {};

  GLuint m_hdrQuadVbo = 0;
};

struct PostProcessRenderPass : RenderPass
{
  FrameBuffer getInputFrameBuffer() override
  {
    return { postproc->m_hdrFramebuffer, postproc->m_resolution };
  }

  void execute(FrameBuffer dst) override
  {
    postproc->applyBloomFilter();

    SAFE_GL(glBindFramebuffer(GL_FRAMEBUFFER, dst.id));
    postproc->drawHdrBuffer(dst.dim);
  }

  std::unique_ptr<PostProcessing> postproc;
};

struct MeshRenderPass : RenderPass
{
  FrameBuffer getInputFrameBuffer() override { return {}; }

  void execute(FrameBuffer dst) override
  {
    SAFE_GL(glBindFramebuffer(GL_FRAMEBUFFER, dst.id));
    SAFE_GL(glViewport(0, 0, dst.dim.width, dst.dim.height));

    SAFE_GL(glClearColor(0, 0, 0, 1));
    SAFE_GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    for(auto& cmd : m_drawCommands)
      executeDrawCommand(cmd);

    Stat("Draw calls", m_drawCommands.size());
  }

  void executeDrawCommand(const DrawCommand& cmd)
  {
    auto& model = *cmd.pMesh;
    auto& where = cmd.where;

    if(cmd.depthtest)
    {
      SAFE_GL(glUseProgram(m_meshShader));

      glEnable(GL_DEPTH_TEST);
      SAFE_GL(glUniform3f(MeshShader::Uniform::ambientLoc, m_ambientLight, m_ambientLight, m_ambientLight));
      SAFE_GL(glUniform4f(MeshShader::Uniform::colorId, 0, 0, 0, 0));

      assert(m_lights.size() < 32);
      SAFE_GL(glUniform1i(MeshShader::Uniform::LightCountLoc, (int)m_lights.size()));

      for(auto& light : m_lights)
      {
        const auto i = int(&light - m_lights.data());
        SAFE_GL(glUniform3f(MeshShader::Uniform::LightPosLoc + i, light.pos.x, light.pos.y, light.pos.z));
        SAFE_GL(glUniform3f(MeshShader::Uniform::LightColorLoc + i, light.color.x, light.color.y, light.color.z));
      }

      if(cmd.blinking)
      {
        if(SDL_GetTicks() % 100 < 50)
          SAFE_GL(glUniform4f(MeshShader::Uniform::colorId, 0.8, 0.4, 0.4, 0));
      }

      // Texture Unit 0: Diffuse
      SAFE_GL(glActiveTexture(GL_TEXTURE0));
      SAFE_GL(glBindTexture(GL_TEXTURE_2D, model.diffuse));
      SAFE_GL(glUniform1i(MeshShader::Uniform::DiffuseTex, 0));

      // Texture Unit 1: Lightmap
      SAFE_GL(glActiveTexture(GL_TEXTURE1));
      SAFE_GL(glBindTexture(GL_TEXTURE_2D, model.lightmap));
      SAFE_GL(glUniform1i(MeshShader::Uniform::LightmapTex, 1));

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

      SAFE_GL(glUniformMatrix4fv(MeshShader::Uniform::M, 1, GL_FALSE, &MV[0][0]));
      SAFE_GL(glUniformMatrix4fv(MeshShader::Uniform::MVP, 1, GL_FALSE, &MVP[0][0]));
      SAFE_GL(glUniform3f(MeshShader::Uniform::CameraPos, cmd.camera.pos.x, cmd.camera.pos.y, cmd.camera.pos.z));

      SAFE_GL(glBindBuffer(GL_ARRAY_BUFFER, model.buffer));

      SAFE_GL(glEnableVertexAttribArray(MeshShader::Attribute::positionLoc));
      SAFE_GL(glVertexAttribPointer(MeshShader::Attribute::positionLoc, 3, GL_FLOAT, GL_FALSE, sizeof(SingleRenderMesh::Vertex), OFFSET(SingleRenderMesh::Vertex, x)));

      SAFE_GL(glEnableVertexAttribArray(MeshShader::Attribute::normalLoc));
      SAFE_GL(glVertexAttribPointer(MeshShader::Attribute::normalLoc, 3, GL_FLOAT, GL_FALSE, sizeof(SingleRenderMesh::Vertex), OFFSET(SingleRenderMesh::Vertex, nx)));

      SAFE_GL(glEnableVertexAttribArray(MeshShader::Attribute::uvDiffuseLoc));
      SAFE_GL(glVertexAttribPointer(MeshShader::Attribute::uvDiffuseLoc, 2, GL_FLOAT, GL_FALSE, sizeof(SingleRenderMesh::Vertex), OFFSET(SingleRenderMesh::Vertex, diffuse_u)));

      SAFE_GL(glEnableVertexAttribArray(MeshShader::Attribute::uvLightmapLoc));
      SAFE_GL(glVertexAttribPointer(MeshShader::Attribute::uvLightmapLoc, 2, GL_FLOAT, GL_FALSE, sizeof(SingleRenderMesh::Vertex), OFFSET(SingleRenderMesh::Vertex, lightmap_u)));

      SAFE_GL(glDrawArrays(GL_TRIANGLES, 0, model.vertices.size()));
    }
    else
    {
      SAFE_GL(glUseProgram(m_textShader));

      glDisable(GL_DEPTH_TEST);

      // Texture Unit 0: Diffuse
      SAFE_GL(glActiveTexture(GL_TEXTURE0));
      SAFE_GL(glBindTexture(GL_TEXTURE_2D, model.diffuse));
      SAFE_GL(glUniform1i(TextShader::Uniform::DiffuseTex, 0));

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

      SAFE_GL(glUniformMatrix4fv(TextShader::Uniform::MVP, 1, GL_FALSE, &MVP[0][0]));

      SAFE_GL(glBindBuffer(GL_ARRAY_BUFFER, model.buffer));

      SAFE_GL(glEnableVertexAttribArray(TextShader::Attribute::positionLoc));
      SAFE_GL(glEnableVertexAttribArray(TextShader::Attribute::uvDiffuseLoc));

      SAFE_GL(glVertexAttribPointer(TextShader::Attribute::positionLoc, 3, GL_FLOAT, GL_FALSE, sizeof(SingleRenderMesh::Vertex), OFFSET(SingleRenderMesh::Vertex, x)));
      SAFE_GL(glVertexAttribPointer(TextShader::Attribute::uvDiffuseLoc, 2, GL_FLOAT, GL_FALSE, sizeof(SingleRenderMesh::Vertex), OFFSET(SingleRenderMesh::Vertex, diffuse_u)));

      SAFE_GL(glDrawArrays(GL_TRIANGLES, 0, model.vertices.size()));
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

  struct MeshShader : Shader
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

  GLint m_textShader;
  GLint m_meshShader;

  struct Light
  {
    Vector3f pos;
    Vector3f color;
  };

  std::vector<DrawCommand> m_drawCommands;
  vector<Light> m_lights;
  float m_ambientLight = 0;
  float m_aspectRatio = 1.0;
};

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

    // Enable vsync
    SDL_GL_SetSwapInterval(1);

    glEnable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_fontModel = loadFontModels("res/font.png", 16, 16);

    for(auto& glyph : m_fontModel)
    {
      uploadVerticesToGPU(glyph);

      // don't GL_REPEAT fonts
      for(auto& single : glyph.singleMeshes)
      {
        glBindTexture(GL_TEXTURE_2D, single.diffuse);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      }
    }

    m_meshRenderPass.m_textShader = loadShader("text");
    m_meshRenderPass.m_meshShader = loadShader("mesh");

    m_postprocRenderPass.postproc = make_unique<PostProcessing>(resolution);

    printf("[display] init OK\n");
  }

  ~OpenglDisplay()
  {
    SAFE_GL(glBindFramebuffer(GL_FRAMEBUFFER, 0));

    m_postprocRenderPass.postproc.reset();

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

  void setHdr(bool enable) override
  {
    m_enablePostProcessing = enable;
  }

  void setFsaa(bool enable) override
  {
    if(enable != m_enableFsaa)
    {
      Size2i size = getCurrentScreenSize();

      if(enable)
        size = size * 2;

      m_postprocRenderPass.postproc = make_unique<PostProcessing>(size);
    }

    m_enableFsaa = enable;
  }

  void setCaption(String caption) override
  {
    SDL_SetWindowTitle(m_window, caption.data);
  }

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

    uploadVerticesToGPU(m_Models[modelId]);
  }

  void setCamera(Vector3f pos, Quaternion dir) override
  {
    auto cam = (Camera { pos, dir });

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
    m_meshRenderPass.m_ambientLight = ambientLight;
  }

  void setLight(int idx, Vector3f pos, Vector3f color) override
  {
    if(idx >= (int)m_meshRenderPass.m_lights.size())
      m_meshRenderPass.m_lights.resize(idx + 1);

    m_meshRenderPass.m_lights[idx] = { pos, color };
  }

  void beginDraw() override
  {
    m_frameCount++;
    m_meshRenderPass.m_drawCommands.clear();
  }

  MeshRenderPass m_meshRenderPass;
  PostProcessRenderPass m_postprocRenderPass;
  ScreenRenderPass m_screenRenderPass;

  void endDraw() override
  {
    const auto t0 = chrono::high_resolution_clock::now();

    auto screenSize = getCurrentScreenSize();

    m_meshRenderPass.m_aspectRatio = float(screenSize.width) / screenSize.height;
    m_screenRenderPass.screenSize = screenSize;

    RenderPass* passes[16];
    int count = 0;

    passes[count++] = &m_meshRenderPass;

    if(m_enablePostProcessing)
      passes[count++] = &m_postprocRenderPass;

    passes[count++] = &m_screenRenderPass;

    for(int i = 0; i + 1 < count; ++i)
    {
      auto framebuffer = passes[i + 1]->getInputFrameBuffer();
      passes[i]->execute(framebuffer);
    }

    const auto t1 = chrono::high_resolution_clock::now();

    Stat("Render time (ms)", chrono::duration_cast<chrono::microseconds>(t1 - t0).count() / 1000.0);
    SDL_GL_SwapWindow(m_window);
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

private:
  Size2i getCurrentScreenSize()
  {
    Size2i screenSize {};
    SDL_GL_GetDrawableSize(m_window, &screenSize.width, &screenSize.height);
    return screenSize;
  }

  void pushMesh(Rect3f where, Quaternion orientation, Camera const& camera, RenderMesh& model, bool blinking, bool depthtest)
  {
    for(auto& single : model.singleMeshes)
      m_meshRenderPass.m_drawCommands.push_back({ &single, where, orientation, camera, blinking, depthtest });
  }

private:
  SDL_Window* m_window;
  SDL_GLContext m_context;

  Camera m_camera;

  vector<RenderMesh> m_Models;
  vector<RenderMesh> m_fontModel;

  int m_frameCount = 0;

  bool m_enablePostProcessing = true;
  bool m_enableFsaa = false;
};
}

Display* createDisplay(Size2i resolution)
{
  return new OpenglDisplay(resolution);
}

