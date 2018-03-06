#pragma once

#include <vulkan/vulkan.hpp>

namespace vka
{
    struct RenderState
    {
        Camera2D camera;
        uint32_t nextImage;
        glm::mat4 vp;
        vk::DeviceSize copyOffset;
        void* mapped;
        SpriteIndex spriteIndex;
    };
}