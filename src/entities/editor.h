#pragma once

#include "game.h" // Matrix
#include "player.h"
#include <memory>

std::unique_ptr<Player> makeEditor(Matrix& tiles);

