#pragma once

#include "vulkan/vulkan.hpp"
#include "fileIO.hpp"
#include "DeviceState.hpp"
#include "RenderState.hpp"
#include "Image2D.hpp"
#include <vector>
#include <string>

namespace vka
{
	struct FragmentPushConstants
	{
		glm::uint32 textureID;
		glm::float32 r, g, b, a;
	};

    struct ShaderState
    {
        std::string vertexShaderPath;
		std::string fragmentShaderPath;
        vk::UniqueShaderModule vertexShader;
		vk::UniqueShaderModule fragmentShader;
		vk::UniqueDescriptorSetLayout vertexDescriptorSetLayout;
		vk::UniqueDescriptorSetLayout fragmentDescriptorSetLayout;
		std::vector<vk::PushConstantRange> pushConstantRanges;
		vk::UniqueDescriptorPool fragmentLayoutDescriptorPool;
		vk::UniqueDescriptorSet fragmentDescriptorSet;
    };

	void CreateShaderModules(ShaderState& shaderState, const DeviceState& deviceState)
	{
		// read from file
		auto vertexShaderBinary = fileIO::readFile(shaderState.vertexShaderPath);
		// create shader module
		auto vertexShaderInfo = vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(),
			vertexShaderBinary.size(),
			reinterpret_cast<const uint32_t*>(vertexShaderBinary.data()));
		shaderState.vertexShader = deviceState.logicalDevice->createShaderModuleUnique(vertexShaderInfo);

		// read from file
		auto fragmentShaderBinary = fileIO::readFile(shaderState.fragmentShaderPath);
		// create shader module
		auto fragmentShaderInfo = vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(),
			fragmentShaderBinary.size(),
			reinterpret_cast<const uint32_t*>(fragmentShaderBinary.data()));
		shaderState.fragmentShader = deviceState.logicalDevice->createShaderModuleUnique(fragmentShaderInfo);
	}

	void CreateVertexDescriptorPool(ShaderState& shaderState, const DeviceState& deviceState)
	{
		auto poolSizes = std::vector<vk::DescriptorPoolSize>
		{
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1U)
		};

		shaderState.vertexDescriptorSetLayout = deviceState.logicalDevice->createDescriptorPoolUnique(
			vk::DescriptorPoolCreateInfo(
				vk::DescriptorPoolCreateFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet),
				BufferCount,
				static_cast<uint32_t>(poolSizes.size()),
				poolSizes.data()));
	}

	void CreateFragmentDescriptorPool(ShaderState& shaderState, 
		const DeviceState& deviceState, 
		const RenderState& renderState)
	{
		auto textureCount = renderState.images.size();
		auto poolSizes = std::vector<vk::DescriptorPoolSize>
		{
			vk::DescriptorPoolSize(vk::DescriptorType::eSampler, 1U),
			vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, textureCount)
		};

		shaderState.fragmentLayoutDescriptorPool = deviceState.logicalDevice->createDescriptorPoolUnique(
			vk::DescriptorPoolCreateInfo(
				vk::DescriptorPoolCreateFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet),
				1U,
				static_cast<uint32_t>(poolSizes.size()),
				poolSizes.data()));
	}

	void CreateVertexSetLayout(ShaderState& shaderState, const DeviceState& deviceState)
	{
		auto vertexLayoutBindings = std::vector<vk::DescriptorSetLayoutBinding>
		{
			// Vertex: matrix uniform buffer (dynamic)
			vk::DescriptorSetLayoutBinding(
				0U,
				vk::DescriptorType::eUniformBufferDynamic,
				1U,
				vk::ShaderStageFlagBits::eVertex)
		};

		shaderState.vertexDescriptorSetLayout = deviceState.logicalDevice->createDescriptorSetLayoutUnique(
			vk::DescriptorSetLayoutCreateInfo(
				vk::DescriptorSetLayoutCreateFlags(),
				static_cast<uint32_t>(vertexLayoutBindings.size()),
				vertexLayoutBindings.data()));
	}

	void CreateFragmentSetLayout(ShaderState& shaderState, 
		const DeviceState& deviceState, 
		const RenderState& renderState)
	{
		auto textureCount = renderState.images.size();
		auto fragmentLayoutBindings = std::vector<vk::DescriptorSetLayoutBinding>
		{
			// Fragment: sampler uniform buffer
			vk::DescriptorSetLayoutBinding(
				0U,
				vk::DescriptorType::eSampler,
				1U,
				vk::ShaderStageFlagBits::eFragment,
				&renderState.sampler),
			// Fragment: image2D uniform buffer (array)
			vk::DescriptorSetLayoutBinding(
				1U,
				vk::DescriptorType::eSampledImage,
				textureCount,
				vk::ShaderStageFlagBits::eFragment)
		};

		shaderState.fragmentDescriptorSetLayout = deviceState.logicalDevice->createDescriptorSetLayoutUnique(
			vk::DescriptorSetLayoutCreateInfo(
				vk::DescriptorSetLayoutCreateFlags(),
				static_cast<uint32_t>(fragmentLayoutBindings.size()),
				fragmentLayoutBindings.data()));
	}

	std::vector<vk::DescriptorSetLayout> GetSetLayouts(const ShaderState& shaderState)
	{
		return std::vector<vk::DescriptorSetLayout>
		{ 
			shaderState.vertexDescriptorSetLayout, 
			shaderState.fragmentDescriptorSetLayout
		};
	}

	void SetupPushConstants(ShaderState& shaderState)
	{
		shaderState.pushConstantRanges.emplace_back(vk::PushConstantRange(
			vk::ShaderStageFlagBits::eFragment,
			0U,
			sizeof(FragmentPushConstants)));
	}

	void CreateAndUpdateFragmentDescriptorSet(ShaderState& shaderState, 
		const DeviceState& deviceState, 
		const RenderState& renderState)
	{
		auto textureCount = renderState.images.size();
		// create fragment descriptor set
		shaderState.fragmentDescriptorSet = std::move(
			deviceState.logicalDevice->allocateDescriptorSetsUnique(
				vk::DescriptorSetAllocateInfo(
					shaderState.fragmentLayoutDescriptorPool.get(),
					1U,
					&shaderState.fragmentDescriptorSetLayout.get()))[0]);

		// update fragment descriptor set
			// binding 0 (sampler)
		auto descriptorImageInfo = vk::DescriptorImageInfo(renderState.sampler.get());
		deviceState.logicalDevice->updateDescriptorSets(
			{
				vk::WriteDescriptorSet(
					shaderState.fragmentDescriptorSet.get(),
					0U,
					0U,
					1U,
					vk::DescriptorType::eSampler,
					&descriptorImageInfo)
			},
			{}
		);
			// binding 1 (images)
		
		auto imageInfos = std::vector<vk::DescriptorImageInfo>();
		imageInfos.reserve(textureCount);
		for (Image2D& image : renderState.images)
		{
			imageInfos.emplace_back(vk::DescriptorImageInfo(
				nullptr,
				image.view,
				vk::ImageLayout::eShaderReadOnlyOptimal));
		}

		deviceState.logicalDevice->updateDescriptorSets(
			{
				vk::WriteDescriptorSet(
					shaderState.fragmentDescriptorSet.get(),
					1U,
					0U,
					static_cast<uint32_t>(imageInfos.size()),
					vk::DescriptorType::eSampledImage,
					imageInfos.data())
			},
			{}
		);
	}
}