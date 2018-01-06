#pragma once
#include <vulkan/vulkan.h>
#include <array>
#include <vector>
#include <glm.hpp>

using ImageIndex = uint32_t;

namespace VulkanData
{
	const auto TextureCount = 2U;

	struct MVPMatricesDescriptors
	{
		std::vector<VkDescriptorSetLayoutBinding> layoutBindings() 
		{ 
			return std::vector<VkDescriptorSetLayoutBinding>{ binding2_transformMatrixDynamic()}; 
		}

		std::vector<VkDescriptorPoolSize> poolSizes()
		{ 
			return std::vector<VkDescriptorPoolSize> { poolSize }; 
		}

	private:
		// Binding 0
		VkDescriptorSetLayoutBinding binding2_transformMatrixDynamic()
		{
			return VkDescriptorSetLayoutBinding{
			0,														// binding
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,				// descriptor type
			1,														// descriptor count
			VK_SHADER_STAGE_VERTEX_BIT,								// shader stage
			nullptr,												// immutable samplers
			};
		};

		VkDescriptorPoolSize poolSize = 
		{
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,				// descriptor type
			1														// descriptor count
		};
	};

	// descriptor set layouts
	struct SamplerAndImagesDescriptors
	{
		std::vector<VkDescriptorSetLayoutBinding> layoutBindings()
		{
			return std::vector<VkDescriptorSetLayoutBinding>
			{
				binding0_sampler(),
				binding1_sampledImages()
			};
		}

		std::vector<VkDescriptorPoolSize> poolSizes() 
		{ 
			return std::vector<VkDescriptorPoolSize> { poolSize0, poolSize1, poolSize2 }; 
		}

	private:
		// Binding 0: 1 sampler
		VkDescriptorSetLayoutBinding binding0_sampler()
		{
			VkDescriptorSetLayoutBinding binding = {
			0,														// binding
			VK_DESCRIPTOR_TYPE_SAMPLER,								// descriptor type
			1, 														// descriptor count
			VK_SHADER_STAGE_FRAGMENT_BIT,							// shader stage
			nullptr,												// immutable samplers
			};
			return binding;
		}
		VkDescriptorPoolSize poolSize0 = 
		{
			VK_DESCRIPTOR_TYPE_SAMPLER,								// descriptor type
			1														// descriptor count
		};

		// Binding 1: array of TextureCount textures
		VkDescriptorSetLayoutBinding binding1_sampledImages()
		{
			VkDescriptorSetLayoutBinding binding = {
				1,													// binding
				VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,					// descriptor type
				TextureCount,										// descriptor count
				VK_SHADER_STAGE_FRAGMENT_BIT,						// shader stage
				nullptr,											// immutable samplers
			};
			return binding;
		}
		VkDescriptorPoolSize poolSize1 =
		{
			VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,						// descriptor type
			TextureCount											// descriptor count
		};

		// Binding 2: uniform vec3 textColor
		VkDescriptorSetLayoutBinding binding2_textColor()
		{
			VkDescriptorSetLayoutBinding binding = {
				2,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				1,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				nullptr
			};
			return binding;
		}
		VkDescriptorPoolSize poolSize2 =
		{
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			1
		};
	};
	struct pushConstantRange1
	{
		glm::uint index;
		glm::float32 r, g, b;
	};
	struct ModelIndexPushConstant
	{
		std::vector<VkPushConstantRange> ranges(VkPhysicalDeviceLimits limits)
		{
			return std::vector<VkPushConstantRange>{ range1(limits) };
		}

	private:
		VkPushConstantRange range1(VkPhysicalDeviceLimits limits)
		{
			// both size and offset must be a multiple of 4
			static_assert(sizeof(pushConstantRange1) % 4 == 0);
			if (sizeof(pushConstantRange1) > limits.maxPushConstantsSize)
				std::runtime_error("Size of push constants exceeds maximum size allowed by device");
			VkPushConstantRange range = {};
			range.offset = 0;
			range.size = sizeof(pushConstantRange1);
			range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			return range;

		}
	};

	// Vertex Binding data
	struct Vertex
	{
		glm::vec3 Position;
		glm::vec2 TextureCoords;
		// get binding descriptions
		static auto getBindingDescription()
		{
			// interleaved position and texCoord binding
			return std::vector<VkVertexInputBindingDescription> 
			{
				{
					0,										// binding
					sizeof(Vertex),							// stride
					VK_VERTEX_INPUT_RATE_VERTEX				// input rate
				}
			};
		}

		// get attribute descriptions
		static auto getAttributeDescriptions()
		{
			return std::vector<VkVertexInputAttributeDescription>
			{
				{
					// position attribute (vec3)
					0,										// location
					0,										// binding
					VK_FORMAT_R32G32B32_SFLOAT,				// format
					offsetof(Vertex, Position)				// offset
				},
				{
					// texture coordinate attribute (vec2)
					1,										// location
					0,										// binding
					VK_FORMAT_R32G32_SFLOAT,				// format
					offsetof(Vertex, TextureCoords)			// offset
				}
			};
		}
	};

	
}