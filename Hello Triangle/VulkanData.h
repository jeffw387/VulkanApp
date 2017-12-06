#pragma once
#include <vulkan/vulkan.h>
#include <array>
#include <glm.hpp>

using ImageIndex = uint32_t;

namespace VulkanData
{
	const auto TextureCount = 2U;

	// TODO: measure whether it's faster to multiply M with VP on CPU or GPU
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
			return std::vector<VkDescriptorPoolSize> { poolSize0, poolSize1 }; 
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
	};

	struct ModelIndexPushConstant
	{
		std::vector<VkPushConstantRange> ranges()
		{
			return std::vector<VkPushConstantRange>{ range1() };
		}

	private:
		VkPushConstantRange range1()
		{
			return VkPushConstantRange{
			VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(ImageIndex),
			};
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