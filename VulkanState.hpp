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
constexpr uint32_t ImageCount = 1;
constexpr VkExtent2D DefaultWindowSize = { 900, 900 };
constexpr uint32_t LightCount = 3;

template<typename E>
constexpr auto get(E e) -> typename std::underlying_type<E>::type
{
	return static_cast<typename std::underlying_type<E>::type>(e);
}

enum class Materials : uint32_t {
	White,
	Red,
	Green,
	Blue,
	COUNT
};

enum class Models : uint32_t {
	Cylinder,
	Cube,
	Triangle,
	IcosphereSub2,
	Pentagon,
	COUNT
};

enum class Queues : uint32_t {
	Graphics,
	Present,
	COUNT
};

enum class SpecConsts : uint32_t {
	MaterialCount,
	ImageCount,
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

enum class VertexBuffers3D : uint32_t {
	Position3D,
	Normal3D,
	Index3D,
	COUNT
};

enum class VertexBuffers2D : uint32_t {
	Vertex2D,
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

struct InstanceUniform
{
	glm::mat4 model;
	glm::mat4 MVP;
};

struct CameraUniform
{
	glm::mat4 view;
	glm::mat4 projection;
};

struct MaterialUniform
{
	glm::vec4 color;
};

struct LightUniform {
	glm::vec4 position;
	glm::vec4 color;
};

struct PushConstantData {
	glm::uint32 materialIndex;
	glm::uint32 textureIndex;
};

struct SpecializationData {
	using MaterialCount = glm::uint32;
	using ImageCount = glm::uint32;
	using LightCount = glm::uint32;
	MaterialCount materialCount;
	ImageCount imageCount;
	LightCount lightCount;
};

struct PipelineState
{
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

	std::array<vka::Image2D, ImageCount> images;
	std::array<LightUniform, LightCount> lightUniforms;

	std::array<MaterialUniform, get(Materials::COUNT)> materialUniforms =
	{
		MaterialUniform{ glm::vec4(1.f) },
		MaterialUniform{ glm::vec4(1.f, 0.f, 0.f, 1.f) },
		MaterialUniform{ glm::vec4(0.f, 1.f, 0.f, 1.f) },
		MaterialUniform{ glm::vec4(0.f, 0.f, 1.f, 1.f) }
	};

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

	VkViewport viewport;
	VkRect2D scissor;
	VkRenderPass renderPass;
	VkSwapchainKHR swapchain;
	vka::Image2D depthImage;
	SpecializationData specData;

	std::vector<VkDescriptorSetLayoutBinding> staticLayoutBindings;
	std::vector<VkDescriptorSetLayoutBinding> frameLayoutBindings;
	std::vector<VkDescriptorSetLayoutBinding> instanceLayoutBindings;
	std::array<VkSampler, get(Samplers::COUNT)> samplers;
	std::array<VkDescriptorSetLayout, get(SetLayouts::COUNT)> setLayouts;
	std::array<VkPushConstantRange, get(PushRanges::COUNT)> pushRanges;
	std::array<VkShaderModule, get(Shaders::COUNT)> shaderModules;
	std::array<vka::Buffer, get(VertexBuffers2D::COUNT)> vertexBuffers2D;
	std::array<vka::Buffer, get(VertexBuffers3D::COUNT)> vertexBuffers3D;
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
	VkDescriptorSet staticSet;
	VkDeviceSize materialsBufferOffsetAlignment;
	vka::Buffer materialsUniformBuffer;
	VkDeviceSize instanceBuffersOffsetAlignment;
	VkDeviceSize lightsBuffersOffsetAlignment;

	std::array<vka::Buffer, BufferCount> cameraUniformBuffers;
	std::array<vka::Buffer, BufferCount> lightsUniformBuffers;
	std::array<vka::Buffer, BufferCount> instanceStagingBuffers;
	std::array<vka::Buffer, BufferCount> instanceUniformBuffers;
	std::array<size_t, BufferCount> instanceBufferCapacities;
	std::array<VkFence, BufferCount> stagingFences;
	std::array<VkDescriptorPool, BufferCount> dynamicLayoutPools;
	std::array<VkDescriptorSet, BufferCount> frameDescriptorSets;
	std::array<VkDescriptorSet, BufferCount> instanceDescriptorSets;
	std::array<VkImage, BufferCount> swapImages;
	std::array<VkImageView, BufferCount> swapViews;
	std::array<VkFramebuffer, BufferCount> framebuffers;
	std::array<VkCommandPool, BufferCount> renderCommandPools;
	std::array<VkCommandBuffer, BufferCount> renderCommandBuffers;
	std::array<VkSemaphore, BufferCount> imageRenderedSemaphores;
	std::array<VkFence, BufferCount> imageReadyFences;
	Pool<VkFence> imageReadyFencePool;
};