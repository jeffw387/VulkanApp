#pragma once

#include "vulkan/vulkan.h"
#include "vka/VulkanFunctionLoader.hpp"
#include "vka/UniqueVulkan.hpp"
#include "vka/Buffer.hpp"
#include "vka/Image2D.hpp"
#include "Pool.hpp"
#include "vka/Allocator.hpp"

#include <array>
#include <vector>

constexpr uint32_t BufferCount = 3;
namespace vka
{
	struct VS
	{
		LibraryHandle VulkanLibrary;

		VkInstance instance;
		VkPhysicalDevice physicalDevice;
		bool unifiedMemory = false;
		VkDeviceSize uniformBufferOffsetAlignment;

		struct Queues {
			std::vector<VkQueueFamilyProperties> familyProperties;
			struct Graphics {
				float priority = 0.f;
				uint32_t familyIndex;
				VkQueue queue;
			} graphics;
			struct Present {
				float priority = 0.f;
				uint32_t familyIndex;
				VkQueue queue;
			} present;
		} queue;
		VkDevice device;
		vka::Allocator allocator;

		struct Utility {
			VkCommandPool pool;
			VkCommandBuffer buffer;
			VkFence fence;
		} utility;

		VkSurfaceKHR surface;
		struct SurfaceData {
			VkExtent2D extent;
			std::vector<VkSurfaceFormatKHR> supportedFormats;
			VkSurfaceFormatKHR swapFormat;
			VkFormat depthFormat;
			std::vector<VkPresentModeKHR> supportedPresentModes;
			VkPresentModeKHR presentMode;
		} surfaceData;

		VkRenderPass renderPass;
		VkSwapchainKHR swapchain;
		vka::Image2D depthImage;

		VkSampler sampler;

		std::vector<VkDescriptorSetLayoutBinding> staticLayoutBindings;
		std::vector<VkDescriptorSetLayoutBinding> frameLayoutBindings;
		std::vector<VkDescriptorSetLayoutBinding> drawLayoutBindings;
		std::array<VkDescriptorSetLayout, 3> layouts;
		struct PushConstantData {
			glm::uint32 materialIndex;
			glm::uint32 textureIndex;
		} pushConstantData;
		VkPushConstantRange pushRange;

		struct Shaders {
			struct P2D {
				VkShaderModule vertex;
				VkShaderModule fragment;
			} p2D;
			struct P3D {
				VkShaderModule vertex;
				VkShaderModule fragment;
			} p3D;
		} shaders;

		struct VertexBuffers {
			struct P2D {
				vka::Buffer vertexBuffer;
			} p2D;
			struct P3D {
				vka::Buffer positionBuffer;
				vka::Buffer normalBuffer;
				vka::Buffer indexBuffer;
			} p3D;
		} vertexBuffers;

		struct SpecializationData {
			using MaterialCount = glm::uint32;
			using TextureCount = glm::uint32;
			using LightCount = glm::uint32;
			MaterialCount materialCount;
			TextureCount textureCount;
			LightCount lightCount;
		} specializationData;

		struct Pipeline {
			VkPipelineLayout layout;
			VkViewport viewport;
			VkRect2D scissor;
			struct Common {
				struct DynamicStates {
					VkDynamicState viewport = VK_DYNAMIC_STATE_VIEWPORT;
					VkDynamicState scissor = VK_DYNAMIC_STATE_SCISSOR;
				} dynamicStates;
				VkPipelineDynamicStateCreateInfo dynamicState;
				VkPipelineMultisampleStateCreateInfo multisampleState;
				VkPipelineViewportStateCreateInfo viewportState;
				VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
				struct SpecializationMapEntries {
					VkSpecializationMapEntry materialCount;
					VkSpecializationMapEntry textureCount;
					VkSpecializationMapEntry lightCount;
				} specializationMapEntries;
				VkSpecializationInfo specializationInfo;
			} common;

			struct P2DState {
				std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
				VkPipelineColorBlendAttachmentState colorBlendAttachmentState;
				VkPipelineColorBlendStateCreateInfo colorBlendState;
				VkPipelineRasterizationStateCreateInfo rasterizationState;
				std::vector<VkVertexInputAttributeDescription> vertexAttributes;
				std::vector<VkVertexInputBindingDescription> vertexBindings;
				VkPipelineVertexInputStateCreateInfo vertexInputState;

			} p2DState;
			struct P3DState {
				std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
				VkPipelineColorBlendAttachmentState colorBlendAttachmentState;
				VkPipelineColorBlendStateCreateInfo colorBlendState;
				VkPipelineDepthStencilStateCreateInfo depthStencilState;
				VkPipelineRasterizationStateCreateInfo rasterizationState;
				std::vector<VkVertexInputAttributeDescription> vertexAttributes;
				std::vector<VkVertexInputBindingDescription> vertexBindings;
				VkPipelineVertexInputStateCreateInfo vertexInputState;
			} p3DState;

			VkGraphicsPipelineCreateInfo p2DCreateInfo;
			VkGraphicsPipelineCreateInfo p3DCreateInfo;
			VkPipeline p2D;
			VkPipeline p3D;
		} pipeline;

		uint32_t nextImage;

		VkDescriptorPool staticLayoutPool;
		vka::Buffer materials;
		VkDeviceSize instanceBuffersOffsetAlignment;
		struct FrameData {
			struct Uniform {
				vka::Buffer camera;
				vka::Buffer lights;
				vka::Buffer instance;
				size_t instanceBufferCapacity = 0;
			} uniform;
			VkDescriptorPool dynamicDescriptorPool;
			VkImage swapImage;
			VkImageView swapView;
			VkFramebuffer framebuffer;
			VkCommandPool renderCommandPool;
			VkCommandBuffer renderCommandBuffer;
			VkSemaphore imageRenderedSemaphore;
		} frameData[BufferCount];
		std::array<VkFence, BufferCount> imagePresentedFences;
		Pool<VkFence> imagePresentedFencePool;
	};
}