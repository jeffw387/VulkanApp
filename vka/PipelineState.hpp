#pragma once

#include "UniqueVulkan.hpp"
#include "vulkan/vulkan.h"
#include "VulkanFunctions.hpp"
#include "DeviceState.hpp"
#include "RenderState.hpp"
#include <vector>

namespace vka
{
    struct PipelineState
    {
        VkPipelineLayoutUnique pipelineLayout;
		VkSpecializationMapEntry fragmentSpecMapEntry;
		VkSpecializationInfo fragmentSpecializationInfo;
		VkPipelineShaderStageCreateInfo vertexShaderStageInfo;
		VkPipelineShaderStageCreateInfo fragmentShaderStageInfo;
		std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStageInfo;
		std::vector<VkVertexInputBindingDescription> vertexBindings;
		std::vector<VkVertexInputAttributeDescription> vertexAttributes;
		VkPipelineVertexInputStateCreateInfo pipelineVertexInputInfo;
		VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyInfo;
		VkPipelineTessellationStateCreateInfo pipelineTesselationStateInfo;
		VkPipelineViewportStateCreateInfo pipelineViewportInfo;
		VkPipelineRasterizationStateCreateInfo pipelineRasterizationInfo;
		VkPipelineMultisampleStateCreateInfo pipelineMultisampleInfo;
		VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilInfo;
		VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState;
		VkPipelineColorBlendStateCreateInfo pipelineBlendStateInfo;
		std::vector<VkDynamicState> dynamicStates;
		VkPipelineDynamicStateCreateInfo pipelineDynamicStateInfo;
		VkGraphicsPipelineCreateInfo pipelineCreateInfo;
        VkPipelineUnique pipeline;
    };

	void CreatePipelineLayout(PipelineState& pipelineState, const DeviceState& deviceState, const ShaderState& shaderState)
	{
		auto device = deviceState.device.get();
		VkDescriptorSetLayout fragmentLayout = shaderState.fragmentDescriptorSetLayout.get();
		
		VkPipelineLayout layout;
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.pNext = nullptr;
		pipelineLayoutInfo.flags = 0;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &fragmentLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &shaderState.pushConstantRange;
		vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &layout);
		VkPipelineLayoutDeleter layoutDeleter;
		layoutDeleter.device = device;
		pipelineState.pipelineLayout = VkPipelineLayoutUnique(layout, layoutDeleter);
	}

	void CreatePipeline(PipelineState& pipelineState, 
		const DeviceState& deviceState, 
		const RenderState& renderState, 
		const ShaderState& shaderState)
	{
		auto textureCount = renderState.images.size();
		auto device = deviceState.device.get();

		pipelineState.fragmentSpecMapEntry.constantID = 0;
		pipelineState.fragmentSpecMapEntry.offset = 0;
		pipelineState.fragmentSpecMapEntry.size = sizeof(glm::uint32);

		pipelineState.fragmentSpecializationInfo.mapEntryCount = 1;
		pipelineState.fragmentSpecializationInfo.pMapEntries = &pipelineState.fragmentSpecMapEntry;
		pipelineState.fragmentSpecializationInfo.dataSize = sizeof(glm::uint32);
		pipelineState.fragmentSpecializationInfo.pData = &textureCount;

		pipelineState.vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		pipelineState.vertexShaderStageInfo.pNext = nullptr;
		pipelineState.vertexShaderStageInfo.flags = 0;
		pipelineState.vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		pipelineState.vertexShaderStageInfo.module = shaderState.vertexShader.get();
		pipelineState.vertexShaderStageInfo.pName = "main";
		pipelineState.vertexShaderStageInfo.pSpecializationInfo = nullptr;

		pipelineState.fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		pipelineState.fragmentShaderStageInfo.pNext = nullptr;
		pipelineState.fragmentShaderStageInfo.flags = 0;
		pipelineState.fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		pipelineState.fragmentShaderStageInfo.module = shaderState.fragmentShader.get();
		pipelineState.fragmentShaderStageInfo.pName = "main";
		pipelineState.fragmentShaderStageInfo.pSpecializationInfo = &pipelineState.fragmentSpecializationInfo;

		pipelineState.pipelineShaderStageInfo = std::vector<VkPipelineShaderStageCreateInfo>
		{
			pipelineState.vertexShaderStageInfo, 
			pipelineState.fragmentShaderStageInfo 
		};

		pipelineState.vertexBindings = Vertex::vertexBindingDescriptions();
		pipelineState.vertexAttributes = Vertex::vertexAttributeDescriptions();
		pipelineState.pipelineVertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		pipelineState.pipelineVertexInputInfo.pNext = nullptr;
		pipelineState.pipelineVertexInputInfo.flags = 0;
		pipelineState.pipelineVertexInputInfo.vertexBindingDescriptionCount = pipelineState.vertexBindings.size();
		pipelineState.pipelineVertexInputInfo.pVertexBindingDescriptions = pipelineState.vertexBindings.data();
		pipelineState.pipelineVertexInputInfo.vertexAttributeDescriptionCount = pipelineState.vertexAttributes.size();
		pipelineState.pipelineVertexInputInfo.pVertexAttributeDescriptions = pipelineState.vertexAttributes.data();

		pipelineState.pipelineInputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		pipelineState.pipelineInputAssemblyInfo.pNext = nullptr;
		pipelineState.pipelineInputAssemblyInfo.flags = 0;
		pipelineState.pipelineInputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		pipelineState.pipelineInputAssemblyInfo.primitiveRestartEnable = false;

		pipelineState.pipelineTesselationStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
		pipelineState.pipelineTesselationStateInfo.pNext = nullptr;
		pipelineState.pipelineTesselationStateInfo.flags = 0;
		pipelineState.pipelineTesselationStateInfo.patchControlPoints = 0;

		pipelineState.pipelineViewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		pipelineState.pipelineViewportInfo.pNext = nullptr;
		pipelineState.pipelineViewportInfo.flags = 0;
		pipelineState.pipelineViewportInfo.viewportCount = 1;
		pipelineState.pipelineViewportInfo.pViewports = nullptr;
		pipelineState.pipelineViewportInfo.scissorCount = 1;
		pipelineState.pipelineViewportInfo.pScissors = nullptr;

		pipelineState.pipelineRasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		pipelineState.pipelineRasterizationInfo.pNext = nullptr;
		pipelineState.pipelineRasterizationInfo.flags = 0;
		pipelineState.pipelineRasterizationInfo.depthClampEnable = false;
		pipelineState.pipelineRasterizationInfo.rasterizerDiscardEnable = false;
		pipelineState.pipelineRasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
		pipelineState.pipelineRasterizationInfo.cullMode = VK_CULL_MODE_NONE;
		pipelineState.pipelineRasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		pipelineState.pipelineRasterizationInfo.depthBiasEnable = false;
		pipelineState.pipelineRasterizationInfo.depthBiasConstantFactor = 0.f;
		pipelineState.pipelineRasterizationInfo.depthBiasClamp = 0.f;
		pipelineState.pipelineRasterizationInfo.depthBiasSlopeFactor = 0.f;
		pipelineState.pipelineRasterizationInfo.lineWidth = 1.f;

		pipelineState.pipelineMultisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		pipelineState.pipelineMultisampleInfo.pNext = nullptr;
		pipelineState.pipelineMultisampleInfo.flags = 0;
		pipelineState.pipelineMultisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		pipelineState.pipelineMultisampleInfo.sampleShadingEnable = false;
		pipelineState.pipelineMultisampleInfo.minSampleShading = 0.f;
		pipelineState.pipelineMultisampleInfo.pSampleMask = nullptr;
		pipelineState.pipelineMultisampleInfo.alphaToCoverageEnable = false;
		pipelineState.pipelineMultisampleInfo.alphaToOneEnable = false;

		pipelineState.pipelineDepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		pipelineState.pipelineDepthStencilInfo.pNext = nullptr;
		pipelineState.pipelineDepthStencilInfo.flags = 0;
		pipelineState.pipelineDepthStencilInfo.depthTestEnable = false;
		pipelineState.pipelineDepthStencilInfo.depthWriteEnable = false;
		pipelineState.pipelineDepthStencilInfo.depthCompareOp = VK_COMPARE_OP_ALWAYS;
		pipelineState.pipelineDepthStencilInfo.depthBoundsTestEnable = false;
		pipelineState.pipelineDepthStencilInfo.stencilTestEnable = false;
		pipelineState.pipelineDepthStencilInfo.front = {};
		pipelineState.pipelineDepthStencilInfo.back = {};
		pipelineState.pipelineDepthStencilInfo.minDepthBounds = 0.f;
		pipelineState.pipelineDepthStencilInfo.maxDepthBounds = 0.f;

		pipelineState.pipelineColorBlendAttachmentState.blendEnable = true;
		pipelineState.pipelineColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		pipelineState.pipelineColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		pipelineState.pipelineColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		pipelineState.pipelineColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		pipelineState.pipelineColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		pipelineState.pipelineColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		pipelineState.pipelineColorBlendAttachmentState.colorWriteMask = 
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;

		pipelineState.pipelineBlendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		pipelineState.pipelineBlendStateInfo.pNext = nullptr;
		pipelineState.pipelineBlendStateInfo.flags = 0;
		pipelineState.pipelineBlendStateInfo.logicOpEnable = false;
		pipelineState.pipelineBlendStateInfo.logicOp = VK_LOGIC_OP_CLEAR;
		pipelineState.pipelineBlendStateInfo.attachmentCount = 1;
		pipelineState.pipelineBlendStateInfo.pAttachments = &pipelineState.pipelineColorBlendAttachmentState;
		pipelineState.pipelineBlendStateInfo.blendConstants[0] = 1.f;
		pipelineState.pipelineBlendStateInfo.blendConstants[1] = 1.f;
		pipelineState.pipelineBlendStateInfo.blendConstants[2] = 1.f;
		pipelineState.pipelineBlendStateInfo.blendConstants[3] = 1.f;

		pipelineState.dynamicStates = std::vector<VkDynamicState>{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		pipelineState.pipelineDynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		pipelineState.pipelineDynamicStateInfo.pNext = nullptr;
		pipelineState.pipelineDynamicStateInfo.flags = 0;
		pipelineState.pipelineDynamicStateInfo.dynamicStateCount = pipelineState.dynamicStates.size();
		pipelineState.pipelineDynamicStateInfo.pDynamicStates = pipelineState.dynamicStates.data();

		pipelineState.pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineState.pipelineCreateInfo.pNext = nullptr;
		pipelineState.pipelineCreateInfo.flags = 0;
		pipelineState.pipelineCreateInfo.stageCount = pipelineState.pipelineShaderStageInfo.size();
		pipelineState.pipelineCreateInfo.pStages = pipelineState.pipelineShaderStageInfo.data();
		pipelineState.pipelineCreateInfo.pVertexInputState = &pipelineState.pipelineVertexInputInfo;
		pipelineState.pipelineCreateInfo.pInputAssemblyState = &pipelineState.pipelineInputAssemblyInfo;
		pipelineState.pipelineCreateInfo.pTessellationState = &pipelineState.pipelineTesselationStateInfo;
		pipelineState.pipelineCreateInfo.pViewportState = &pipelineState.pipelineViewportInfo;
		pipelineState.pipelineCreateInfo.pRasterizationState = &pipelineState.pipelineRasterizationInfo;
		pipelineState.pipelineCreateInfo.pMultisampleState = &pipelineState.pipelineMultisampleInfo;
		pipelineState.pipelineCreateInfo.pDepthStencilState = &pipelineState.pipelineDepthStencilInfo;
		pipelineState.pipelineCreateInfo.pColorBlendState = &pipelineState.pipelineBlendStateInfo;
		pipelineState.pipelineCreateInfo.pDynamicState = &pipelineState.pipelineDynamicStateInfo;
		pipelineState.pipelineCreateInfo.layout = pipelineState.pipelineLayout.get();
		pipelineState.pipelineCreateInfo.renderPass = renderState.renderPass.get();
		pipelineState.pipelineCreateInfo.subpass = 0;
		pipelineState.pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineState.pipelineCreateInfo.basePipelineIndex = 0;
		
		VkPipeline pipeline;
		VkPipelineDeleter pipelineDeleter;
		pipelineDeleter.device = device;
		vkCreateGraphicsPipelines(
			device,
			VK_NULL_HANDLE,
			1,
			&pipelineState.pipelineCreateInfo,
			nullptr,
			&pipeline);
		pipelineState.pipeline = VkPipelineUnique(pipeline, pipelineDeleter);
	}
}