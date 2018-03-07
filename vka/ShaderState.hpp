#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

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
		std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
		std::vector<vk::PushConstantRange> pushConstantRanges;
		vk::UniqueDescriptorPool fragmentLayoutDescriptorPool;
		vk::UniqueDescriptorSet fragmentDescriptorSet;
    };
}