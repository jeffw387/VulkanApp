#pragma once

#include "vulkan/vulkan.h"

#include <vector>

namespace vka
{
    class VertexData
    {
    public:
        VertexData(std::vector<VkVertexInputBindingDescription>&& vertexBindings,
		    std::vector<VkVertexInputAttributeDescription>&& vertexAttributes)
            :
            vertexBindings(std::move(vertexBindings)),
            vertexAttributes(std::move(vertexAttributes))
        {
            CreateVertexInputInfo();
            CreateInputAssemblyInfo();
        }

        VertexData() noexcept = default;

        const VkPipelineVertexInputStateCreateInfo& GetVertexInputInfo()
        {
            return vertexInputInfo;
        }

        const VkPipelineInputAssemblyStateCreateInfo& GetInputAssemblyInfo()
        {
            return inputAssemblyInfo;
        }

    private:
        std::vector<VkVertexInputBindingDescription> vertexBindings;
		std::vector<VkVertexInputAttributeDescription> vertexAttributes;
		VkPipelineVertexInputStateCreateInfo vertexInputInfo;
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;

        void CreateVertexInputInfo()
        {
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertexInputInfo.pNext = nullptr;
            vertexInputInfo.flags = 0;
            vertexInputInfo.vertexBindingDescriptionCount = vertexBindings.size();
            vertexInputInfo.pVertexBindingDescriptions = vertexBindings.data();
            vertexInputInfo.vertexAttributeDescriptionCount = vertexAttributes.size();
            vertexInputInfo.pVertexAttributeDescriptions = vertexAttributes.data();
        }

        void CreateInputAssemblyInfo()
        {
            inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            inputAssemblyInfo.pNext = nullptr;
            inputAssemblyInfo.flags = 0;
            inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            inputAssemblyInfo.primitiveRestartEnable = false;
        }
    };
}