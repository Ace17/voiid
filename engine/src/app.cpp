// Copyright (C) 2018 - Sebastien Alaiwan <sebastien.alaiwan@gmail.com>
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Main loop timing.
// No game-specific code should be here,
// and no platform-specific code should be here (SDL is OK).

#include <vector>
#include <string>
#include <memory>

#include "SDL.h"

#include "base/geom.h"
#include "base/resource.h"
#include "base/scene.h"
#include "base/view.h"
#include "ratecounter.h"
#include "audio.h"
#include "display.h"

using namespace std;

auto const TIMESTEP = 1;

Display* createDisplay(Size2i resolution);
Audio* createAudio();

Scene* createGame(View* view, vector<string> argv);

class App : View
{
public:
  App(vector<string> argv)
    :
    m_args(argv),
    m_scene(createGame(this, m_args))
  {
    SDL_Init(0);

    m_display.reset(createDisplay(Size2i(1024, 1024)));
    m_audio.reset(createAudio());

    for(auto res : getResources())
    {
      switch(res.type)
      {
      case ResourceType::Sound:
        m_audio->loadSound(res.id, res.path);
        break;
      case ResourceType::Model:
        m_display->loadModel(res.id, res.path);
        break;
      }
    }

    m_display->enableGrab(m_doGrab);

    m_lastTime = SDL_GetTicks();
  }

  virtual ~App()
  {
    SDL_Quit();
  }

  bool tick()
  {
    processInput();

    auto const now = (int)SDL_GetTicks();
    bool dirty = false;

    while(m_lastTime < now)
    {
      m_lastTime += m_slowMotion ? TIMESTEP * 10 : TIMESTEP;

      if(!m_paused)
        m_scene->tick(m_control);

      dirty = true;
    }

    if(dirty)
    {
      draw();
      m_fps.tick(now);
    }

    auto fps = m_fps.slope();

    if(fps != m_lastFps)
    {
      fpsChanged(fps);
      m_lastFps = fps;
    }

    return m_running;
  }

private:
  void processInput()
  {
    SDL_Event event;

    while(SDL_PollEvent(&event))
    {
      switch(event.type)
      {
      case SDL_MOUSEBUTTONDOWN:
        m_doGrab = true;
        m_display->enableGrab(m_doGrab);
        break;
      case SDL_MOUSEMOTION:

        if(m_doGrab)
          onMouseMotion(&event);

        break;
      case SDL_QUIT:
        onQuit();
        break;
      case SDL_KEYDOWN:
        onKeyDown(&event);
        break;
      case SDL_KEYUP:
        onKeyUp(&event);
        break;
      }
    }

    m_control.left = keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A];
    m_control.right = keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D];
    m_control.forward = keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W];
    m_control.backward = keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S];

    m_control.use = keys[SDL_SCANCODE_E];
    m_control.jump = keys[SDL_SCANCODE_SPACE];

    m_control.restart = keys[SDL_SCANCODE_R];
    m_control.debug = keys[SDL_SCANCODE_SCROLLLOCK];
  }

  void draw()
  {
    m_display->setAmbientLight(m_scene->ambientLight);

    m_display->beginDraw();

    auto actors = m_scene->getActors();

    for(auto& actor : actors)
    {
      if(actor.focus)
      {
        auto const size = Vector3f(actor.scale.cx, actor.scale.cy, actor.scale.cz);
        m_display->setCamera(actor.pos + size * 0.5, actor.orientation);
        break;
      }
    }

    for(auto& actor : actors)
    {
      auto where = Rect3f(
          actor.pos.x, actor.pos.y, actor.pos.z,
          actor.scale.cx, actor.scale.cy, actor.scale.cz);
      m_display->drawActor(where, (int)actor.model, actor.effect == Effect::Blinking, actor.action, actor.ratio);
    }

    if(m_paused)
      m_display->drawText(Vector2f(0, 0), "PAUSE");
    else if(m_slowMotion)
      m_display->drawText(Vector2f(0, 0), "SLOW-MOTION MODE");
    else if(m_control.debug)
      m_display->drawText(Vector2f(0, 0), "DEBUG MODE");

    if(m_textboxDelay > 0)
    {
      m_display->drawText(Vector2f(0, 2), m_textbox.c_str());
      m_textboxDelay--;
    }

    m_display->endDraw();
  }

  void fpsChanged(int fps)
  {
    char title[128];
    sprintf(title, "Voiid (%d FPS)", fps);
    m_display->setCaption(title);
  }

  void onQuit()
  {
    m_running = 0;
  }

  void onMouseMotion(SDL_Event* evt)
  {
    auto const speed = 0.001;

    m_control.look_horz -= evt->motion.xrel * speed;
    m_control.look_vert -= evt->motion.yrel * speed;

    m_control.look_vert = clamp<float>(m_control.look_vert, -PI * 0.4, PI * 0.4);
  }

  void onKeyDown(SDL_Event* evt)
  {
    if(evt->key.keysym.sym == SDLK_ESCAPE)
      onQuit();

    if(evt->key.keysym.sym == SDLK_F2)
      m_scene.reset(createGame(this, m_args));

    if(evt->key.keysym.sym == SDLK_TAB)
      m_slowMotion = !m_slowMotion;

    if(evt->key.keysym.sym == SDLK_PAUSE)
    {
      m_audio->playSound(0);
      m_paused = !m_paused;
    }

    if(evt->key.keysym.scancode == SDL_SCANCODE_RCTRL)
    {
      m_doGrab = !m_doGrab;
      m_display->enableGrab(m_doGrab);
    }

    keys[evt->key.keysym.scancode] = 1;
  }

  void onKeyUp(SDL_Event* evt)
  {
    keys[evt->key.keysym.scancode] = 0;
  }

  // View implementation
  void textBox(char const* msg) override
  {
    m_textbox = msg;
    m_textboxDelay = 60 * 2;
  }

  void playMusic(int id) override
  {
    m_audio->playMusic(id);
  }

  void playSound(int sound) override
  {
    m_audio->playSound(sound);
  }

  int keys[SDL_NUM_SCANCODES] {};
  int m_running = 1;

  int m_lastTime;
  int m_lastFps = 0;
  RateCounter m_fps;
  Control m_control;
  vector<string> m_args;
  unique_ptr<Scene> m_scene;
  bool m_slowMotion = false;
  bool m_paused = false;
  bool m_doGrab = true;
  unique_ptr<Audio> m_audio;
  unique_ptr<Display> m_display;

  string m_textbox;
  int m_textboxDelay = 0;
};

///////////////////////////////////////////////////////////////////////////////

App* App_create(vector<string> argv)
{
  return new App(argv);
}

void App_destroy(App* app)
{
  delete app;
}

bool App_tick(App* app)
{
  return app->tick();
}

