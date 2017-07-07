#pragma once
#include <vulkan/vulkan.h>
#include <array>
#include <glm/glm.hpp>

// descriptor set layouts
struct SamplerAndImagesLayout
{
	std::array<VkDescriptorSetLayoutBinding, 2> bindings = {};
	VkDescriptorSetLayout layout;

	void init(VkDevice device)
	{
		// Sampler
		bindings[0].binding = 0;
		bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		bindings[0].descriptorCount = 1; 
		bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[0].pImmutableSamplers = nullptr; // TODO: point to the sampler for the textures

		bindings[1].binding = 1;
		bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		bindings[1].descriptorCount = 0; // TODO: match this to number of textures
		bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[1].pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create sampler and images set layout!");
		}
	}
};
// TODO: measure whether it's faster to multiply M with VP on CPU or GPU
struct MVPMatricesLayout
{
	std::array<VkDescriptorSetLayoutBinding, 1> bindings = {};
	VkDescriptorSetLayout layout;

	void init(VkDevice device)
	{
		bindings[0].binding = 0;
		bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		bindings[0].descriptorCount = 1;
		bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		bindings[0].pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create MVP matrices set layout!");
		}
	}
};

struct ModelIndexPushConstant
{
	VkPushConstantRange range = {};

	void init()
	{
		range.offset = 0;
		range.size = sizeof(uint32_t);
		range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	}
};

struct PipelineLayout
{
	VkPipelineLayout layout;

	SamplerAndImagesLayout samplerImageLayout;
	MVPMatricesLayout mvpLayout;
	ModelIndexPushConstant pushConstant;

	void init(VkDevice device)
	{
		// layout init
		samplerImageLayout.init(device);
		mvpLayout.init(device);
		const uint32_t layoutCount = 2;
		VkDescriptorSetLayout layouts[layoutCount] = { samplerImageLayout.layout, mvpLayout.layout };

		// push constant init
		pushConstant.init();
		const uint32_t pushconstantCount = 1;
		VkPushConstantRange pushConstants[pushconstantCount] = { pushConstant.range };
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = layoutCount;
		pipelineLayoutInfo.pSetLayouts = layouts;
		pipelineLayoutInfo.pushConstantRangeCount = pushconstantCount;
		pipelineLayoutInfo.pPushConstantRanges = pushConstants;

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}
	}
};

// Vertex Binding data
struct Vertex
{
	// get binding descriptions
	static std::array<VkVertexInputBindingDescription, 2> getBindingDescription()
	{
		// position binding
		std::array<VkVertexInputBindingDescription, 2> bindingDescriptions;
		bindingDescriptions[0] = {};
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(glm::vec3);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		// texture coordinate binding
		bindingDescriptions[1] = {};
		bindingDescriptions[1].binding = 1;
		bindingDescriptions[1].stride = sizeof(glm::vec2);
		bindingDescriptions[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescriptions;
	}

	// get attribute descriptions
	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};

		// position attribute
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = 0;

		// texture coordinate attribute
		attributeDescriptions[1].binding = 1;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[1].offset = 0;

		return attributeDescriptions;
	}
};