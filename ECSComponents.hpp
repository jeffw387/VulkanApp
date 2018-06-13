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

    struct CollisionMesh
    {
        
    };

    // do collision testing for each awake object
    // add velocity where needed from collisions and wake objects that have velocity added
    // add velocity to physics entities based on ai or player input
    // wake physics entities if velocity was added
    // simulate motion for awake objects
    // update transform for awake objects
    // apply drag to awake objects

    struct Velocity
    {
        glm::vec2 velocity;
        bool awake = false;
    };

    struct PlayerControl
    {
    };

    struct Engine
    {
        glm::vec2 thrustDirection;

        Engine() noexcept = default;
        Engine(glm::vec2 thrustDirection) : thrustDirection(std::move(thrustDirection)) {}
    };
}
