#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

namespace vka
{
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