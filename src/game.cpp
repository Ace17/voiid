/*
 * Copyright (C) 2017 - Sebastien Alaiwan <sebastien.alaiwan@gmail.com>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

// Game logic

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
#include "game.h"
#include "models.h" // MDL_TILES
#include "room.h"

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

    r.push_back(Actor(Vector(0, 0, 0), MDL_ROOMS));

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

  vector<Brush> world;

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
    m_theme = level.theme;
    m_view->playMusic(level.theme);
    printf("Now in: %s\n", level.name.c_str());

    if(!m_player)
    {
      m_player = makeRockman().release();

      m_player->pos = Vector(level.start.x, level.start.y, level.start.z);
    }

    world = level.brushes;

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
  int m_editorMode = 0;
  Vector m_transform;
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

  bool isRectSolid(Box box)
  {
    for(auto& brush : world)
    {
      auto const pos = Vector3f(box.x, box.y, box.z);
      auto t = brush.trace(pos, pos, 1.0);

      if(t.fraction < 1.0)
        return true;
    }

    return false;
  }

  struct Plane
  {
    Vector3f N;
    float dist;
  };

  static bool boxIntersectsTriangle(Box b, Vector3f A, Vector3f B, Vector3f C)
  {
    auto boxMin = Vector3f(b.x, b.y, b.y);
    auto boxMax = boxMin + Vector3f(b.cx, b.cy, b.cz);

    // test triangle against one box face
    if(max3(A.x, B.x, C.x) < boxMin.x)
      return false;

    if(min3(A.x, B.x, C.x) > boxMax.x)
      return false;

    // test triangle against one box face
    if(max3(A.y, B.y, C.y) < boxMin.y)
      return false;

    if(min3(A.y, B.y, C.y) > boxMax.y)
      return false;

    // test triangle against one box face
    if(max3(A.z, B.z, C.z) < boxMin.z)
      return false;

    if(min3(A.z, B.z, C.z) > boxMax.z)
      return false;

    // test box against triangle face
    // auto const N = crossProduct(B - A, C - A);
    // auto const dist = dotProduct(N, A);

    return true;
  }

  static float max3(float a, float b, float c)
  {
    return max(a, max(b, c));
  }

  static float min3(float a, float b, float c)
  {
    return min(a, min(b, c));
  }

  static bool boxIntersectsPlane(Box box, Plane p)
  {
    auto posMin = Vector3f(box.x, box.y, box.y);
    auto posMax = posMin + Vector3f(box.cx, box.cy, box.cz);

    if(sign(dotProduct(posMin, p.N)) != sign(dotProduct(posMax, p.N)))
      return true;

    return false;
  }

  static float sign(float val)
  {
    return val < 0 ? -1 : 1;
  }
};

Scene* createGame(View* view, vector<string> args)
{
  auto r = make_unique<Game>(view);

  if(args.size() >= 1)
    r->m_level = atoi(args[0].c_str());

  if(args.size() >= 2)
    r->m_editorMode = atoi(args[1].c_str());

  return r.release();
}

