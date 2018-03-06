#pragma once
#include "glm/glm.hpp"
#include "Image2D.hpp"
#include "entt.hpp"
#include "Vertex.hpp"

namespace vka
{
	using SpriteIndex = uint32_t;

	struct Sprite
	{
		vk::Image image;
		std::vector<Vertex> vertices;
	};
	// vertices:
	// Vertex[4];

	// indices:
	// Index[6];
}