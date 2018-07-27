#pragma once
#include "vka/Image2D.hpp"
#include "glm/glm.hpp"
#include "TimeHelper.hpp"
#include "vka/Input.hpp"
#include <optional>

namespace cmp
{
	struct Drawable
	{};

	struct Light
	{};

	struct Material
	{
		uint64_t index;
	};

    struct Sprite
    {
        uint64_t index;

        Sprite() noexcept = default;
        Sprite(uint64_t index) : index(std::move(index)) {}
    };

    struct Transform
    {
        glm::mat4 matrix;

        Transform() noexcept = default;
        Transform(glm::mat4 matrix) : matrix(std::move(matrix)) {}

		operator const glm::mat4&() const
		{
			return matrix;
		}

		operator glm::mat4&()
		{
			return matrix;
		}
    };

    struct Color
    {
        glm::vec4 rgba;

        Color() noexcept = default;
        Color(glm::vec4 rgba) : rgba(std::move(rgba)) {}

		operator const glm::vec4&() const
		{
			return rgba;
		}

		operator glm::vec4&()
		{
			return rgba;
		}
    };

    struct PlayerControl
    {
    };
}
