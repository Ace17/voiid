#include "base/resource.h"
#include "sounds.h"
#include "models.h"

static const Resource sounds[] =
{
  { SND_PAUSE, "res/sounds/pause.ogg" },
  { SND_FIRE, "res/sounds/fire.ogg" },
  { SND_JUMP, "res/sounds/jump.ogg" },
  { SND_LAND, "res/sounds/land.ogg" },
  { SND_SWITCH, "res/sounds/switch.ogg" },
  { SND_DOOR, "res/sounds/door.ogg" },
  { SND_HURT, "res/sounds/hurt.ogg" },
  { SND_DIE, "res/sounds/die.ogg" },
  { SND_BONUS, "res/sounds/bonus.ogg" },
  { SND_DAMAGE, "res/sounds/damage.ogg" },
  { SND_EXPLODE, "res/sounds/explode.ogg" },
  { SND_DISAPPEAR, "res/sounds/disappear.ogg" },
};

static const Resource models[] =
{
  { MDL_TILES, "res/tiles.mdl" },
  { MDL_DOOR, "res/sprites/door.json" },
  { MDL_RECT, "res/sprites/rect.json" },
  { MDL_AMULET, "res/sprites/amulet.json" },
  { MDL_INVRECT, "res/sprites/invrect.json" },
  { MDL_SWITCH, "res/sprites/switch.json" },
  { MDL_WHEEL, "res/sprites/wheel.json" },
  { MDL_LIFEBAR, "res/sprites/lifebar.json" },
  { MDL_TELEPORTER, "res/sprites/teleporter.json" },
  { MDL_BONUS, "res/sprites/bonus.json" },
  { MDL_BULLET, "res/sprites/bullet.json" },
  { MDL_EXPLOSION, "res/sprites/explosion.json" },
  { MDL_SPIKES, "res/sprites/spikes.json" },
  { MDL_SIGN, "res/sprites/sign.json" },
  { MDL_ROOMS + 0, "res/rooms/00/mesh.json" },
  { MDL_ROOMS + 1, "res/rooms/01/mesh.json" },
  { MDL_ROOMS + 2, "res/rooms/02/mesh.json" },
  { MDL_ROOMS + 3, "res/rooms/03/mesh.json" },
  { MDL_ROOMS + 4, "res/rooms/04/mesh.json" },
  { MDL_ROOMS + 5, "res/rooms/05/mesh.json" },
  { MDL_ROOMS + 6, "res/rooms/ending/mesh.json" },
};

Span<const Resource> getSounds()
{
  return makeSpan(sounds);
}

Span<const Resource> getModels()
{
  return makeSpan(models);
}

