// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Main loop timing.
// No game-specific code should be here,
// and no platform-specific code should be here.

#include "app.h"

#include <memory>
#include <string>
#include <vector>

#include "base/geom.h"
#include "base/logger.h"
#include "base/resource.h"
#include "base/scene.h"
#include "base/view.h"
#include "misc/file.h"
#include "misc/time.h"

#include "audio.h"
#include "audio_backend.h"
#include "graphics_backend.h"
#include "input.h"
#include "ratecounter.h"
#include "renderer.h"
#include "stats.h"
#include "video_capture.h"

auto const TIMESTEP = 10;
auto const RESOLUTION = Vec2i(1280, 720);
auto const CAPTURE_FRAME_PERIOD = 40;

IGraphicsBackend* createGraphicsBackend(Vec2i resolution);
IRenderer* createRenderer(IGraphicsBackend* backend);
MixableAudio* createAudio();
UserInput* createUserInput();

// Implemented by the game-specific part
Scene* createGame(View* view, Span<const string> argv);

class App : View, public IApp
{
public:
  App(Span<char*> args)
    : m_args({ args.data, args.data + args.len })
  {
    m_graphicsBackend.reset(createGraphicsBackend(RESOLUTION));
    m_renderer.reset(createRenderer(m_graphicsBackend.get()));
    m_audio.reset(createAudio());
    m_audioBackend.reset(createAudioBackend(m_audio.get()));
    m_input.reset(createUserInput());

    m_scene.reset(createGame(this, m_args));

    m_graphicsBackend->enableGrab(m_doGrab);

    m_lastTime = GetSteadyClockMs();
    m_lastDisplayFrameTime = GetSteadyClockMs();

    registerUserInputActions();
  }

  bool tick() override
  {
    m_input->process();

    auto const now = (int)GetSteadyClockMs();

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

    return m_running != AppState::Exit;
  }

private:
  void tickOneDisplayFrame(int now)
  {
    const auto timeStep = m_slowMotion ? TIMESTEP * 10 : TIMESTEP;

    int ticksPerFrame = 0;

    while(m_lastTime + timeStep < now)
    {
      m_lastTime += timeStep;
      ++ticksPerFrame;
    }

    for(int k = 0; k < ticksPerFrame; ++k)
    {
      if(!m_paused && m_running == AppState::Running)
      {
        tickGameplay();
        m_tps.tick(now);
        Stat("TPS", m_tps.slope());
      }
    }

    Stat("Ticks/Frame", ticksPerFrame);

    // draw the frame
    m_actors.clear();
    m_lightActors.clear();
    m_scene->draw();
    draw();

    m_fps.tick(now);
    Stat("FPS", m_fps.slope());
  }

  void tickGameplay()
  {
    m_control.debug = m_debugMode;

    auto const t0 = GetSteadyClockUs();

    auto next = m_scene->tick(m_control);
    m_control.look_horz = 0;
    m_control.look_vert = 0;

    if(next != m_scene.get())
      m_scene.reset(next);

    auto const t1 = GetSteadyClockUs();
    Stat("Tick duration", (t1 - t0) / 1000.0f);
  }

  void registerUserInputActions()
  {
    // App keys
    m_input->listenToQuit([&] () { m_running = AppState::Exit; });

    m_input->listenToKey(Key::RightControl, [&] (bool isDown) { if(isDown) toggleGrab(); }, true);
    m_input->listenToKey(Key::PrintScreen, [&] (bool isDown) { if(isDown) toggleVideoCapture(); }, true);
    m_input->listenToKey(Key::PrintScreen, [&] (bool isDown) { if(isDown) m_recorder.takeScreenshot(); }, false);
    m_input->listenToKey(Key::Return, [&] (bool isDown) { if(isDown) toggleFullScreen(); }, false, true);

    m_input->listenToKey(Key::F3, [&] (bool isDown) { if(isDown) toggleFsaa(); });
    m_input->listenToKey(Key::F4, [&] (bool isDown) { if(isDown) toggleHdr(); });

    m_input->listenToKey(Key::Y, [&] (bool isDown) { if(isDown && m_running == AppState::ConfirmExit) m_running = AppState::Exit; });
    m_input->listenToKey(Key::N, [&] (bool isDown) { if(isDown && m_running == AppState::ConfirmExit) m_running = AppState::Running; });

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
    m_input->listenToKey(Key::ScrollLock, [&] (bool isDown) { if(isDown) toggleDebug(); });
    m_input->listenToKey(Key::Pause, [&] (bool isDown) { if(isDown){ playSound(0); togglePause(); } });
  }

  void draw()
  {
    m_renderer->beginDraw();

    for(auto& actor : m_actors)
    {
      auto where = Rect3f(
        actor.pos.x, actor.pos.y, actor.pos.z,
        actor.scale.x, actor.scale.y, actor.scale.z);
      m_renderer->drawActor(where, actor.orientation, actor.model, actor.effect == Effect::Blinking);
    }

    for(auto& actor : m_lightActors)
      m_renderer->drawLight(actor.pos, actor.color);

    if(m_running == AppState::ConfirmExit)
      m_renderer->drawText(Vec2f(0, 0), "QUIT? [Y/N]");
    else if(m_paused)
      m_renderer->drawText(Vec2f(0, 0), "PAUSE");
    else if(m_slowMotion)
      m_renderer->drawText(Vec2f(0, 0), "SLOW-MOTION MODE");

    if(m_debugMode)
    {
      for(int i = 0; i < getStatCount(); ++i)
      {
        auto stat = getStat(i);
        char buf[256];
        m_renderer->drawText(Vec2f(0, 4 - i * 0.25), format(buf, "%s: %.2f", stat.name, stat.val));
      }
    }

    if(m_textboxDelay > 0)
    {
      m_renderer->drawText(Vec2f(0, 2), m_textbox);
      m_textboxDelay--;
    }

    m_renderer->endDraw();

    m_recorder.captureDisplayFrameIfNeeded(m_graphicsBackend.get(), RESOLUTION);
  }

  void onQuit()
  {
    if(m_running == AppState::ConfirmExit)
      m_running = AppState::Running;
    else
      m_running = AppState::ConfirmExit;
  }

  void toggleVideoCapture()
  {
    if(m_fullscreen)
    {
      logMsg("Can't capture video in fullscreen mode");
      return;
    }

    if(m_recorder.toggleVideoCapture())
    {
      m_fixedDisplayFramePeriod = CAPTURE_FRAME_PERIOD;
      logMsg("Capturing video at %d Hz...", 1000 / CAPTURE_FRAME_PERIOD);
    }
    else
    {
      m_fixedDisplayFramePeriod = 0;
    }
  }

  void toggleFullScreen()
  {
    if(m_fixedDisplayFramePeriod)
    {
      logMsg("Can't toggle full-screen during video capture");
      return;
    }

    m_fullscreen = !m_fullscreen;
    m_graphicsBackend->setFullscreen(m_fullscreen);
  }

  void onMouseClick()
  {
    m_doGrab = true;
    m_graphicsBackend->enableGrab(m_doGrab);
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
    playSound(0);
    m_paused = !m_paused;
  }

  void toggleGrab()
  {
    m_doGrab = !m_doGrab;
    m_graphicsBackend->enableGrab(m_doGrab);
  }

  void toggleFsaa()
  {
    m_enableFsaa = !m_enableFsaa;
    m_renderer->setFsaa(m_enableFsaa);
  }

  void toggleHdr()
  {
    m_enableHdr = !m_enableHdr;
    m_renderer->setHdr(m_enableHdr);
  }

  // View implementation
  void setTitle(String gameTitle) override
  {
    m_graphicsBackend->setCaption(gameTitle);
  }

  void preload(Resource res) override
  {
    switch(res.type)
    {
    case ResourceType::Sound:
      m_audio->loadSound(res.id, res.path);
      break;
    case ResourceType::Model:
      m_renderer->loadModel(res.id, res.path);
      break;
    }
  }

  void textBox(String msg) override
  {
    m_textbox.assign(msg.data, msg.len);
    m_textboxDelay = 60 * 2;
  }

  void playMusic(int musicName) override
  {
    if(m_currMusicName == musicName)
      return;

    stopMusic();

    char buffer[256];
    m_audio->loadSound(1024, format(buffer, "res/music/music-%02d.ogg", musicName));

    m_musicVoice = m_audio->createVoice();
    printf("playing music #%d on voice %d\n", musicName, m_musicVoice);

    m_audio->playVoice(m_musicVoice, 1024, true);
    m_currMusicName = musicName;
  }

  void stopMusic() override
  {
    if(m_musicVoice == -1)
      return;

    m_audio->stopVoice(m_musicVoice); // maybe add a fade out here?
    m_audio->releaseVoice(m_musicVoice, true);
    m_musicVoice = -1;
  }

  Audio::VoiceId m_musicVoice = -1;
  int m_currMusicName = -1;

  void playSound(int soundId) override
  {
    auto voiceId = m_audio->createVoice();
    m_audio->playVoice(voiceId, soundId);
    m_audio->releaseVoice(voiceId, true);
  }

  void setCameraPos(Vec3f pos, Quaternion orientation) override
  {
    m_renderer->setCamera(pos, orientation);
  }

  void setAmbientLight(float amount) override
  {
    m_renderer->setAmbientLight(amount);
  }

  void sendLight(LightActor const& actor) override
  {
    m_lightActors.push_back(actor);
  }

  void sendActor(Actor const& actor) override
  {
    m_actors.push_back(actor);
  }

  enum class AppState
  {
    Exit = 0,
    Running = 1,
    ConfirmExit = 2,
  };

  AppState m_running = AppState::Running;
  int m_fixedDisplayFramePeriod = 0;

  VideoCapture m_recorder;

  bool m_debugMode = false;
  bool m_enableHdr = true;
  bool m_enableFsaa = false;

  int m_lastTime;
  int m_lastDisplayFrameTime;
  RateCounter m_fps;
  RateCounter m_tps;
  Control m_control {};
  vector<string> m_args;
  bool m_slowMotion = false;
  bool m_fullscreen = false;
  bool m_paused = false;
  bool m_doGrab = true;
  unique_ptr<MixableAudio> m_audio;
  unique_ptr<IAudioBackend> m_audioBackend;
  unique_ptr<IRenderer> m_renderer;
  unique_ptr<IGraphicsBackend> m_graphicsBackend;
  vector<Actor> m_actors;
  vector<LightActor> m_lightActors;
  unique_ptr<UserInput> m_input;

  string m_textbox;
  int m_textboxDelay = 0;

  unique_ptr<Scene> m_scene;
};

///////////////////////////////////////////////////////////////////////////////

unique_ptr<IApp> createApp(Span<char*> args)
{
  return make_unique<App>(args);
}

