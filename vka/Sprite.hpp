#pragma once
#include "vulkan/vulkan.hpp"
#include "glm/glm.hpp"
#include "Image2D.hpp"
#include "entt.hpp"
#include "Vertex.hpp"

namespace vka
{
	using SpriteIndex = uint32_t;
	constexpr auto VerticesPerQuad = 6U;
	using QuadVertices = std::array<Vertex, VerticesPerQuad>;

	struct Quad
	{
		QuadVertices vertices;
	};

	struct Sprite
	{
		SpriteIndex index;
		ImageIndex imageIndex;
	};
}