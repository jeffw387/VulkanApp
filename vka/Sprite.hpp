#pragma once
#include "glm/glm.hpp"
#include "Image2D.hpp"
#include "entt.hpp"
#include "Vertex.hpp"

namespace vka
{
	constexpr auto VerticesPerQuad = 6U;
	using QuadVertices = std::array<Vertex, VerticesPerQuad>;

	struct Quad
	{
		QuadVertices vertices;
	};

	static Quad MakeQuad(float left, float top, float right, float bottom, float leftUV, float topUV, float rightUV, float bottomUV)
	{
		Quad quad;
		Vertex LT, LB, RB, RT;
		LT.Position = glm::vec2(left, top);
		LT.UV = glm::vec2(leftUV, topUV);

		LB.Position = glm::vec2(left, bottom);
		LB.UV = glm::vec2(leftUV, bottomUV);

		RB.Position = glm::vec2(right, bottom);
		RB.UV = glm::vec2(rightUV, bottomUV);

		RT.Position = glm::vec2(right, top);
		RT.UV = glm::vec2(rightUV, topUV);

		quad.vertices[0] = LT;
		quad.vertices[1] = LB;
		quad.vertices[2] = RB;
		quad.vertices[3] = RB;
		quad.vertices[4] = RT;
		quad.vertices[5] = LT;

		return quad;
	}

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