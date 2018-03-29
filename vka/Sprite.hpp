#pragma once
#include "vulkan/vulkan.hpp"
#include "glm/glm.hpp"
#include "Image2D.hpp"
#include "entt.hpp"
#include "Vertex.hpp"

namespace vka
{
	using SpriteIndex = entt::HashedString;
	constexpr auto VerticesPerQuad = 6U;
	using QuadVertices = std::array<Vertex, VerticesPerQuad>;

	struct Quad
	{
		QuadVertices vertices;
	};

	struct Sprite
	{
		uint64_t imageID;
		size_t imageOffset;
		size_t vertexOffset;
		Quad quad;
	};

	static Quad CreateQuad(float left, float top, float width, float height, float imageWidth, float imageHeight)
		{
			float right  = 	left + width;
			float bottom = 	top + height;

			glm::vec2 LeftTopPos, LeftBottomPos, RightBottomPos, RightTopPos;
			LeftTopPos     = { left,		top };
			LeftBottomPos  = { left,		bottom };
			RightBottomPos = { right,		bottom };
			RightTopPos    = { right,		top };

			float leftUV = left / imageWidth;
			float rightUV = right / imageWidth;
			float topUV = top / imageHeight;
			float bottomUV = bottom / imageHeight;

			glm::vec2 LeftTopUV, LeftBottomUV, RightBottomUV, RightTopUV;
			LeftTopUV     = { leftUV, topUV };
			LeftBottomUV  = { leftUV, bottomUV };
			RightBottomUV = { rightUV, bottomUV };
			RightTopUV    = { rightUV, topUV };

			Vertex LeftTop, LeftBottom, RightBottom, RightTop;
			LeftTop 	= { LeftTopPos,		LeftTopUV 		};
			LeftBottom  = { LeftBottomPos,	LeftBottomUV 	};
			RightBottom = { RightBottomPos,	RightBottomUV 	};
			RightTop 	= { RightTopPos,	RightTopUV 		};

			// push back vertices
			Quad quad;
			quad.vertices[0] = LeftTop;
			quad.vertices[1] = LeftBottom;
			quad.vertices[2] = RightBottom;
			quad.vertices[3] = RightBottom;
			quad.vertices[4] = RightTop;
			quad.vertices[5] = LeftTop;

			return quad;
		}
}