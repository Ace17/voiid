/*
 * Copyright (C) 2017 - Sebastien Alaiwan <sebastien.alaiwan@gmail.com>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

// Top-level game logic

#include <algorithm>
#include <list>
#include "base/scene.h"
#include "base/util.h"

#include "entities/player.h"
#include "entities/rockman.h"
#include "entities/editor.h"
#include "entity_factory.h"
#include "game.h"
#include "room.h"
#include "variable.h"
#include "state_machine.h"

#include "entities/finish.h" // TouchFinishLineEvent

using namespace std;

// from physics.cpp
unique_ptr<IPhysics> createPhysics();

struct GameState : Scene, IGame
{
  GameState(View* view) :
    m_view(view)
  {
    m_shouldLoadLevel = true;
    m_physics = createPhysics();
    m_physics->setEdifice(bind(&GameState::traceEdifice, this, placeholders::_1, placeholders::_2));
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
    cameraRect.size.cx = 32;
    cameraRect.size.cy = 32;
    cameraRect.size.cz = 16;
    cameraRect.pos.x = cameraPos.x - cameraRect.size.cx / 2;
    cameraRect.pos.y = cameraPos.y - cameraRect.size.cy / 2;
    cameraRect.pos.z = cameraPos.z - cameraRect.size.cz / 2;

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

  void loadLevel(int levelIdx)
  {
    m_levelBoundarySubscription.reset();

    if(m_player)
    {
      for(auto& entity : m_entities)
        if(m_player == entity.get())
          entity.release();
    }

    m_physics->clearBodies();

    m_entities.clear();
    m_spawned.clear();
    assert(m_listeners.empty());

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
      auto f = bind(&GameState::onTouchLevelBoundary, this, placeholders::_1);
      m_levelBoundary = makeDelegator<TouchFinishLineEvent>(f);
      m_levelBoundarySubscription = subscribeForEvents(&m_levelBoundary);
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
  unique_ptr<Handle> m_levelBoundarySubscription;

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

  unique_ptr<Handle> subscribeForEvents(IEventSink* sink) override
  {
    auto it = m_listeners.insert(m_listeners.begin(), sink);

    auto unsubscribe = [ = ] () { m_listeners.erase(it); };

    return make_unique<HandleWithDeleter>(unsubscribe);
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

  list<IEventSink*> m_listeners;

  vector<Convex> world;
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
    auto r = Actor(rect.pos, MDL_RECT);
    r.scale = rect.size;
    return r;
  }

  Trace traceEdifice(Box box, Vector delta) const
  {
    Trace r {};
    r.fraction = 1.0;

    for(auto& brush : world)
    {
      auto const halfSize = Vector3f(box.size.cx, box.size.cy, box.size.cz) * 0.5;
      auto const pos = box.pos + halfSize;
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

unique_ptr<Scene> createGameState(StateMachine* fsm, View* view, int level)
{
  (void)fsm;
  auto r = make_unique<GameState>(view);
  r->m_level = level;
  return r;
}

