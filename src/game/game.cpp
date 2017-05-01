// Game logic

/*
 * Copyright (C) 2017 - Sebastien Alaiwan <sebastien.alaiwan@gmail.com>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <array>
#include <map>
#include <set>
#include "base/scene.h"
#include "base/util.h"
#include "entities/player.h"
#include "entities/rockman.h"
#include "game.h"
#include "models.h" // MDL_TILES
#include "room.h"

using namespace std;

// from physics.cpp
unique_ptr<IPhysics> createPhysics();

struct Game : Scene, IGame
{
  Game(View* view) :
    m_view(view),
    m_tiles(Size3i(1, 1, 1))
  {
    m_shouldLoadLevel = true;
    m_physics = createPhysics();
    m_physics->setEdifice(bind(&Game::isRectSolid, this, placeholders::_1));
  }

  ////////////////////////////////////////////////////////////////
  // Scene: Game, seen by the engine

  void tick(Control const& c) override
  {
    if(m_shouldLoadLevel)
    {
      loadLevel(m_level);
      m_player->pos += m_transform;
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
  }

  vector<Actor> getActors() const override
  {
    vector<Actor> r;

    auto cameraPos = m_player->pos;

    addActorsForTileMap(r, cameraPos);

    Box cameraRect;
    cameraRect.cx = 16;
    cameraRect.cx = 16;
    cameraRect.cz = 16;
    cameraRect.x = cameraPos.x - cameraRect.cx / 2;
    cameraRect.y = cameraPos.y - cameraRect.cy / 2;
    cameraRect.z = cameraPos.z - cameraRect.cz / 2;

    for(auto& entity : m_entities)
    {
      if(!overlaps(entity->getRect(), cameraRect))
        continue;

      r.push_back(entity->getActor());

      if(m_debug)
        r.push_back(getDebugActor(entity.get()));
    }

    return r;
  }

  vector<SOUND> readSounds() override
  {
    return std::move(m_sounds);
  }

  void addActorsForTileMap(vector<Actor>& r, Vector cameraPos) const
  {
    auto onCell =
      [&] (int x, int y, int z, int tile)
      {
        if(!tile)
          return;

        if(abs(x - cameraPos.x) > 16)
          return;

        if(abs(y - cameraPos.y) > 16)
          return;

        if(abs(z - cameraPos.z) > 9)
          return;

        auto actor = Actor(Vector(x, y, z), MDL_TILES);
        actor.action = (m_theme % 8) * 8;
        actor.scale = UnitSize;
        r.push_back(actor);
      };

    m_tiles.scan(onCell);
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

    auto level = Graph_loadRoom(levelIdx, this);
    m_tiles = move(level.tiles);
    m_theme = level.theme;
    m_view->playMusic(level.theme);
    printf("Now in: %s\n", level.name.c_str());

    if(!m_player)
    {
      m_player = makeRockman().release();
      m_player->pos = Vector(level.start.x, level.start.y, level.start.z);
    }

    spawn(m_player);

    {
      auto f = bind(&Game::onTouchLevelBoundary, this, std::placeholders::_1);
      m_levelBoundary = makeDelegator<TouchLevelBoundary>(f);
      subscribeForEvents(&m_levelBoundary);
    }
  }

  void onTouchLevelBoundary(const TouchLevelBoundary* event)
  {
    (void)event;
    m_shouldLoadLevel = true;
    m_transform = event->transform;
    m_level = event->targetLevel;
  }

  int m_level = 1;
  int m_theme = 0;
  Vector m_transform;
  bool m_shouldLoadLevel = false;

  EventDelegator m_levelBoundary;

  ////////////////////////////////////////////////////////////////
  // IGame: game, as seen by the entities

  void playSound(SOUND sound) override
  {
    m_sounds.push_back(sound);
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

  Matrix m_tiles;
  vector<SOUND> m_sounds;
  bool m_debug;
  bool m_debugFirstTime = true;

  // static stuff

  void removeDeadEntities(uvector<Entity>& entities)
  {
    auto oldEntities = std::move(entities);

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
    auto rect = entity->getRect();
    auto r = Actor(Vector(rect.x, rect.y, rect.z), MDL_RECT);
    r.scale = rect;
    return r;
  }

  bool isRectSolid(Box rect)
  {
    auto const x1 = (int)min(rect.x, rect.x + rect.cx);
    auto const x2 = (int)max(rect.x, rect.x + rect.cx);

    auto const y1 = (int)min(rect.y, rect.y + rect.cy);
    auto const y2 = (int)max(rect.y, rect.y + rect.cy);

    auto const z1 = (int)min(rect.z, rect.z + rect.cz);
    auto const z2 = (int)max(rect.z, rect.z + rect.cz);

    for(int x = x1; x <= x2; ++x)
      for(int y = y1; y <= y2; ++y)
        for(int z = z1; z <= z2; ++z)
          if(isPointSolid(Vector(x, y, z)))
            return true;

    return false;
  }

  bool isPointSolid(Vector pos)
  {
    auto const x = (int)pos.x;
    auto const y = (int)pos.y;
    auto const z = (int)pos.z;

    if(!m_tiles.isInside(x, y, z))
      return false;

    if(m_tiles.get(x, y, z) == 0)
      return false;

    return true;
  }
};

Scene* createGame(View* view, vector<string> args)
{
  auto r = make_unique<Game>(view);

  if(args.size() == 1)
    r->m_level = atoi(args[0].c_str());

  return r.release();
}

