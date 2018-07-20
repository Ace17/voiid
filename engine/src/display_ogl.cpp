// Copyright (C) 2018 - Sebastien Alaiwan <sebastien.alaiwan@gmail.com>
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// OpenGL stuff

#include "display.h"

#include <cassert>
#include <cstdio>
#include <sstream>
#include <vector>
#include <stdexcept>
using namespace std;

#define GL_GLEXT_PROTOTYPES 1
#include "GL/gl.h"
#include "SDL_video.h"
#include "SDL_image.h"

#include "base/util.h"
#include "base/scene.h"
#include "base/geom.h"
#include "base/span.h"
#include "model.h"
#include "matrix4.h"

#ifdef NDEBUG
#define SAFE_GL(a) a
#else
#define SAFE_GL(a) \
  do { a; ensureGl(# a, __LINE__); } while(0)
#endif

static
void ensureGl(char const* expr, int line)
{
  auto const errorCode = glGetError();

  if(errorCode == GL_NO_ERROR)
    return;

  stringstream ss;
  ss << "OpenGL error" << endl;
  ss << "Expr: " << expr << endl;
  ss << "Line: " << line << endl;
  ss << "Code: 0x" << std::hex << errorCode;
  throw runtime_error(ss.str());
}

static
int compileShader(Span<unsigned char> code, int type)
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

static
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

static
SDL_Surface* loadPicture(string path)
{
  auto surface = IMG_Load((char*)path.c_str());

  if(!surface)
    throw runtime_error(string("Can't load texture: ") + SDL_GetError());

  if(surface->format->BitsPerPixel != 32)
    throw runtime_error("only 32 bit pictures are supported");

  return surface;
}

// exported to Model
int loadTexture(string path, Rect2i rect)
{
  auto surface = loadPicture(path);

  if(rect.size.width == 0 && rect.size.height == 0)
    rect = Rect2i(0, 0, surface->w, surface->h);

  if(rect.pos.x < 0 || rect.pos.y < 0 || rect.pos.x + rect.size.width > surface->w || rect.pos.y + rect.size.height > surface->h)
    throw runtime_error("Invalid boundaries for '" + path + "'");

  auto const bpp = surface->format->BytesPerPixel;

  vector<uint8_t> img(rect.size.width* rect.size.height* bpp);

  auto src = (Uint8*)surface->pixels + rect.pos.x * bpp + rect.pos.y * surface->pitch;
  auto dst = (Uint8*)img.data() + bpp * rect.size.width * rect.size.height;

  // from glTexImage2D doc:
  // "The first element corresponds to the lower left corner of the texture image",
  // (e.g (u,v) = (0,0))
  for(int y = 0; y < rect.size.height; ++y)
  {
    dst -= bpp * rect.size.width;
    memcpy(dst, src, bpp * rect.size.width);
    src += surface->pitch;
  }

  GLuint texture;

  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rect.size.width, rect.size.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.data());

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  SDL_FreeSurface(surface);

  return texture;
}

extern const Span<unsigned char> VertexShaderCode;
extern const Span<unsigned char> FragmentShaderCode;

static
GLuint loadShaders()
{
  auto const vertexId = compileShader(VertexShaderCode, GL_VERTEX_SHADER);
  auto const fragmentId = compileShader(FragmentShaderCode, GL_FRAGMENT_SHADER);

  auto const progId = linkShaders(vector<int>({ vertexId, fragmentId }));

  SAFE_GL(glDeleteShader(vertexId));
  SAFE_GL(glDeleteShader(fragmentId));

  return progId;
}

Model boxModel();

struct Camera
{
  Vector3f pos;
  Vector3f dir;
  bool valid = false;
};

static
void sendToOpengl(Model& model)
{
  SAFE_GL(glGenBuffers(1, &model.buffer));
  SAFE_GL(glBindBuffer(GL_ARRAY_BUFFER, model.buffer));
  SAFE_GL(glBufferData(GL_ARRAY_BUFFER, sizeof(model.vertices[0]) * model.vertices.size(), model.vertices.data(), GL_STATIC_DRAW));
  SAFE_GL(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

static
Model loadTiledAnimation(string path, int count, int COLS, int SIZE)
{
  auto m = boxModel();

  for(int i = 0; i < count; ++i)
  {
    auto col = i % COLS;
    auto row = i / COLS;

    Action action;
    action.addTexture(path, Rect2i(col * SIZE, row * SIZE, SIZE, SIZE));
    m.actions.push_back(action);
  }

  return m;
}

static
Model loadAnimation(string path)
{
  if(endsWith(path, ".json"))
  {
    return loadModel(path);
  }
  else if(endsWith(path, ".mdl"))
  {
    path = setExtension(path, "png");

    return loadTiledAnimation(path, 32, 4, 32);
  }
  else
  {
    auto m = boxModel();
    Action action;
    action.addTexture(path, Rect2i());
    m.actions.push_back(action);
    return m;
  }
}

static
void printOpenGlVersion()
{
  auto sVersion = (char const*)glGetString(GL_VERSION);
  auto sLangVersion = (char const*)glGetString(GL_SHADING_LANGUAGE_VERSION);

  auto notNull = [] (char const* s) -> char const*
    {
      return s ? s : "<null>";
    };

  printf("[display] OpenGL version: %s\n", notNull(sVersion));
  printf("[display] OpenGL shading version: %s\n", notNull(sLangVersion));
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
      throw runtime_error("Can't init SDL");

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    m_window = SDL_CreateWindow(
        "My Game",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        resolution.width, resolution.height,
        SDL_WINDOW_OPENGL
        );

    if(!m_window)
      throw runtime_error("Can't set video mode");

    // Create our opengl context and attach it to our window
    m_context = SDL_GL_CreateContext(m_window);

    if(!m_context)
      throw runtime_error("Can't create OpenGL context");

    printOpenGlVersion();

    // This makes our buffer swap syncronized with the monitor's vertical refresh
    SDL_GL_SetSwapInterval(1);

    GLuint VertexArrayID;
    SAFE_GL(glGenVertexArrays(1, &VertexArrayID));
    SAFE_GL(glBindVertexArray(VertexArrayID));

    m_programId = loadShaders();

    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_fontModel = loadTiledAnimation("res/font.png", 256, 16, 16);

    // don't GL_REPEAT fonts
    for(auto& action : m_fontModel.actions)
    {
      for(auto& texture : action.textures)
      {
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
      }
    }

    sendToOpengl(m_fontModel);

    m_MVP = glGetUniformLocation(m_programId, "MVP");
    assert(m_MVP >= 0);

    m_colorId = glGetUniformLocation(m_programId, "v_color");
    assert(m_colorId >= 0);

    m_ambientLoc = glGetUniformLocation(m_programId, "ambientLight");
    assert(m_ambientLoc >= 0);

    m_positionLoc = glGetAttribLocation(m_programId, "a_position");
    assert(m_positionLoc >= 0);

    m_textCoordLoc = glGetAttribLocation(m_programId, "a_texCoord");
    assert(m_textCoordLoc >= 0);

    m_normalLoc = glGetAttribLocation(m_programId, "a_normal");
    assert(m_normalLoc >= 0);
  }

  void loadModel(int id, const char* path) override
  {
    if((int)m_Models.size() <= id)
      m_Models.resize(id + 1);

    m_Models[id] = loadAnimation(path);
    sendToOpengl(m_Models[id]);
  }

  void setCamera(Vector3f pos, Vector3f dir) override
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

  void setFullscreen(bool fs) override
  {
    auto flags = fs ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
    SDL_SetWindowFullscreen(m_window, flags);
  }

  void setCaption(const char* caption) override
  {
    SDL_SetWindowTitle(m_window, caption);
  }

  void setAmbientLight(float ambientLight) override
  {
    m_ambientLight = ambientLight;
  }

  int m_blinkCounter = 0;

  void drawModel(Rect3f where, Camera const& camera, Model& model, bool blinking, int actionIdx, float ratio)
  {
    SAFE_GL(glUniform4f(m_colorId, 0, 0, 0, 0));

    if(blinking)
    {
      if((m_blinkCounter / 4) % 2)
        SAFE_GL(glUniform4f(m_colorId, 0.8, 0.4, 0.4, 0));
    }

    if(actionIdx < 0 || actionIdx >= (int)model.actions.size())
      throw runtime_error("invalid action index");

    auto const& action = model.actions[actionIdx];

    if(action.textures.empty())
      throw runtime_error("action has no textures");

    auto const N = (int)action.textures.size();
    auto const idx = ::clamp<int>(ratio * N, 0, N - 1);
    glBindTexture(GL_TEXTURE_2D, action.textures[idx]);

    auto const target = camera.pos + camera.dir;
    auto const view = ::lookAt(camera.pos, target, Vector3f(0, 0, 1));
    auto const pos = ::translate(where.pos);
    auto const scale = ::scale(Vector3f(where.size.cx, where.size.cy, where.size.cz));

    static const float fovy = (float)((60.0f / 180) * PI);
    static const float aspect = 1.0f;
    static const float near_ = 0.1f;
    static const float far_ = 100.0f;
    static const auto perspective = ::perspective(fovy, aspect, near_, far_);

    auto mat = perspective * view * pos * scale;

    SAFE_GL(glUniformMatrix4fv(m_MVP, 1, GL_FALSE, &mat[0][0]));

    SAFE_GL(glBindBuffer(GL_ARRAY_BUFFER, model.buffer));

    {
      // connect the xyz to the "a_position" attribute of the vertex shader
      SAFE_GL(glEnableVertexAttribArray(m_positionLoc));
      SAFE_GL(glVertexAttribPointer(m_positionLoc, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (const GLvoid*)(0 * sizeof(GLfloat))));

      // connect the N to the "a_normal" attribute of the vertex shader
      SAFE_GL(glEnableVertexAttribArray(m_normalLoc));
      SAFE_GL(glVertexAttribPointer(m_normalLoc, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (const GLvoid*)(5 * sizeof(GLfloat))));

      // connect the uv coords to the "v_texCoord" attribute of the vertex shader
      SAFE_GL(glEnableVertexAttribArray(m_textCoordLoc));
      SAFE_GL(glVertexAttribPointer(m_textCoordLoc, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat))));
    }
    SAFE_GL(glDrawArrays(GL_TRIANGLES, 0, model.vertices.size()));
  }

  void enableGrab(bool enable) override
  {
    SDL_SetRelativeMouseMode(enable ? SDL_TRUE : SDL_FALSE);
    SDL_SetWindowGrab(m_window, enable ? SDL_TRUE : SDL_FALSE);
    SDL_ShowCursor(enable ? 0 : 1);
  }

  void drawActor(Rect3f where, int modelId, bool blinking, int actionIdx, float ratio) override
  {
    auto& model = m_Models.at(modelId);
    drawModel(where, m_camera, model, blinking, actionIdx, ratio);
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
      drawModel(rect, cam, m_fontModel, false, *text, 0);
      rect.pos.x += rect.size.cx;
      ++text;
    }
  }

  void beginDraw() override
  {
    m_blinkCounter++;

    {
      int w, h;
      SDL_GL_GetDrawableSize(m_window, &w, &h);
      auto size = min(w, h);
      SAFE_GL(glViewport((w - size) / 2, (h - size) / 2, size, size));
    }

    SAFE_GL(glUseProgram(m_programId));

    glEnable(GL_DEPTH_TEST);
    SAFE_GL(glClearColor(0, 0, 0, 1));
    SAFE_GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    SAFE_GL(glUniform3f(m_ambientLoc, m_ambientLight, m_ambientLight, m_ambientLight));
  }

  void endDraw() override
  {
    SDL_GL_SwapWindow(m_window);
  }

  // end-of public API

  SDL_Window* m_window;
  SDL_GLContext m_context;

  Camera m_camera;

  GLint m_MVP;
  GLint m_colorId;
  GLint m_ambientLoc;
  GLint m_positionLoc;
  GLint m_textCoordLoc;
  GLint m_normalLoc;

  GLuint m_programId;
  vector<Model> m_Models;
  Model m_fontModel;

  float m_ambientLight = 0;
};

Display* createDisplay(Size2i resolution)
{
  return new OpenglDisplay(resolution);
}

