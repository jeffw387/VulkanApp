#pragma once
#include "vka/Image2D.hpp"
#include "glm/glm.hpp"
#include "TimeHelper.hpp"
#include "vka/Input.hpp"
#include <optional>

namespace cmp
{
    struct Sprite
    {
        uint64_t index;

        Sprite() noexcept = default;
        Sprite(uint64_t index) : index(std::move(index)) {}
    };

    struct Position
    {
        glm::vec2 position;

        Position() noexcept = default;
        Position(glm::vec2 position) : position(std::move(position)) {}
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

    struct PlayerControl
    {
        std::chrono::milliseconds up;
        std::chrono::milliseconds down;
        std::chrono::milliseconds right;
        std::chrono::milliseconds left;
        std::chrono::milliseconds fire;
    };
}
