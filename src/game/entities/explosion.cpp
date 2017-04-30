#include "explosion.h"

#include "base/util.h"
#include "base/scene.h"
#include "game/entity.h"
#include "game/models.h"
#include "game/sounds.h"
#include "game/entities/player.h"

static auto const DURATION = 500;

struct Explosion : Entity
{
  Explosion()
  {
    size = UnitSize * 0.1;
  }

  virtual void tick() override
  {
    time++;

    if(time >= DURATION)
    {
      time = DURATION;
      dead = true;
    }
  }

  virtual Actor getActor() const override
  {
    auto r = Actor(pos, MDL_EXPLOSION);

    r.ratio = time / (float)DURATION;
    r.scale = UnitSize * 1.5;
    r.pos -= r.scale * 0.5;

    return r;
  }

  int time = 0;
};

std::unique_ptr<Entity> makeExplosion()
{
  return make_unique<Explosion>();
}

