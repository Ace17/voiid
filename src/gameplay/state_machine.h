#pragma once

#include "base/scene.h"
#include "base/view.h"

Scene* createSplashState(View* view);
Scene* createPlayingState(View* view);
Scene* createEndingState(View* view);
Scene* createPlayingStateAtLevel(View* view, int level);

