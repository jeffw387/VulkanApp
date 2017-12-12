#pragma once

#include "components.hpp"
#include "stx/btree_map.h"

using Entity = size_t;

using SpriteTree = stx::btree_map<Entity, Sprite>;