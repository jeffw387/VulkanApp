#pragma once

#include "vulkan/vulkan.h"
#include "glm/glm.hpp"
#include <vector>
#include <array>

namespace vka
{
    struct Vertex
    {
        glm::vec2 Position;
        glm::vec2 UV;
        static std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions()
        {
            VkVertexInputBindingDescription bindingDescription = {};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            auto bindingDescriptions = std::vector<VkVertexInputBindingDescription>
            { 
                bindingDescription
            };
            return bindingDescriptions;
        }

        static std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions()
        {
            // Binding both attributes to one buffer, interleaving Position and UV
            VkVertexInputAttributeDescription attrib0 = {};
            attrib0.location = 0;
            attrib0.binding = 0;
            attrib0.format = VK_FORMAT_R32G32_SFLOAT;
            attrib0.offset = offsetof(Vertex, Position);
            
            VkVertexInputAttributeDescription attrib1 = {};
            attrib1.location = 1;
            attrib1.binding = 0;
            attrib1.format = VK_FORMAT_R32G32_SFLOAT;
            attrib1.offset = offsetof(Vertex, UV);
            
            return std::vector<VkVertexInputAttributeDescription>{ attrib0, attrib1 };
        }
    };
}