#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

namespace vka
{
    struct PipelineState
    {
        vk::UniquePipelineLayout pipelineLayout;
		std::vector<vk::SpecializationMapEntry> vertexSpecializations;
		std::vector<vk::SpecializationMapEntry> fragmentSpecializations;
		vk::SpecializationInfo vertexSpecializationInfo;
		vk::SpecializationInfo fragmentSpecializationInfo;
		vk::PipelineShaderStageCreateInfo vertexShaderStageInfo;
		vk::PipelineShaderStageCreateInfo fragmentShaderStageInfo;
		std::vector<vk::PipelineShaderStageCreateInfo> pipelineShaderStageInfo;
		std::vector<vk::VertexInputBindingDescription> vertexBindings;
		std::vector<vk::VertexInputAttributeDescription> vertexAttributes;
		vk::PipelineVertexInputStateCreateInfo pipelineVertexInputInfo;
		vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyInfo;
		vk::PipelineTessellationStateCreateInfo pipelineTesselationStateInfo;
		vk::PipelineViewportStateCreateInfo pipelineViewportInfo;
		vk::PipelineRasterizationStateCreateInfo pipelineRasterizationInfo;
		vk::PipelineMultisampleStateCreateInfo pipelineMultisampleInfo;
		vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilInfo;
		vk::PipelineColorBlendAttachmentState pipelineColorBlendAttachmentState;
		vk::PipelineColorBlendStateCreateInfo pipelineBlendStateInfo;
		std::vector<vk::DynamicState> dynamicStates;
		vk::PipelineDynamicStateCreateInfo pipelineDynamicStateInfo;
		vk::GraphicsPipelineCreateInfo pipelineCreateInfo;
        vk::UniquePipeline pipeline;
    };
}