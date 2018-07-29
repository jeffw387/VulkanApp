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

template<typename E>
constexpr auto get(E e) -> typename std::underlying_type<E>::type
{
	return static_cast<typename std::underlying_type<E>::type>(e);
}

enum class Queues : uint32_t {
	Graphics,
	Present,
	COUNT
};

enum class SpecConsts : uint32_t {
	MaterialCount,
	TextureCount,
	LightCount,
	COUNT
};

enum class SetLayouts : uint32_t {
	Static,
	Frame,
	Instance,
	COUNT
};

enum class Shaders : uint32_t {
	Vertex2D,
	Fragment2D,
	Vertex3D,
	Fragment3D,
	COUNT
};

enum class Samplers : uint32_t {
	Texture2D,
	COUNT
};

enum class PushRanges : uint32_t {
	Primary,
	COUNT
};

enum class VertexBuffers : uint32_t {
	Vertex2D,
	Position3D,
	Normal3D,
	Index3D,
	COUNT
};

enum class UniformBuffers : uint32_t {
	Camera,
	Lights,
	InstanceStaging,
	Instance,
	COUNT
};

enum class PipelineLayouts : uint32_t {
	Primary,
	COUNT
};

enum class ClearValues : uint32_t {
	Color,
	Depth,
	COUNT
};

enum class Pipelines : uint32_t {
	P3D,
	P2D,
	COUNT
};

struct PushConstantData {
	glm::uint32 materialIndex;
	glm::uint32 textureIndex;
};

struct SpecializationData {
	using MaterialCount = glm::uint32;
	using TextureCount = glm::uint32;
	using LightCount = glm::uint32;
	MaterialCount materialCount;
	TextureCount textureCount;
	LightCount lightCount;
};

struct PipelineState
{
	VkViewport viewport;
	VkRect2D scissor;
	std::vector<VkDynamicState> dynamicStates;
	VkPipelineDynamicStateCreateInfo dynamicState;
	VkPipelineMultisampleStateCreateInfo multisampleState;
	VkPipelineViewportStateCreateInfo viewportState;
	VkSpecializationInfo specializationInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState;
	VkPipelineColorBlendStateCreateInfo colorBlendState;
	VkPipelineDepthStencilStateCreateInfo depthStencilState;
	VkPipelineRasterizationStateCreateInfo rasterizationState;
	std::vector<VkVertexInputAttributeDescription> vertexAttributes;
	std::vector<VkVertexInputBindingDescription> vertexBindings;
	VkPipelineVertexInputStateCreateInfo vertexInputState;
};


struct VS
{
	vka::LibraryHandle VulkanLibrary;

	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	bool unifiedMemory = false;
	VkDeviceSize uniformBufferOffsetAlignment;

	std::vector<VkQueueFamilyProperties> familyProperties;
	std::array<float, get(Queues::COUNT)> queuePriorities;
	std::array<uint32_t, get(Queues::COUNT)> queueFamilyIndices;
	std::array<VkQueue, get(Queues::COUNT)> queues;

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
	SpecializationData specData;

	std::vector<VkDescriptorSetLayoutBinding> staticLayoutBindings;
	std::vector<VkDescriptorSetLayoutBinding> frameLayoutBindings;
	std::vector<VkDescriptorSetLayoutBinding> instanceLayoutBindings;
	std::array<VkSampler, get(Samplers::COUNT)> samplers;
	std::array<VkDescriptorSetLayout, get(SetLayouts::COUNT)> layouts;
	std::array<VkPushConstantRange, get(PushRanges::COUNT)> pushRanges;
	std::array<VkShaderModule, get(Shaders::COUNT)> shaderModules;
	std::array<vka::Buffer, get(VertexBuffers::COUNT)> vertexBuffers;
	std::array<VkPipelineLayout, get(PipelineLayouts::COUNT)> pipelineLayouts;
	std::array<VkSpecializationMapEntry, get(SpecConsts::COUNT)> specializationMapEntries;
	std::array<PipelineState, get(Pipelines::COUNT)> pipelineStates;
	std::array<VkGraphicsPipelineCreateInfo, get(Pipelines::COUNT)> pipelineCreateInfos;
	std::array<VkPipeline, get(Pipelines::COUNT)> pipelines;

	uint32_t nextImage;

	std::array<VkClearValue, get(ClearValues::COUNT)> clearValues =
	{
		VkClearValue{ 0.f, 0.f, 0.f, 0.f },
		VkClearValue{ 1.f, 0U }
	};

	VkDescriptorPool staticLayoutPool;
	vka::Buffer materials;
	VkDeviceSize instanceBuffersOffsetAlignment;
	struct FrameData {
		std::array<vka::Buffer, get(UniformBuffers::COUNT)> uniformBuffers;
		size_t instanceBufferCapacity = 0;
		VkFence stagingFence;
		VkDescriptorPool dynamicDescriptorPool;
		VkImage swapImage;
		VkImageView swapView;
		VkFramebuffer framebuffer;
		VkCommandPool renderCommandPool;
		VkCommandBuffer renderCommandBuffer;
		VkSemaphore imageRenderedSemaphore;
	} frameData[BufferCount];
	std::array<VkFence, BufferCount> imageReadyFences;
	Pool<VkFence> imageReadyFencePool;
};