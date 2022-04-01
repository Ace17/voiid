// Copyright (C) 2021 - Sebastien Alaiwan
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// Game logic

#include <algorithm>
#include <list>
#include <map>

#include "base/scene.h"
#include "base/string.h"
#include "base/util.h"
#include "misc/file.h"

#include "entity_factory.h"
#include "game.h"
#include "models.h"
#include "physics.h"
#include "player.h"
#include "room.h"
#include "state_machine.h"
#include "variable.h"

using namespace std;

std::unique_ptr<Player> makeHero();

namespace
{
struct EntityConfigImpl : IEntityConfig
{
  string getString(const char* varName, string defaultValue) override
  {
    auto i = values.find(varName);

    if(i == values.end())
      return defaultValue;

    return i->second;
  }

  int getInt(const char* varName, int defaultValue) override
  {
    auto i = values.find(varName);

    if(i == values.end())
      return defaultValue;

    return atoi(i->second.c_str());
  }

  map<string, string> values;
};

void spawnEntities(Room const& room, IGame* game, int levelIdx)
{
  // avoid collisions between static entities from different rooms
  int id = levelIdx * 1000;

  for(auto& spawner : room.things)
  {
    EntityConfigImpl config;
    config.values = spawner.config;

    auto entity = createEntity(spawner.name, &config);
    entity->pos = spawner.pos;
    game->spawn(entity.release());

    ++id;
  }
}

struct ShapePolyhedron : Shape
{
  Trace raycast(Vector3f A, Vector3f B, Vector3f boxHalfSize) const override
  {
    return convex.trace(A, B, boxHalfSize);
  }

  Convex convex {};
};

struct Brush
{
  std::unique_ptr<Body> body;
  ShapePolyhedron shape;
};

struct GameState : Scene, private IGame
{
  GameState(View* view) :
    m_view(view)
  {
    resetPhysics();
  }

  void resetPhysics()
  {
    m_physics = createPhysics();

    for(auto& brush : m_brushes)
      m_physics->addBody(brush.body.get());
  }

  ////////////////////////////////////////////////////////////////
  // Scene: Game, seen by the engine

  Scene* tick(Control c) override
  {
    loadLevelIfNeeded();

    m_player->think(c);

    for(auto& e : m_entities)
      e->tick();

    m_physics->checkForOverlaps();
    removeDeadThings();

    processEvents();

    updateDebugFlag(c.debug);

    if(m_gameFinished)
    {
      return createEndingState(m_view);
    }

    return this;
  }

  void draw() override
  {
    if(!m_levelIsLoaded)
      return;

    m_view->sendActor(Actor(Vector(0, 0, 0), MDL_ROOMS));

    {
      auto playerLight = LightActor{ m_player->getCenter() + Vector3f(0, 0, 1), Vector3f(0.3, 0.3, 0.3) };
      m_view->sendLight(playerLight);

      for(auto light: m_staticLevelLights)
        m_view->sendLight(light);
    }

    for(auto& entity : m_entities)
    {
      entity->onDraw(m_view);

      if(m_debug)
        m_view->sendActor(getDebugActor(entity.get()));
    }
  }

  ////////////////////////////////////////////////////////////////
  // internals

  void loadLevelIfNeeded()
  {
    if(!m_levelIsLoaded)
    {
      loadLevel(m_level);
      m_levelIsLoaded = true;
    }
  }

  void updateDebugFlag(float debugFlag)
  {
    m_debug = debugFlag;

    if(debugFlag && m_debugFirstTime)
    {
      m_debugFirstTime = false;
      m_player->addUpgrade(-1);
    }
  }

  void processEvents()
  {
    auto events = move(m_eventQueue);

    for(auto& event : events)
    {
      for(auto& listener : m_listeners)
        listener->notify(event.get());
    }
  }

  void removeDeadThings()
  {
    for(auto& entity : m_entities)
    {
      if(entity->dead)
      {
        entity->leave();
        m_physics->removeBody(entity.get());
      }
    }

    unstableRemove(m_entities, &isDead);

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

  static bool isDead(unique_ptr<Entity> const& e)
  {
    return e->dead;
  }

  void loadLevel(int levelIdx)
  {
    char buf[256];

    printf("[gameplay] loading level %d\n", levelIdx);

    {
      const auto filename = format(buf, "res/rooms/%02d/room.render", levelIdx);
      m_view->preload(Resource { ResourceType::Model, MDL_ROOMS, filename });
    }

    {
      float ambientLight = 1.0;
      const auto filename = format(buf, "res/rooms/%02d/room.settings", levelIdx);
      const auto text = File::read(filename);
      sscanf(text.c_str(), "%f", &ambientLight);
      m_view->setAmbientLight(ambientLight);
    }

    if(m_player)
    {
      for(auto& entity : m_entities)
        if(m_player == entity.get())
          entity.release();
    }

    m_entities.clear();
    m_spawned.clear();
    assert(m_listeners.empty());

    {
      const auto filename = format(buf, "res/rooms/%02d/room.fbx", levelIdx);

      auto level = loadRoom(filename);

      m_brushes.resize(level.colliders.size());

      for(int i = 0; i < (int)m_brushes.size(); ++i)
      {
        m_brushes[i].shape.convex = level.colliders[i];
        m_brushes[i].body = std::make_unique<Body>();
        m_brushes[i].body->shape = &m_brushes[i].shape;
        m_brushes[i].body->solid = 1;
      }

      if(!m_player)
        m_player = makeHero().release();

      m_player->pos = Vector(level.start.x, level.start.y, level.start.z);

      spawnEntities(level, this, levelIdx);

      m_staticLevelLights.clear();

      for(auto& light : level.lights)
        m_staticLevelLights.push_back({ light.pos, light.color });
    }

    resetPhysics();

    m_view->playMusic(levelIdx);

    spawn(m_player);

    removeDeadThings();

    printf("[gameplay] level loaded : %d brushes, %d lights\n", (int)m_brushes.size(), (int)m_staticLevelLights.size());
  }

  void endLevel() override
  {
    m_levelIsLoaded = false;
    m_level++;

    m_gameFinished = true;
  }

  int m_level = 1;
  bool m_levelIsLoaded = false;
  std::vector<LightActor> m_staticLevelLights;

  vector<unique_ptr<Event>> m_eventQueue;

  ////////////////////////////////////////////////////////////////
  // IGame: game, as seen by the entities

  void playSound(int sound) override
  {
    m_view->playSound(sound);
  }

  void spawn(Entity* e) override
  {
    m_spawned.push_back(unique_ptr<Entity>(e));
  }

  void postEvent(unique_ptr<Event> event) override
  {
    m_eventQueue.push_back(move(event));
  }

  unique_ptr<Handle> subscribeForEvents(IEventSink* sink) override
  {
    auto it = m_listeners.insert(m_listeners.begin(), sink);

    auto unsubscribe = [ = ] () { m_listeners.erase(it); };

    return make_unique<HandleWithDeleter>(unsubscribe);
  }

  void textBox(String msg) override
  {
    m_view->textBox(msg);
  }

  Player* m_player = nullptr;
  vector<unique_ptr<Entity>> m_spawned;
  View* const m_view;
  unique_ptr<IPhysics> m_physics;

  bool m_gameFinished = false;

  list<IEventSink*> m_listeners;

  std::vector<Brush> m_brushes;
  bool m_debug;
  bool m_debugFirstTime = true;

  vector<unique_ptr<Entity>> m_entities;

  // static stuff

  static Actor getDebugActor(Entity* entity)
  {
    auto rect = entity->getBox();
    auto r = Actor(rect.pos, MDL_RECT);
    r.scale = rect.size;
    return r;
  }
};
}

Scene* createPlayingStateAtLevel(View* view, int level)
{
  auto gameState = make_unique<GameState>(view);
  gameState->m_level = level;
  return gameState.release();
}

Scene* createPlayingState(View* view)
{
  return createPlayingStateAtLevel(view, 1);
}

