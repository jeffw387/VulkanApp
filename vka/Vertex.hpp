#pragma once

#include <array>
#include <vector>
#include "glm/glm.hpp"
#include "vulkan/vulkan.h"

namespace vka {
struct Vertex2D {
  glm::vec2 Position;
  glm::vec2 UV;
};
}  // namespace vka