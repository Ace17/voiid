#include "explosion.h"

#include "base/scene.h"
#include "base/util.h"
#include "entities/player.h"
#include "entity.h"
#include "models.h"
#include "sounds.h"

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

  virtual void onDraw(View* view) const override
  {
    auto r = Actor(pos, MDL_RECT);

    r.ratio = time / (float)DURATION;
    r.scale = UnitSize * 1.5;
    r.pos -= r.scale * 0.5;

    view->sendActor(r);
  }

  int time = 0;
};

std::unique_ptr<Entity> makeExplosion()
{
  return make_unique<Explosion>();
}

