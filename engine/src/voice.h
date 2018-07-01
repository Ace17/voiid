// Copyright (C) 2018 - Sebastien Alaiwan <sebastien.alaiwan@gmail.com>
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// An audio voice

#pragma once

#include "sound.h"

struct Voice
{
  bool isDead() const
  {
    return m_isDead;
  }

  void play(Sound* sound, bool loop = false)
  {
    m_sound = sound;
    m_player = sound->createPlayer();
    m_isDead = false;
    m_loop = loop;
  }

  void mix(Span<float> output)
  {
    while(output.len > 0)
    {
      auto const n = m_player->mix(output);
      output.data += n;
      output.len -= n;

      if(output.len == 0)
        break;

      m_sound = nextSound();

      if(!m_sound)
      {
        m_isDead = true;
        break;
      }

      m_player = m_sound->createPlayer();
    }
  }

private:
  Sound* nextSound()
  {
    if(m_loop)
      return m_sound;

    return nullptr;
  }

  bool m_isDead = true;
  bool m_loop = false;
  Sound* m_sound;
  unique_ptr<ISoundPlayer> m_player;
};

