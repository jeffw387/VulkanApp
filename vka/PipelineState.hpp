#pragma once

#include "vulkan/vulkan.hpp"
#include "DeviceState.hpp"
#include "RenderState.hpp"
#include <vector>

namespace vka
{
    struct PipelineState
    {
        vk::UniquePipelineLayout pipelineLayout;
		std::vector<vk::SpecializationMapEntry> fragmentSpecializations;
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

	void CreatePipelineLayout(PipelineState& pipelineState, const DeviceState& deviceState, const ShaderState& shaderState)
	{
		auto setLayouts = std::vector<vk::DescriptorSetLayout>();
		setLayouts.push_back(shaderState.fragmentDescriptorSetLayout.get());
		auto pipelineLayoutInfo = vk::PipelineLayoutCreateInfo(
			vk::PipelineLayoutCreateFlags(),
			static_cast<uint32_t>(setLayouts.size()),
			setLayouts.data(),
			static_cast<uint32_t>(shaderState.pushConstantRanges.size()),
			shaderState.pushConstantRanges.data());
		pipelineState.pipelineLayout = deviceState.logicalDevice->createPipelineLayoutUnique(pipelineLayoutInfo);
	}

	void CreatePipeline(PipelineState& pipelineState, 
		const DeviceState& deviceState, 
		const RenderState& renderState, 
		const ShaderState& shaderState)
	{
		auto textureCount = renderState.images.size();
		pipelineState.fragmentSpecializations.emplace_back(vk::SpecializationMapEntry(
			0U,
			0U,
			sizeof(glm::uint32)));

		pipelineState.fragmentSpecializationInfo = vk::SpecializationInfo(
			static_cast<uint32_t>(pipelineState.fragmentSpecializations.size()),
			pipelineState.fragmentSpecializations.data(),
			static_cast<uint32_t>(sizeof(glm::uint32)),
			&textureCount
		);

		pipelineState.vertexShaderStageInfo = vk::PipelineShaderStageCreateInfo(
			vk::PipelineShaderStageCreateFlags(),
			vk::ShaderStageFlagBits::eVertex,
			shaderState.vertexShader.get(),
			"main",
			nullptr);

		pipelineState.fragmentShaderStageInfo = vk::PipelineShaderStageCreateInfo(
			vk::PipelineShaderStageCreateFlags(),
			vk::ShaderStageFlagBits::eFragment,
			shaderState.fragmentShader.get(),
			"main",
			&pipelineState.fragmentSpecializationInfo);

		pipelineState.pipelineShaderStageInfo = std::vector<vk::PipelineShaderStageCreateInfo>
		{
			pipelineState.vertexShaderStageInfo, 
			pipelineState.fragmentShaderStageInfo 
		};

		pipelineState.vertexBindings = Vertex::vertexBindingDescriptions();
		pipelineState.vertexAttributes = Vertex::vertexAttributeDescriptions();
		pipelineState.pipelineVertexInputInfo = vk::PipelineVertexInputStateCreateInfo(
			vk::PipelineVertexInputStateCreateFlags(),
			static_cast<uint32_t>(pipelineState.vertexBindings.size()),
			pipelineState.vertexBindings.data(),
			static_cast<uint32_t>(pipelineState.vertexAttributes.size()),
			pipelineState.vertexAttributes.data());

		pipelineState.pipelineInputAssemblyInfo = vk::PipelineInputAssemblyStateCreateInfo(
			vk::PipelineInputAssemblyStateCreateFlags(),
			vk::PrimitiveTopology::eTriangleList);

		pipelineState.pipelineTesselationStateInfo = vk::PipelineTessellationStateCreateInfo(
			vk::PipelineTessellationStateCreateFlags());

		pipelineState.pipelineViewportInfo = vk::PipelineViewportStateCreateInfo(
			vk::PipelineViewportStateCreateFlags(),
			1U,
			nullptr,
			1U,
			nullptr);

		pipelineState.pipelineRasterizationInfo = vk::PipelineRasterizationStateCreateInfo(
			vk::PipelineRasterizationStateCreateFlags(),
			false,
			false,
			vk::PolygonMode::eFill,
			vk::CullModeFlagBits::eNone,
			vk::FrontFace::eCounterClockwise,
			false,
			0.f,
			0.f,
			0.f,
			1.f);

		pipelineState.pipelineMultisampleInfo = vk::PipelineMultisampleStateCreateInfo();

		pipelineState.pipelineDepthStencilInfo = vk::PipelineDepthStencilStateCreateInfo();

		pipelineState.pipelineColorBlendAttachmentState = vk::PipelineColorBlendAttachmentState(
			true,
			vk::BlendFactor::eSrcAlpha,
			vk::BlendFactor::eOneMinusSrcAlpha,
			vk::BlendOp::eAdd,
			vk::BlendFactor::eZero,
			vk::BlendFactor::eZero,
			vk::BlendOp::eAdd,
			vk::ColorComponentFlagBits::eR |
			vk::ColorComponentFlagBits::eG |
			vk::ColorComponentFlagBits::eB |
			vk::ColorComponentFlagBits::eA);

		pipelineState.pipelineBlendStateInfo = vk::PipelineColorBlendStateCreateInfo(
			vk::PipelineColorBlendStateCreateFlags(),
			0U,
			vk::LogicOp::eClear,
			1U,
			&pipelineState.pipelineColorBlendAttachmentState);

		pipelineState.dynamicStates = std::vector<vk::DynamicState>{ vk::DynamicState::eViewport, vk::DynamicState::eScissor };

		pipelineState.pipelineDynamicStateInfo = vk::PipelineDynamicStateCreateInfo(
			vk::PipelineDynamicStateCreateFlags(),
			static_cast<uint32_t>(pipelineState.dynamicStates.size()),
			pipelineState.dynamicStates.data());

		pipelineState.pipelineCreateInfo = vk::GraphicsPipelineCreateInfo(
			vk::PipelineCreateFlags(),
			static_cast<uint32_t>(pipelineState.pipelineShaderStageInfo.size()),
			pipelineState.pipelineShaderStageInfo.data(),
			&pipelineState.pipelineVertexInputInfo,
			&pipelineState.pipelineInputAssemblyInfo,
			&pipelineState.pipelineTesselationStateInfo,
			&pipelineState.pipelineViewportInfo,
			&pipelineState.pipelineRasterizationInfo,
			&pipelineState.pipelineMultisampleInfo,
			&pipelineState.pipelineDepthStencilInfo,
			&pipelineState.pipelineBlendStateInfo,
			&pipelineState.pipelineDynamicStateInfo,
			pipelineState.pipelineLayout.get(),
			renderState.renderPass.get()
		);

		pipelineState.pipeline = deviceState.logicalDevice->createGraphicsPipelineUnique(vk::PipelineCache(), 
				pipelineState.pipelineCreateInfo);
	}
}