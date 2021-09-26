// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// User input event source: SDL implementation

#include "engine/input.h"
#include "SDL.h"
#include <map>
#include <stdexcept>
#include <string>

namespace
{
void unbound()
{
  printf("input: this key/event is unbound\n");
}

void unbound2(int)
{
  printf("input: this key/event is unbound\n");
}

void unbound3(int, int)
{
}

Uint32 translateToSdlKey(Key key)
{
  switch(key)
  {
  case Key::Space: return SDLK_SPACE;
  case Key::Esc: return SDLK_ESCAPE;
  case Key::Pause: return SDLK_PAUSE;
  case Key::Return: return SDLK_RETURN;
  case Key::RightControl: return SDLK_RCTRL;
  case Key::PrintScreen: return SDLK_PRINTSCREEN;
  case Key::F2: return SDLK_F2;
  case Key::F3: return SDLK_F3;
  case Key::F4: return SDLK_F4;
  case Key::Left: return SDLK_LEFT;
  case Key::Right: return SDLK_RIGHT;
  case Key::Up: return SDLK_UP;
  case Key::Down: return SDLK_DOWN;
  case Key::Tab: return SDLK_TAB;
  case Key::CapsLock: return SDLK_CAPSLOCK;
  case Key::ScrollLock: return SDLK_SCROLLLOCK;
  case Key::A: return SDLK_a;
  case Key::C: return SDLK_c;
  case Key::D: return SDLK_d;
  case Key::E: return SDLK_e;
  case Key::N: return SDLK_n;
  case Key::R: return SDLK_r;
  case Key::S: return SDLK_s;
  case Key::W: return SDLK_w;
  case Key::X: return SDLK_x;
  case Key::Y: return SDLK_y;
  case Key::Z: return SDLK_z;
  }

  return 0;
}

Uint32 withModifiers(Uint32 sdlKey, bool modControl, bool modAlt)
{
  Uint32 r = sdlKey;

  if(modControl)
    r |= 0x80000000;

  if(modAlt)
    r |= 0x40000000;

  return r;
}

struct SdlUserInput : UserInput
{
  SdlUserInput()
  {
    if(SDL_InitSubSystem(SDL_INIT_EVENTS))
      throw std::runtime_error(std::string("Can't init input: ") + SDL_GetError());

    m_quitDelegate = &unbound;
  }

  ~SdlUserInput()
  {
    SDL_QuitSubSystem(SDL_INIT_EVENTS);
  }

  void process() override
  {
    SDL_Event event;

    while(SDL_PollEvent(&event))
    {
      switch(event.type)
      {
      case SDL_QUIT:
        m_quitDelegate();
        break;
      case SDL_KEYDOWN:
        onKeyDown(&event);
        break;
      case SDL_KEYUP:
        onKeyUp(&event);
        break;
      case SDL_MOUSEWHEEL:
        onMouseWheel(&event);
        break;
      case SDL_MOUSEMOTION:
        onMouseMotion(&event);
        break;
      case SDL_MOUSEBUTTONDOWN:
        onMouseClick(&event);
        break;
      }
    }
  }

  void listenToKey(Key key, Delegate<void(bool)> func, bool modControl, bool modAlt) override
  {
    m_keyDelegates[withModifiers(translateToSdlKey(key), modControl, modAlt)] = std::move(func);
  }

  void listenToQuit(Delegate<void()> onQuit) override
  {
    m_quitDelegate = std::move(onQuit);
  }

  void listenToMouseWheel(Delegate<void(int)> onWheel) override
  {
    m_wheelDelegate = std::move(onWheel);
  }

  void listenToMouseMove(Delegate<void(int, int)> onRelativeMove) override
  {
    m_relativeMoveDelegate = std::move(onRelativeMove);
  }

  void listenToMouseClick(Delegate<void(int, int)> onClick) override
  {
    m_clickDelegate = std::move(onClick);
  }

private:
  std::map<Uint32, Delegate<void(bool)>> m_keyDelegates;
  Delegate<void()> m_quitDelegate = &unbound;
  Delegate<void(int)> m_wheelDelegate = &unbound2;
  Delegate<void(int, int)> m_relativeMoveDelegate = &unbound3;
  Delegate<void(int, int)> m_clickDelegate = &unbound3;

  void onKeyDown(SDL_Event* evt)
  {
    if(evt->key.repeat > 0)
      return;

    bool modControl = evt->key.keysym.mod & KMOD_CTRL;
    bool modAlt = evt->key.keysym.mod & KMOD_ALT;
    auto key = withModifiers(evt->key.keysym.sym, modControl, modAlt);
    auto i = m_keyDelegates.find(key);

    if(i == m_keyDelegates.end())
    {
      unbound();
      return;
    }

    i->second(true);
  }

  void onKeyUp(SDL_Event* evt)
  {
    bool modControl = evt->key.keysym.mod & KMOD_CTRL;
    bool modAlt = evt->key.keysym.mod & KMOD_ALT;

    auto key = withModifiers(evt->key.keysym.sym, modControl, modAlt);
    auto i = m_keyDelegates.find(key);

    if(i == m_keyDelegates.end())
    {
      unbound();
      return;
    }

    i->second(false);
  }

  void onMouseWheel(SDL_Event* evt)
  {
    m_wheelDelegate(evt->wheel.y);
  }

  void onMouseMotion(SDL_Event* evt)
  {
    m_relativeMoveDelegate(evt->motion.xrel, evt->motion.yrel);
  }

  void onMouseClick(SDL_Event* evt)
  {
    m_clickDelegate(evt->button.x, evt->button.y);
  }
};
}

UserInput* createUserInput()
{
  return new SdlUserInput();
}

