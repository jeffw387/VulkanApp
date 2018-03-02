#pragma once
#include "glm/glm.hpp"
#include "Texture2D.hpp"

struct Sprite
{
	TextureIndex textureIndex;
	glm::mat4 transform;
	glm::vec4 color;
};

struct SpriteFromSheet
{
	TextureIndex textureIndex;
};