/*
 * Copyright (C) 2017 - Sebastien Alaiwan <sebastien.alaiwan@gmail.com>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

// Top-level game logic

#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <array>
#include <set>
#include "base/scene.h"
#include "base/util.h"
#include "entities/player.h"
#include "entities/rockman.h"
#include "entities/editor.h"
#include "entity_factory.h"
#include "game.h"
#include "room.h"

#include "entities/finish.h" // TouchFinishLineEvent

using namespace std;

// from physics.cpp
unique_ptr<IPhysics> createPhysics();

struct Game : Scene, IGame
{
  Game(View* view) :
    m_view(view)
  {
    m_shouldLoadLevel = true;
    m_physics = createPhysics();
    m_physics->setEdifice(bind(&Game::traceEdifice, this, placeholders::_1, placeholders::_2));
  }

  ////////////////////////////////////////////////////////////////
  // Scene: Game, seen by the engine

  void tick(Control const& c) override
  {
    if(m_shouldLoadLevel)
    {
      loadLevel(m_level);
      m_shouldLoadLevel = false;
    }

    m_player->think(c);

    for(auto& e : m_entities)
      e->tick();

    m_physics->checkForOverlaps();
    removeDeadThings();

    m_debug = c.debug;

    if(c.debug && m_debugFirstTime)
    {
      m_debugFirstTime = false;
      m_player->addUpgrade(-1);
    }

    ambientLight = 1.0 + min(0.0f, 0.02f * m_player->pos.z);
  }

  vector<Actor> getActors() const override
  {
    vector<Actor> r;

    r.push_back(Actor(Vector(0, 0, 0), MDL_ROOMS + m_level));

    auto cameraPos = m_player->pos;

    Box cameraRect;
    cameraRect.cx = 32;
    cameraRect.cy = 32;
    cameraRect.cz = 16;
    cameraRect.x = cameraPos.x - cameraRect.cx / 2;
    cameraRect.y = cameraPos.y - cameraRect.cy / 2;
    cameraRect.z = cameraPos.z - cameraRect.cz / 2;

    for(auto& entity : m_entities)
    {
      r.push_back(entity->getActor());

      if(m_debug)
        r.push_back(getDebugActor(entity.get()));
    }

    return r;
  }

  void removeDeadThings()
  {
    removeDeadEntities(m_entities);

    for(auto& spawned : m_spawned)
    {
      spawned->game = this;
      spawned->physics = m_physics.get();
      spawned->enter();

      m_physics->addBody(spawned.get());
      m_entities.push_back(move(spawned));
    }

    m_spawned.clear();
  }

  vector<Convex> world;

  void loadLevel(int levelIdx)
  {
    if(m_player)
    {
      for(auto& entity : m_entities)
        if(m_player == entity.get())
          entity.release();
    }

    m_physics->clearBodies();

    m_entities.clear();
    m_spawned.clear();
    m_listeners.clear();

    auto level = loadRoom(levelIdx);
    m_view->playMusic(levelIdx);

    if(!m_player)
      m_player = makeRockman().release();

    m_player->pos = Vector(level.start.x, level.start.y, level.start.z) - m_player->size * 0.5;

    for(auto& thing : level.things)
    {
      auto ent = createEntity(thing.formula);
      ent->pos = thing.pos;
      spawn(ent.release());
    }

    world = level.brushes;

    spawn(m_player);

    {
      auto f = bind(&Game::onTouchLevelBoundary, this, placeholders::_1);
      m_levelBoundary = makeDelegator<TouchFinishLineEvent>(f);
      subscribeForEvents(&m_levelBoundary);
    }
  }

  void onTouchLevelBoundary(const TouchFinishLineEvent* event)
  {
    (void)event;
    m_shouldLoadLevel = true;
    m_level++;
  }

  int m_level = 1;
  bool m_shouldLoadLevel = false;

  EventDelegator m_levelBoundary;

  ////////////////////////////////////////////////////////////////
  // IGame: game, as seen by the entities

  void playSound(SOUND sound) override
  {
    m_view->playSound(sound);
  }

  void spawn(Entity* e) override
  {
    m_spawned.push_back(unique(e));
  }

  void postEvent(unique_ptr<Event> event) override
  {
    for(auto& listener : m_listeners)
      listener->notify(event.get());
  }

  void subscribeForEvents(IEventSink* sink) override
  {
    m_listeners.insert(sink);
  }

  void unsubscribeForEvents(IEventSink* sink) override
  {
    m_listeners.erase(sink);
  }

  Vector getPlayerPosition() override
  {
    return m_player->pos;
  }

  void textBox(char const* msg) override
  {
    m_view->textBox(msg);
  }

  Player* m_player = nullptr;
  uvector<Entity> m_entities;
  uvector<Entity> m_spawned;
  View* const m_view;
  unique_ptr<IPhysics> m_physics;

  set<IEventSink*> m_listeners;

  bool m_debug;
  bool m_debugFirstTime = true;

  // static stuff

  void removeDeadEntities(uvector<Entity>& entities)
  {
    auto oldEntities = move(entities);

    for(auto& entity : oldEntities)
    {
      if(!entity->dead)
        entities.push_back(move(entity));
      else
      {
        entity->leave();
        m_physics->removeBody(entity.get());
      }
    }
  }

  static Actor getDebugActor(Entity* entity)
  {
    auto rect = entity->getBox();
    auto r = Actor(Vector(rect.x, rect.y, rect.z), MDL_RECT);
    r.scale = rect;
    return r;
  }

  Trace traceEdifice(Box box, Vector delta) const
  {
    Trace r {};
    r.fraction = 1.0;

    for(auto& brush : world)
    {
      auto const halfSize = Vector3f(box.cx, box.cy, box.cz) * 0.5;
      auto const pos = Vector3f(box.x, box.y, box.z) + halfSize;
      auto t = brush.trace(pos, pos + delta, halfSize.x);

      if(t.fraction < r.fraction)
      {
        r.fraction = t.fraction;
        r.plane = t.plane;
      }
    }

    return r;
  }
};

Scene* createGame(View* view, vector<string> args)
{
  auto r = make_unique<Game>(view);

  if(args.size() >= 1)
    r->m_level = atoi(args[0].c_str());

  return r.release();
}

