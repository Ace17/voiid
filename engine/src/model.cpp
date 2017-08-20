/*
 * Copyright (C) 2017 - Sebastien Alaiwan <sebastien.alaiwan@gmail.com>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

#include "model.h"
#include "base/geom.h"
#include "base/util.h"
#include "json.h"

extern int loadTexture(string path, Rect2i rect = Rect2i(0, 0, 0, 0));

static
Action loadSheetAction(json::Value* val, string sheetPath, Size2i cell)
{
  Action r;

  auto action = json::cast<json::Object>(val);
  action->getMember<json::String>("name");
  auto frames = action->getMember<json::Array>("frames");

  for(auto& frame : frames->elements)
  {
    auto const idx = (json::cast<json::Number>(frame.get()))->value;

    auto const col = idx % 16;
    auto const row = idx / 16;
    r.addTexture(sheetPath, Rect2i(col * cell.width, row * cell.height, cell.width, cell.height));
  }

  return r;
}

Model boxModel()
{
  static const float vertices[] =
  {
    -0.5, -0.5, +0.5, /* uv */ 0, 1, /* N */ 0, 0, 1,
    -0.5, +0.5, +0.5, /* uv */ 0, 0, /* N */ 0, 0, 1,
    +0.5, +0.5, +0.5, /* uv */ 1, 0, /* N */ 0, 0, 1,
    +0.5, -0.5, +0.5, /* uv */ 1, 1, /* N */ 0, 0, 1,
    -0.5, -0.5, -0.5, /* uv */ 0, 1, /* N */ 0, 0, -1,
    +0.5, -0.5, -0.5, /* uv */ 0, 0, /* N */ 0, 0, -1,
    +0.5, +0.5, -0.5, /* uv */ 1, 0, /* N */ 0, 0, -1,
    -0.5, +0.5, -0.5, /* uv */ 1, 1, /* N */ 0, 0, -1,
    -0.5, -0.5, +0.5, /* uv */ 0, 1, /* N */ -1, 0, 0,
    -0.5, -0.5, -0.5, /* uv */ 0, 0, /* N */ -1, 0, 0,
    -0.5, +0.5, -0.5, /* uv */ 1, 0, /* N */ -1, 0, 0,
    -0.5, +0.5, +0.5, /* uv */ 1, 1, /* N */ -1, 0, 0,
    -0.5, +0.5, +0.5, /* uv */ 0, 1, /* N */ 0, 1, 0,
    -0.5, +0.5, -0.5, /* uv */ 0, 0, /* N */ 0, 1, 0,
    +0.5, +0.5, -0.5, /* uv */ 1, 0, /* N */ 0, 1, 0,
    +0.5, +0.5, +0.5, /* uv */ 1, 1, /* N */ 0, 1, 0,
    +0.5, +0.5, +0.5, /* uv */ 0, 1, /* N */ 1, 0, 0,
    +0.5, +0.5, -0.5, /* uv */ 0, 0, /* N */ 1, 0, 0,
    +0.5, -0.5, -0.5, /* uv */ 1, 0, /* N */ 1, 0, 0,
    +0.5, -0.5, +0.5, /* uv */ 1, 1, /* N */ 1, 0, 0,
    -0.5, -0.5, -0.5, /* uv */ 0, 1, /* N */ 0, -1, 0,
    -0.5, -0.5, +0.5, /* uv */ 0, 0, /* N */ 0, -1, 0,
    +0.5, -0.5, +0.5, /* uv */ 1, 0, /* N */ 0, -1, 0,
    +0.5, -0.5, -0.5, /* uv */ 1, 1, /* N */ 0, -1, 0,
  };

  static const short faces[] =
  {
    0, 2, 1,
    0, 3, 2,

    4, 6, 5,
    4, 7, 6,

    8, 10, 9,
    8, 11, 10,

    12, 14, 13,
    12, 15, 14,

    16, 18, 17,
    16, 19, 18,

    20, 22, 21,
    20, 23, 22,
  };

  Model model;

  model.vertices.assign(begin(vertices), end(vertices));
  model.faces.assign(begin(faces), end(faces));

  return model;
}

Model loadModel(string jsonPath)
{
  Model r;
  auto obj = json::load(jsonPath);
  auto dir = dirName(jsonPath);

  auto type = obj->getMember<json::String>("type")->value;
  auto actions = obj->getMember<json::Array>("actions");

  if(type != "sheet")
    throw runtime_error("Unknown model type: '" + type + "'");

  auto sheet = obj->getMember<json::String>("sheet")->value;
  auto width = obj->getMember<json::Number>("width")->value;
  auto height = obj->getMember<json::Number>("height")->value;

  auto cell = Size2i(width, height);

  for(auto& action : actions->elements)
    r.actions.push_back(loadSheetAction(action.get(), dir + "/" + sheet, cell));

  return r;
}

void Action::addTexture(string path, Rect2i rect)
{
  textures.push_back(loadTexture(path, rect));
}

