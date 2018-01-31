#pragma once
#include "glm/glm.hpp"

struct Sprite
{
	uint32_t textureIndex;
	glm::mat4 transform;
	glm::vec4 color;
};