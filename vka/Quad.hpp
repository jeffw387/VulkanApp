#pragma once

namespace vka
{
	struct Quad
	{
		std::array<glm::vec2, 12> vertices;
		size_t firstVertex;
	};
}