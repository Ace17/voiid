// OpenGL stuff

/*
 * Copyright (C) 2017 - Sebastien Alaiwan <sebastien.alaiwan@gmail.com>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

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

static GLint g_MVP;
static GLint g_colorId;
static GLint g_ambientLoc;

static GLuint g_ProgramId;
static vector<Model> g_Models;
static Model g_fontModel;

#ifdef NDEBUG
#define SAFE_GL(a) a
#else
#define SAFE_GL(a) \
  do { a; ensureGl(# a, __LINE__); } while(0)
#endif

inline
void ensureGl(char const* expr, int line)
{
  auto const errorCode = glGetError();

  if(errorCode == GL_NO_ERROR)
    return;

  stringstream ss;
  ss << "OpenGL error" << endl;
  ss << "Expr: " << expr << endl;
  ss << "Line: " << line << endl;
  ss << "Code: " << errorCode;
  throw runtime_error(ss.str());
}

static
int compileShader(string code, int type)
{
  auto shaderId = glCreateShader(type);

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

int loadTexture(string path, Rect2i rect)
{
  auto surface = loadPicture(path);

  if(rect.width == 0 && rect.height == 0)
    rect = Rect2i(0, 0, surface->w, surface->h);

  if(rect.x < 0 || rect.y < 0 || rect.x + rect.width > surface->w || rect.y + rect.height > surface->h)
    throw runtime_error("Invalid boundaries for '" + path + "'");

  GLuint texture;

  auto const bpp = surface->format->BytesPerPixel;

  vector<uint8_t> img(rect.width* rect.height* bpp);

  auto src = (Uint8*)surface->pixels + rect.x * bpp + rect.y * surface->pitch;
  auto dst = (Uint8*)img.data();

  for(int y = 0; y < rect.height; ++y)
  {
    memcpy(dst, src, bpp * rect.width);
    src += surface->pitch;
    dst += bpp * rect.width;
  }

  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rect.width, rect.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.data());

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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

static
Model cubeModel()
{
  const GLfloat myTriangle[] =
  {
    -0.5, -0.5, +0.5, /* uv */ 0, 1, /* N */ 0, 0, 1,
    -0.5, +0.5, +0.5, /* uv */ 0, 0, /* N */ 0, 0, 1,
    +0.5, +0.5, +0.5, /* uv */ 1, 0, /* N */ 0, 0, 1,
    +0.5, -0.5, +0.5, /* uv */ 1, 1, /* N */ 0, 0, 1,
    -0.5, -0.5, -0.5, /* uv */ 0, 1, /* N */ 0, 0, -1,
    +0.5, -0.5, -0.5, /* uv */ 0, 0, /* N */ 0, 0, -1,
    +0.5, +0.5, -0.5, /* uv */ 1, 0, /* N */ 0, 0, -1,
    -0.5, +0.5, -0.5, /* uv */ 1, 1, /* N */ 0, 0, -1,
    -0.5, -0.5, +0.5, /* uv */ 0, 1, /* N */ -1, 0, 0,
    -0.5, -0.5, -0.5, /* uv */ 0, 0, /* N */ -1, 0, 0,
    -0.5, +0.5, -0.5, /* uv */ 1, 0, /* N */ -1, 0, 0,
    -0.5, +0.5, +0.5, /* uv */ 1, 1, /* N */ -1, 0, 0,
    -0.5, +0.5, +0.5, /* uv */ 0, 1, /* N */ 0, 1, 0,
    -0.5, +0.5, -0.5, /* uv */ 0, 0, /* N */ 0, 1, 0,
    +0.5, +0.5, -0.5, /* uv */ 1, 0, /* N */ 0, 1, 0,
    +0.5, +0.5, +0.5, /* uv */ 1, 1, /* N */ 0, 1, 0,
    +0.5, +0.5, +0.5, /* uv */ 0, 1, /* N */ 1, 0, 0,
    +0.5, +0.5, -0.5, /* uv */ 0, 0, /* N */ 1, 0, 0,
    +0.5, -0.5, -0.5, /* uv */ 1, 0, /* N */ 1, 0, 0,
    +0.5, -0.5, +0.5, /* uv */ 1, 1, /* N */ 1, 0, 0,
    -0.5, -0.5, -0.5, /* uv */ 0, 1, /* N */ 0, -1, 0,
    -0.5, -0.5, +0.5, /* uv */ 0, 0, /* N */ 0, -1, 0,
    +0.5, -0.5, +0.5, /* uv */ 1, 0, /* N */ 0, -1, 0,
    +0.5, -0.5, -0.5, /* uv */ 1, 1, /* N */ 0, -1, 0,
  };

  const GLushort indices[] =
  {
    0, 2, 1,
    0, 3, 2,

    4, 6, 5,
    4, 7, 6,

    8, 10, 9,
    8, 11, 10,

    12, 14, 13,
    12, 15, 14,

    16, 18, 17,
    16, 19, 18,

    20, 22, 21,
    20, 23, 22,
  };

  Model model;

  model.size = 24;

  SAFE_GL(glGenBuffers(1, &model.buffer));
  SAFE_GL(glBindBuffer(GL_ARRAY_BUFFER, model.buffer));
  SAFE_GL(glBufferData(GL_ARRAY_BUFFER, sizeof(myTriangle), myTriangle, GL_STATIC_DRAW));

  SAFE_GL(glGenBuffers(1, &model.indices));
  SAFE_GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.indices));
  SAFE_GL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW));

  model.numIndices = sizeof(indices) / sizeof(*indices);

  return model;
}

static
Model loadTiledAnimation(string path, int count, int COLS, int SIZE)
{
  auto m = cubeModel();

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
    auto m = cubeModel();

    auto m2 = loadModel(path);
    m.actions = move(m2.actions);
    return m;
  }
  else if(endsWith(path, ".mdl"))
  {
    path = setExtension(path, "png");

    return loadTiledAnimation(path, 32, 4, 32);
  }
  else
  {
    auto m = cubeModel();
    Action action;
    action.addTexture(path, Rect2i());
    m.actions.push_back(action);
    return m;
  }
}

void Display_loadModel(int id, const char* path)
{
  if((int)g_Models.size() <= id)
    g_Models.resize(id + 1);

  g_Models[id] = loadAnimation(path);
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

static SDL_Window* mainWindow;
static SDL_GLContext mainContext;

void Display_init(int width, int height)
{
  if(SDL_InitSubSystem(SDL_INIT_VIDEO))
    throw runtime_error("Can't init SDL");

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  mainWindow = SDL_CreateWindow(
    "My Game",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    width, height,
    SDL_WINDOW_OPENGL
    );

  if(!mainWindow)
    throw runtime_error("Can't set video mode");

  // Create our opengl context and attach it to our window
  mainContext = SDL_GL_CreateContext(mainWindow);

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

  g_fontModel = loadTiledAnimation("res/font.png", 256, 16, 8);

  g_MVP = glGetUniformLocation(g_ProgramId, "MVP");
  assert(g_MVP >= 0);

  g_colorId = glGetUniformLocation(g_ProgramId, "v_color");
  assert(g_colorId >= 0);

  g_ambientLoc = glGetUniformLocation(g_ProgramId, "ambientLight");
  assert(g_ambientLoc >= 0);
}

struct Camera
{
  Vector3f pos;
  Vector3f dir;
  bool valid = false;
};

Camera g_camera;

float baseAmbientLight = 0;

void Display_setCamera(Vector3f pos, Vector3f dir)
{
  auto cam = (Camera { pos, dir });

  if(!g_camera.valid)
  {
    g_camera = cam;
    g_camera.valid = true;
  }

  auto blend = [] (Vector3f a, Vector3f b)
               {
                 auto const alpha = 0.3f;
                 return a * (1 - alpha) + b * alpha;
               };

  g_camera.pos = blend(g_camera.pos, cam.pos);
  g_camera.dir = cam.dir;
}

void Display_setCaption(const char* caption)
{
  SDL_SetWindowTitle(mainWindow, caption);
}

void Display_setAmbientLight(float ambientLight_)
{
  baseAmbientLight = ambientLight_;
}

void Display_enableGrab(bool enable)
{
  SDL_SetRelativeMouseMode(enable ? SDL_TRUE : SDL_FALSE);
  SDL_SetWindowGrab(mainWindow, enable ? SDL_TRUE : SDL_FALSE);
  SDL_ShowCursor(enable ? 0 : 1);
}

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtc/type_ptr.hpp"
using namespace glm;

static
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

  auto const idx = ::clamp<int>(ratio * action.textures.size(), 0, action.textures.size() - 1);
  glBindTexture(GL_TEXTURE_2D, action.textures[idx]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  auto Vec3 = [] (Vector3f v) { return vec3(v.x, v.y, v.z); };

  auto const target = camera.pos + camera.dir;
  auto const view = glm::lookAt(Vec3(camera.pos), Vec3(target), vec3(0, 0, 1));
  auto const pos = glm::translate(vec3(where.x, where.y, where.z));
  auto const scale = glm::scale(vec3(where.cx, where.cy, where.cz));

  static const float fovy = (float)((60.0f / 180) * M_PI);
  static const float aspect = 1.0f;
  static const float near_ = 0.1f;
  static const float far_ = 100.0f;
  static const auto perspective = glm::perspective(fovy, aspect, near_, far_);

  auto mat = perspective * view * pos * scale;

  SAFE_GL(glUniformMatrix4fv(g_MVP, 1, GL_FALSE, glm::value_ptr(mat)));

  SAFE_GL(glBindBuffer(GL_ARRAY_BUFFER, model.buffer));
  SAFE_GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.indices));

  SAFE_GL(glDrawElements(GL_TRIANGLES, model.numIndices, GL_UNSIGNED_SHORT, 0));
}

void Display_drawActor(Rect3f where, int modelId, bool blinking, int actionIdx, float ratio)
{
  auto& model = g_Models.at(modelId);
  drawModel(where, g_camera, model, blinking, actionIdx, ratio);
}

void Display_drawText(Vector2f pos, char const* text)
{
  Rect3f rect;
  rect.cx = 0.5;
  rect.cy = 0;
  rect.cz = 0.5;
  rect.x = pos.x - strlen(text) * rect.cx / 2;
  rect.y = 0;
  rect.z = pos.y;

  auto cam = (Camera { Vector3f(0, -10, 0), Vector3f(0, 1, 0) });

  glDisable(GL_DEPTH_TEST);

  while(*text)
  {
    drawModel(rect, cam, g_fontModel, false, *text, 0);
    rect.x += rect.cx;
    ++text;
  }
}

void Display_beginDraw()
{
  SAFE_GL(glUseProgram(g_ProgramId));

  glEnable(GL_DEPTH_TEST);
  SAFE_GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
  SAFE_GL(glClearColor(0, 0, 0, 1));
  SAFE_GL(glClear(GL_COLOR_BUFFER_BIT));

  SAFE_GL(glUniform3f(g_ambientLoc, 1, 1, 1));

  {
    auto const positionLoc = glGetAttribLocation(g_ProgramId, "a_position");
    auto const texCoordLoc = glGetAttribLocation(g_ProgramId, "a_texCoord");
    auto const normalLoc = glGetAttribLocation(g_ProgramId, "a_normal");

    assert(positionLoc >= 0);
    assert(texCoordLoc >= 0);
    assert(normalLoc >= 0);

    // connect the xyz to the "a_position" attribute of the vertex shader
    SAFE_GL(glEnableVertexAttribArray(positionLoc));
    SAFE_GL(glVertexAttribPointer(positionLoc, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), nullptr));

    // connect the N to the "a_normal" attribute of the vertex shader
    SAFE_GL(glEnableVertexAttribArray(normalLoc));
    SAFE_GL(glVertexAttribPointer(normalLoc, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (const GLvoid*)(5 * sizeof(GLfloat))));

    // connect the uv coords to the "v_texCoord" attribute of the vertex shader
    SAFE_GL(glEnableVertexAttribArray(texCoordLoc));
    SAFE_GL(glVertexAttribPointer(texCoordLoc, 2, GL_FLOAT, GL_TRUE, 8 * sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat))));
  }
}

void Display_endDraw()
{
  SDL_GL_SwapWindow(mainWindow);
}

