#pragma once
#include "vulkan/vulkan.hpp"
#include "glm/glm.hpp"
#include "Image2D.hpp"
#include "entt.hpp"
#include "Vertex.hpp"

namespace vka
{
	using SpriteIndex = uint32_t;
	constexpr auto VerticesPerQuad = 4U;
	struct Quad
	{
		std::array<Vertex, VerticesPerQuad> vertices;

		static std::array<VertexIndex, IndicesPerQuad> getIndices(SpriteIndex spriteIndex)
		{
			auto result = IndexArray;
			auto offset = spriteIndex * VerticesPerQuad;
			for (auto& index : result)
			{
				index += offset;
			}
			return result;
		}
	};

	struct Sprite
	{
		vk::Image image;
		Quad quad;
	};
}