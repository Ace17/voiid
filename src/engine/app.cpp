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
#include "misc/file.h"

#include "display.h"
#include "input.h"
#include "ratecounter.h"

using namespace std;

auto const TIMESTEP = 10;
auto const RESOLUTION = Size2i(1280, 720);

Display* createDisplay(Size2i resolution);
Audio* createAudio();
UserInput* createUserInput();

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
    m_input.reset(createUserInput());

    m_scene.reset(createGame(this, m_args));

    m_display->enableGrab(m_doGrab);

    m_lastTime = SDL_GetTicks();
    m_lastDisplayFrameTime = SDL_GetTicks();

    registerUserInputActions();
  }

  virtual ~App()
  {
    SDL_Quit();
  }

  bool tick() override
  {
    m_input->process();

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

      if(!m_paused && m_running == 1)
        tickGameplay();
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
        File::write("screenshot.rgba", pixels);
        fprintf(stderr, "Saved screenshot to 'screenshot.rgba'\n");

        m_mustScreenshot = false;
      }
    }
  }

  void tickGameplay()
  {
    m_control.debug = m_debugMode;

    auto next = m_scene->tick(m_control);
    m_control.look_horz = 0;
    m_control.look_vert = 0;

    if(next != m_scene.get())
      m_scene.reset(next);
  }

  void registerUserInputActions()
  {
    // App keys
    m_input->listenToQuit([&] () { m_running = 0; });

    m_input->listenToKey(Key::RightControl, [&] (bool isDown) { if(isDown) toggleGrab(); }, true);
    m_input->listenToKey(Key::PrintScreen, [&] (bool isDown) { if(isDown) toggleVideoCapture(); }, true);
    m_input->listenToKey(Key::PrintScreen, [&] (bool isDown) { if(isDown) m_mustScreenshot = true; }, false);
    m_input->listenToKey(Key::Return, [&] (bool isDown) { if(isDown) toggleFullScreen(); }, false, true);

    m_input->listenToKey(Key::F3, [&] (bool isDown) { if(isDown) toggleFsaa(); });
    m_input->listenToKey(Key::F4, [&] (bool isDown) { if(isDown) toggleHdr(); });

    m_input->listenToKey(Key::Y, [&] (bool isDown) { if(isDown && m_running == 2) m_running = 0; });
    m_input->listenToKey(Key::N, [&] (bool isDown) { if(isDown && m_running == 2) m_running = 1; });

    // Player keys
    m_input->listenToKey(Key::Esc, [&] (bool isDown) { if(isDown) onQuit(); });

    m_input->listenToMouseMove([this] (int dx, int dy) { onMouseMotion(dx, dy); });
    m_input->listenToMouseClick([this] (int, int) { onMouseClick(); });

    m_input->listenToKey(Key::Left, [&] (bool isDown) { m_control.left = isDown; });
    m_input->listenToKey(Key::Right, [&] (bool isDown) { m_control.right = isDown; });
    m_input->listenToKey(Key::Up, [&] (bool isDown) { m_control.forward = isDown; });
    m_input->listenToKey(Key::Down, [&] (bool isDown) { m_control.backward = isDown; });

    m_input->listenToKey(Key::A, [&] (bool isDown) { m_control.left = isDown; });
    m_input->listenToKey(Key::D, [&] (bool isDown) { m_control.right = isDown; });
    m_input->listenToKey(Key::W, [&] (bool isDown) { m_control.forward = isDown; });
    m_input->listenToKey(Key::S, [&] (bool isDown) { m_control.backward = isDown; });

    m_input->listenToKey(Key::E, [&] (bool isDown) { m_control.use = isDown; });
    m_input->listenToKey(Key::Space, [&] (bool isDown) { m_control.jump = isDown; });

    m_input->listenToKey(Key::R, [&] (bool isDown) { m_control.restart = isDown; });

    // Debug keys
    m_input->listenToKey(Key::F2, [&] (bool isDown) { if(isDown) m_scene.reset(createGame(this, m_args)); });
    m_input->listenToKey(Key::Tab, [&] (bool isDown) { if(isDown) m_slowMotion = !m_slowMotion; });
    m_input->listenToKey(Key::ScrollLock, [&] (bool isDown) { if(isDown) m_debugMode = !m_debugMode; });
    m_input->listenToKey(Key::Pause, [&] (bool isDown) { if(isDown){ m_audio->playSound(0); m_paused = !m_paused; } });
  }

  void draw()
  {
    m_display->beginDraw();

    for(auto& actor : m_actors)
    {
      auto where = Rect3f(
        actor.pos.x, actor.pos.y, actor.pos.z,
        actor.scale.cx, actor.scale.cy, actor.scale.cz);
      m_display->drawActor(where, actor.orientation, (int)actor.model, actor.effect == Effect::Blinking, actor.action, actor.ratio);
    }

    if(m_running == 2)
      m_display->drawText(Vector2f(0, 0), "QUIT? [Y/N]");
    else if(m_paused)
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
    if(m_running == 2)
      m_running = 1;
    else
      m_running = 2;
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

  void onMouseClick()
  {
    m_doGrab = true;
    m_display->enableGrab(m_doGrab);
  }

  void onMouseMotion(int dx, int dy)
  {
    if(!m_doGrab)
      return;

    auto const speed = 0.001;

    m_control.look_horz += dx * speed;
    m_control.look_vert += dy * speed;
  }

  void toggleDebug()
  {
    m_debugMode = !m_debugMode;
  }

  void togglePause()
  {
    m_audio->playSound(0);
    m_paused = !m_paused;
  }

  void toggleGrab()
  {
    m_doGrab = !m_doGrab;
    m_display->enableGrab(m_doGrab);
  }

  void toggleFsaa()
  {
    m_enableFsaa = !m_enableFsaa;
    m_display->setFsaa(m_enableFsaa);
  }

  void toggleHdr()
  {
    m_enableHdr = !m_enableHdr;
    m_display->setHdr(m_enableHdr);
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

  void setCameraPos(Vector3f pos, Quaternion orientation) override
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

  int m_running = 1;
  int m_fixedDisplayFramePeriod = 0;
  FILE* m_captureFile = nullptr;
  bool m_mustScreenshot = false;

  bool m_debugMode = false;
  bool m_enableHdr = true;
  bool m_enableFsaa = false;

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
  unique_ptr<UserInput> m_input;

  string m_textbox;
  int m_textboxDelay = 0;
};

///////////////////////////////////////////////////////////////////////////////

unique_ptr<IApp> createApp(Span<char*> args)
{
  return make_unique<App>(args);
}

