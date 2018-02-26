/*
 * Copyright (C) 2017 - Sebastien Alaiwan <sebastien.alaiwan@gmail.com>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

// OpenGL stuff

#include "display.h"

#include <cassert>
#include <sstream>
#include <vector>
#include <iostream>
#include <stdexcept>
using namespace std;

#define GL_GLEXT_PROTOTYPES 1
#include "GL/gl.h"
#include "SDL_video.h"
#include "SDL_image.h"

#include "base/util.h"
#include "base/scene.h"
#include "base/geom.h"
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
int compileShader(string code, int type)
{
  auto shaderId = glCreateShader(type);

  if(!shaderId)
    throw runtime_error("Can't create shader");

  cout << "Compiling " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader ... ";
  auto srcPtr = code.c_str();
  SAFE_GL(glShaderSource(shaderId, 1, &srcPtr, nullptr));
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
    cerr << msg.data();

    throw runtime_error("Can't compile shader");
  }

  cout << "OK" << endl;

  return shaderId;
}

static
int linkShaders(vector<int> ids)
{
  // Link the program
  cout << "Linking shaders ... ";
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
    cout << msg.data();

    throw runtime_error("Can't link shader");
  }

  cout << "OK" << endl;

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
  // "The first element corresponds to the lower left corner of the texture image"
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

extern char VertexShaderCode[];
extern size_t VertexShaderCode_size;
extern char FragmentShaderCode[];
extern size_t FragmentShaderCode_size;

static
GLuint loadShaders()
{
  auto const vsCode = string(VertexShaderCode, VertexShaderCode + VertexShaderCode_size);
  auto const fsCode = string(FragmentShaderCode, FragmentShaderCode + FragmentShaderCode_size);
  auto const vertexId = compileShader(vsCode, GL_VERTEX_SHADER);
  auto const fragmentId = compileShader(fsCode, GL_FRAGMENT_SHADER);

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

  auto notNull = [] (char const* s) -> string
    {
      return s ? s : "<null>";
    };

  cout << "OpenGL version: " << notNull(sVersion) << endl;
  cout << "OpenGL shading version: " << notNull(sLangVersion) << endl;
}

struct SdlDisplay : Display
{
  void init(Size2i resolution) override
  {
    if(SDL_InitSubSystem(SDL_INIT_VIDEO))
      throw runtime_error("Can't init SDL");

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    mainWindow = SDL_CreateWindow(
        "My Game",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        resolution.width, resolution.height,
        SDL_WINDOW_OPENGL
        );

    if(!mainWindow)
      throw runtime_error("Can't set video mode");

    // Create our opengl context and attach it to our window
    mainContext = SDL_GL_CreateContext(mainWindow);

    if(!mainContext)
      throw runtime_error("Can't create OpenGL context");

    printOpenGlVersion();

    // This makes our buffer swap syncronized with the monitor's vertical refresh
    SDL_GL_SetSwapInterval(1);

    GLuint VertexArrayID;
    SAFE_GL(glGenVertexArrays(1, &VertexArrayID));
    SAFE_GL(glBindVertexArray(VertexArrayID));

    g_ProgramId = loadShaders();

    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    g_fontModel = loadTiledAnimation("res/font.png", 256, 16, 16);

    // don't GL_REPEAT fonts
    for(auto& action : g_fontModel.actions)
    {
      for(auto& texture : action.textures)
      {
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
      }
    }

    sendToOpengl(g_fontModel);

    g_MVP = glGetUniformLocation(g_ProgramId, "MVP");
    assert(g_MVP >= 0);

    g_colorId = glGetUniformLocation(g_ProgramId, "v_color");
    assert(g_colorId >= 0);

    g_ambientLoc = glGetUniformLocation(g_ProgramId, "ambientLight");
    assert(g_ambientLoc >= 0);

    g_positionLoc = glGetAttribLocation(g_ProgramId, "a_position");
    assert(g_positionLoc >= 0);

    g_texCoordLoc = glGetAttribLocation(g_ProgramId, "a_texCoord");
    assert(g_texCoordLoc >= 0);

    g_normalLoc = glGetAttribLocation(g_ProgramId, "a_normal");
    assert(g_normalLoc >= 0);
  }

  void loadModel(int id, const char* path) override
  {
    if((int)g_Models.size() <= id)
      g_Models.resize(id + 1);

    g_Models[id] = loadAnimation(path);
    sendToOpengl(g_Models[id]);
  }

  void setCamera(Vector3f pos, Vector3f dir) override
  {
    auto cam = (Camera { pos, dir });

    if(!g_camera.valid)
    {
      g_camera = cam;
      g_camera.valid = true;
    }

    // avoid big camera jumps
    {
      auto delta = g_camera.pos - pos;

      if(dotProduct(delta, delta) > 10)
        g_camera = cam;
    }

    auto blend = [] (Vector3f a, Vector3f b)
      {
        auto const alpha = 0.3f;
        return a * (1 - alpha) + b * alpha;
      };

    g_camera.pos = blend(g_camera.pos, cam.pos);
    g_camera.dir = cam.dir;
  }

  void setCaption(const char* caption) override
  {
    SDL_SetWindowTitle(mainWindow, caption);
  }

  void setAmbientLight(float ambientLight_) override
  {
    baseAmbientLight = ambientLight_;
  }

  void drawModel(Rect3f where, Camera const& camera, Model& model, bool blinking, int actionIdx, float ratio)
  {
    SAFE_GL(glUniform4f(g_colorId, 0, 0, 0, 0));

    if(blinking)
    {
      static int blinkCounter;
      blinkCounter++;

      if((blinkCounter / 4) % 2)
        SAFE_GL(glUniform4f(g_colorId, 0.8, 0.4, 0.4, 0));
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

    SAFE_GL(glUniformMatrix4fv(g_MVP, 1, GL_FALSE, &mat[0][0]));

    SAFE_GL(glBindBuffer(GL_ARRAY_BUFFER, model.buffer));

    {
      // connect the xyz to the "a_position" attribute of the vertex shader
      SAFE_GL(glEnableVertexAttribArray(g_positionLoc));
      SAFE_GL(glVertexAttribPointer(g_positionLoc, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (const GLvoid*)(0 * sizeof(GLfloat))));

      // connect the N to the "a_normal" attribute of the vertex shader
      SAFE_GL(glEnableVertexAttribArray(g_normalLoc));
      SAFE_GL(glVertexAttribPointer(g_normalLoc, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (const GLvoid*)(5 * sizeof(GLfloat))));

      // connect the uv coords to the "v_texCoord" attribute of the vertex shader
      SAFE_GL(glEnableVertexAttribArray(g_texCoordLoc));
      SAFE_GL(glVertexAttribPointer(g_texCoordLoc, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat))));
    }
    SAFE_GL(glDrawArrays(GL_TRIANGLES, 0, model.vertices.size()));
  }

  void enableGrab(bool enable) override
  {
    SDL_SetRelativeMouseMode(enable ? SDL_TRUE : SDL_FALSE);
    SDL_SetWindowGrab(mainWindow, enable ? SDL_TRUE : SDL_FALSE);
    SDL_ShowCursor(enable ? 0 : 1);
  }

  void drawActor(Rect3f where, int modelId, bool blinking, int actionIdx, float ratio) override
  {
    auto& model = g_Models.at(modelId);
    drawModel(where, g_camera, model, blinking, actionIdx, ratio);
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
      drawModel(rect, cam, g_fontModel, false, *text, 0);
      rect.pos.x += rect.size.cx;
      ++text;
    }
  }

  void beginDraw() override
  {
    SAFE_GL(glUseProgram(g_ProgramId));

    glEnable(GL_DEPTH_TEST);
    SAFE_GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    SAFE_GL(glClearColor(0, 0, 0, 1));
    SAFE_GL(glClear(GL_COLOR_BUFFER_BIT));

    SAFE_GL(glUniform3f(g_ambientLoc, baseAmbientLight, baseAmbientLight, baseAmbientLight));
  }

  void endDraw() override
  {
    SDL_GL_SwapWindow(mainWindow);
  }

  // end-of public API

  SDL_Window* mainWindow;
  SDL_GLContext mainContext;

  Camera g_camera;

  GLint g_MVP;
  GLint g_colorId;
  GLint g_ambientLoc;
  GLint g_positionLoc;
  GLint g_texCoordLoc;
  GLint g_normalLoc;

  GLuint g_ProgramId;
  vector<Model> g_Models;
  Model g_fontModel;

  float baseAmbientLight = 0;
};

Display* createDisplay()
{
  return new SdlDisplay;
}

