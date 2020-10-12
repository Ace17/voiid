#pragma once

#include "gameplay/game.h" // Matrix
struct Player;

#include <memory>

std::unique_ptr<Player> makeEditor(Matrix& tiles);

