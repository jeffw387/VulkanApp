#pragma once
#include "ECSComponents.hpp"

struct PhysicsSettings
{
    float maxSpeedSquare;
    float acceleration;
    float deceleration;
    float sleepSpeed;
};

void ProcessPhysics(
    const PhysicsSettings& settings, 
    cmp::Velocity& velocity, 
    cmp::Position& position, 
    cmp::Transform& transform)
{
    if (forces.awake)
    {
        velocity.velocity += forces.combined;
        forces.combined = glm::vec2();

    }

    position.position += velocity.velocity;
    transform.matrix = glm::translate(
        glm::mat4(1.f), 
        glm::vec3(position.position.x,
            position.position.y,
            0.f));
}