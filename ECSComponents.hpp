#pragma once
#include "Texture2D.hpp"
#include "glm/glm.hpp"
#include <optional>

namespace cmp
{
    struct TextureID
    {
        TextureIndex index;

        TextureID() noexcept = default;
        TextureID(TextureIndex index) : index(std::move(index)) {}
    };

    struct Position
    {
        glm::vec3 position;

        Position() noexcept = default;
        Position(glm::vec3 position) : position(std::move(position)) {}
    };

    struct PositionMatrix
    {
        std::optional<glm::mat4> matrix;

        PositionMatrix() noexcept = default;
        PositionMatrix(glm::mat4 matrix) : matrix(std::move(matrix)) {}
    };

    struct RectSize
    {
        float width;
        float height;

        RectSize() noexcept = default;
        RectSize(float width, float height) : width(std::move(width)), height(std::move(height)) {}
    };

    struct Color
    {
        glm::vec4 rgba;

        Color() noexcept = default;
        Color(glm::vec4 rgba) : rgba(std::move(rgba)) {}
    };

    struct Velocity
    {
        glm::vec2 velocity;

        Velocity() noexcept = default;
        Velocity(glm::vec2 velocity) : velocity(std::move(velocity)) {}
    };
}
