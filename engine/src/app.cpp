// Copyright (C) 2018 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Main loop timing.
// No game-specific code should be here,
// and no platform-specific code should be here (SDL is OK).

#include "app.h"

#include <memory>
#include <string>
#include <vector>

#include "SDL.h"

#include "audio/audio.h"
#include "base/geom.h"
#include "base/resource.h"
#include "base/scene.h"
#include "base/util.h" // clamp
#include "base/view.h"
#include "render/display.h"

#include "ratecounter.h"

using namespace std;

auto const TIMESTEP = 10;
auto const RESOLUTION = Size2i(1024, 1024);

Display* createDisplay(Size2i resolution);
Audio* createAudio();

Scene* createGame(View* view, vector<string> argv);

class App : View, public IApp
{
public:
  App(Span<char*> args)
    : m_args({ args.data, args.data + args.len })
  {
    SDL_Init(0);

    m_display.reset(createDisplay(RESOLUTION));
    m_audio.reset(createAudio());

    m_scene.reset(createGame(this, m_args));

    m_display->enableGrab(m_doGrab);

    m_lastTime = SDL_GetTicks();
    m_lastDisplayFrameTime = SDL_GetTicks();
  }

  virtual ~App()
  {
    SDL_Quit();
  }

  bool tick() override
  {
    processInput();

    auto const now = (int)SDL_GetTicks();

    if(m_fixedDisplayFramePeriod)
    {
      while(m_lastDisplayFrameTime + m_fixedDisplayFramePeriod < now)
      {
        m_lastDisplayFrameTime += m_fixedDisplayFramePeriod;
        tickOneDisplayFrame(m_lastDisplayFrameTime);
      }
    }
    else
    {
      m_lastDisplayFrameTime = now;
      tickOneDisplayFrame(now);
    }

    return m_running;
  }

private:
  void tickOneDisplayFrame(int now)
  {
    auto timestep = m_slowMotion ? TIMESTEP * 10 : TIMESTEP;

    while(m_lastTime + timestep < now)
    {
      m_lastTime += timestep;

      if(!m_paused)
      {
        m_scene->tick(m_control);
      }
    }

    // draw the frame
    m_actors.clear();
    m_scene->draw();
    draw();

    m_fps.tick(now);

    captureDisplayFrameIfNeeded();
  }

  void captureDisplayFrameIfNeeded()
  {
    if(m_captureFile || m_mustScreenshot)
    {
      vector<uint8_t> pixels(RESOLUTION.width * RESOLUTION.height * 4);
      m_display->readPixels({ pixels.data(), (int)pixels.size() });

      if(m_captureFile)
        fwrite(pixels.data(), 1, pixels.size(), m_captureFile);

      if(m_mustScreenshot)
      {
        FILE* fp = fopen("screenshot.rgba", "wb");
        fwrite(pixels.data(), 1, pixels.size(), fp);
        fflush(fp);
        fclose(fp);
        fprintf(stderr, "Saved screenshot to 'screenshot.rgba'\n");

        m_mustScreenshot = false;
      }
    }
  }

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

    m_control.debug = m_debugMode;
  }

  void draw()
  {
    m_display->beginDraw();

    for(auto& actor : m_actors)
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

    if(m_debugMode)
    {
      char debugText[256];
      sprintf(debugText, "FPS: %d", m_fps.slope());
      m_display->drawText(Vector2f(0, -4), debugText);
    }

    if(m_textboxDelay > 0)
    {
      m_display->drawText(Vector2f(0, 2), m_textbox.c_str());
      m_textboxDelay--;
    }

    m_display->endDraw();
  }

  void onQuit()
  {
    m_running = 0;
  }

  void toggleVideoCapture()
  {
    if(!m_captureFile)
    {
      if(m_fullscreen)
      {
        fprintf(stderr, "Can't capture video in fullscreen mode\n");
        return;
      }

      m_captureFile = fopen("capture.rgba", "wb");

      if(!m_captureFile)
      {
        fprintf(stderr, "Can't start video capture!\n");
        return;
      }

      m_fixedDisplayFramePeriod = 40;
      fprintf(stderr, "Capturing video at %d Hz...\n", 1000 / m_fixedDisplayFramePeriod);
    }
    else
    {
      fprintf(stderr, "Stopped video capture\n");
      fclose(m_captureFile);
      m_captureFile = nullptr;
      m_fixedDisplayFramePeriod = 0;
    }
  }

  void toggleFullScreen()
  {
    if(m_captureFile)
    {
      fprintf(stderr, "Can't toggle full-screen during video capture\n");
      return;
    }

    m_fullscreen = !m_fullscreen;
    m_display->setFullscreen(m_fullscreen);
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
    keys[evt->key.keysym.scancode] = 1;

    if(evt->key.repeat > 0)
      return;

    switch(evt->key.keysym.sym)
    {
    case SDLK_ESCAPE:
      {
        onQuit();
        break;
      }

    case SDLK_F2:
      {
        m_scene.reset(createGame(this, m_args));
        break;
      }

    case SDLK_TAB:
      {
        m_slowMotion = !m_slowMotion;
        break;
      }

    case SDLK_PRINTSCREEN:
      {
        if(evt->key.keysym.mod & KMOD_CTRL)
        {
          toggleVideoCapture();
        }
        else
        {
          m_mustScreenshot = true;
        }

        break;
      }

    case SDLK_RETURN:
      {
        if(evt->key.keysym.mod & KMOD_LALT)
          toggleFullScreen();

        break;
      }

    case SDLK_SCROLLLOCK:
      {
        m_debugMode = !m_debugMode;

        break;
      }

    case SDLK_PAUSE:
      {
        m_audio->playSound(0);
        m_paused = !m_paused;
        break;
      }

    case SDLK_RCTRL:
      {
        m_doGrab = !m_doGrab;
        m_display->enableGrab(m_doGrab);
        break;
      }
    }
  }

  void onKeyUp(SDL_Event* evt)
  {
    keys[evt->key.keysym.scancode] = 0;
  }

  // View implementation
  void setTitle(char const* gameTitle) override
  {
    m_display->setCaption(gameTitle);
  }

  void preload(Resource res) override
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

  void textBox(char const* msg) override
  {
    m_textbox = msg;
    m_textboxDelay = 60 * 2;
  }

  void playMusic(int id) override
  {
    m_audio->playMusic(id);
  }

  void stopMusic() override
  {
    m_audio->stopMusic();
  }

  void playSound(int sound) override
  {
    m_audio->playSound(sound);
  }

  void setCameraPos(Vector3f pos, Quaternion orientation)
  {
    m_display->setCamera(pos, orientation);
  }

  void setAmbientLight(float amount) override
  {
    m_display->setAmbientLight(amount);
  }

  void sendActor(Actor const& actor) override
  {
    m_actors.push_back(actor);
  }

  int keys[SDL_NUM_SCANCODES] {};
  int m_running = 1;
  int m_fixedDisplayFramePeriod = 0;
  FILE* m_captureFile = nullptr;
  bool m_mustScreenshot = false;

  bool m_debugMode = false;

  int m_lastTime;
  int m_lastDisplayFrameTime;
  RateCounter m_fps;
  Control m_control {};
  vector<string> m_args;
  unique_ptr<Scene> m_scene;
  bool m_slowMotion = false;
  bool m_fullscreen = false;
  bool m_paused = false;
  bool m_doGrab = true;
  unique_ptr<Audio> m_audio;
  unique_ptr<Display> m_display;
  vector<Actor> m_actors;

  string m_textbox;
  int m_textboxDelay = 0;
};

///////////////////////////////////////////////////////////////////////////////

unique_ptr<IApp> createApp(Span<char*> args)
{
  return make_unique<App>(args);
}

