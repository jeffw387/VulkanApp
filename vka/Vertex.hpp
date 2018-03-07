#pragma once

#include "vulkan/vulkan.hpp"
#include "glm/glm.hpp"
#include <vector>
#include <array>

namespace vka
{
    using VertexIndex = uint16_t;
    constexpr VertexIndex IndicesPerQuad = 6U;
	constexpr std::array<VertexIndex, IndicesPerQuad> IndexArray = 
	{
		0, 1, 2, 2, 3, 0
	};

    struct Vertex
    {
        glm::vec2 Position;
        glm::vec2 UV;
        static std::vector<vk::VertexInputBindingDescription> vertexBindingDescriptions()
        {
            auto bindingDescriptions = std::vector<vk::VertexInputBindingDescription>
            { 
                vk::VertexInputBindingDescription(0U, sizeof(Vertex), vk::VertexInputRate::eVertex) 
            };
            return bindingDescriptions;
        }

        static std::vector<vk::VertexInputAttributeDescription> vertexAttributeDescriptions()
        {
            // Binding both attributes to one buffer, interleaving Position and UV
            auto attrib0 = vk::VertexInputAttributeDescription(0U, 0U, vk::Format::eR32G32Sfloat, offsetof(Vertex, Position));
            auto attrib1 = vk::VertexInputAttributeDescription(1U, 0U, vk::Format::eR32G32Sfloat, offsetof(Vertex, UV));
            return std::vector<vk::VertexInputAttributeDescription>{ attrib0, attrib1 };
        }
    };
}