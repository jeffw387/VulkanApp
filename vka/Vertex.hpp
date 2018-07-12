#pragma once

#include "vulkan/vulkan.h"
#include "glm/glm.hpp"
#include <vector>
#include <array>

namespace vka
{
    struct Vertex2D
    {
        glm::vec2 Position;
        glm::vec2 UV;
    };
}