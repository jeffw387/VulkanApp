#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm.hpp"
#include "vulkan/vulkan.hpp"
#include "Window.hxx"
#include <algorithm>
#include "fileIO.h"
#include "vk_mem_alloc.h"
#include "Bitmap.hpp"
#include "Texture2D.hpp"

//#include "get_matches.hpp"

struct ShaderData
{
	const char* vertexShaderPath;
	const char* fragmentShaderPath;
};

struct Vertex
{
	glm::vec2 Position;
	glm::vec2 UV;
	static std::vector<vk::VertexInputBindingDescription> vertexBindingDescriptions();
	static std::vector<vk::VertexInputAttributeDescription> vertexAttributeDescriptions();
};

struct FragmentPushConstants
{
	glm::uint32 textureID;
	glm::float32 r,g,b,a;
};

struct VMAWrapper
{
	VMAWrapper() {}
	VMAWrapper(VmaAllocator alloc) : allocator(alloc) {}
	~VMAWrapper();
	operator VmaAllocator();

	VMAWrapper operator=(VmaAllocator tmp)
	{
		std::swap(allocator, tmp);
		return *this;
	}
	VmaAllocator allocator = nullptr;
};

class SwapChainDependencies
{
public:
	SwapChainDependencies() = default;
	SwapChainDependencies(SwapChainDependencies&& other);
	SwapChainDependencies operator=(SwapChainDependencies&& other)
	{
		*this = std::move(other);
	}
	SwapChainDependencies(
		const vk::Device & logicalDevice, 
		vk::UniqueSwapchainKHR swapChain,
		const uint32_t bufferCount,
		const vk::RenderPass & renderPass, 
		const vk::Format & surfaceFormat, 
		const vk::Extent2D & surfaceExtent, 
		const vk::GraphicsPipelineCreateInfo& pipelineCreateInfo
		);

	vk::UniqueSwapchainKHR m_Swapchain;
	std::vector<vk::Image> m_SwapImages;
	std::vector<vk::UniqueImageView> m_SwapViews;
	std::vector<vk::UniqueFramebuffer> m_Framebuffers;
	vk::UniquePipeline m_Pipeline;
};

class FramebufferSupports
{
public:
	vk::UniqueCommandPool m_CommandPool;
	vk::UniqueCommandBuffer m_CommandBuffer;
	vk::UniqueBuffer m_MatrixStagingBuffer;
	vk::UniqueBuffer m_MatrixBuffer;
	size_t m_BufferCapacity = 0U;
	vk::UniqueDescriptorPool m_VertexLayoutDescriptorPool;
	vk::UniqueDescriptorSet m_VertexDescriptorSet;
	vk::UniqueSemaphore m_MatrixBufferStagingCompleteSemaphore;
	vk::UniqueFence m_ImagePresentCompleteFence;
	vk::UniqueSemaphore m_ImagePresentCompleteSemaphore;
	vk::UniqueSemaphore m_ImageRenderCompleteSemaphore;
};

class VulkanApp;
class Device
{
public:
	Device(const vk::Instance& instance, 
		const Window & window, 
		const std::vector<const char*> deviceExtensions, 
		const ShaderData & shaderData,
		std::function<void(VulkanApp*)> imageLoadCallback,
		VulkanApp* app);

	vk::UniqueSwapchainKHR createSwapChain();

	vk::UniqueShaderModule createShaderModule(const vk::Device& device, const char* path);
	vk::UniqueDescriptorPool createVertexDescriptorPool(const vk::Device & logicalDevice, uint32_t bufferCount);
	vk::UniqueDescriptorPool createFragmentDescriptorPool(const vk::Device & logicalDevice, uint32_t textureCount);
	vk::UniqueDescriptorSetLayout createVertexSetLayout(const vk::Device& logicalDevice);
	vk::UniqueDescriptorSetLayout createFragmentSetLayout(
		const vk::Device & logicalDevice, 
		const uint32_t textureCount, 
		const vk::Sampler sampler);

	vk::PhysicalDevice m_PhysicalDevice;
	vk::UniqueDevice m_LogicalDevice;
	VMAWrapper m_Allocator;
	vk::Queue m_GraphicsQueue;
	uint32_t m_GraphicsQueueFamilyID;
	vk::UniqueSurfaceKHR m_Surface;
	vk::Extent2D m_SurfaceExtent;
	vk::Format m_SurfaceFormat;
	vk::ColorSpaceKHR m_SurfaceColorSpace;
	std::vector<vk::SubpassDependency> m_SubpassDependencies;
	vk::UniqueRenderPass m_RenderPass;
	vk::UniqueSampler m_Sampler;
	vk::UniqueDescriptorSetLayout m_VertexDescriptorSetLayout;
	vk::UniqueDescriptorSetLayout m_FragmentDescriptorSetLayout;
	std::vector<vk::DescriptorSetLayout> m_DescriptorSetLayouts;
	std::vector<vk::PushConstantRange> m_PushConstantRanges;
	vk::UniqueShaderModule m_VertexShader;
	vk::UniqueShaderModule m_FragmentShader;
	vk::UniqueDescriptorPool m_FragmentLayoutDescriptorPool;
	vk::UniqueDescriptorSet m_FragmentDescriptorSet;
	vk::PresentModeKHR m_PresentMode;
	const uint32_t m_BufferCount = 3U;
	uint32_t m_TextureCount = 0U;
	vk::UniquePipelineLayout m_PipelineLayout;
	std::vector<vk::SpecializationMapEntry> m_VertexSpecializations;
	std::vector<vk::SpecializationMapEntry> m_FragmentSpecializations;
	vk::SpecializationInfo m_VertexSpecializationInfo;
	vk::SpecializationInfo m_FragmentSpecializationInfo;
	vk::PipelineShaderStageCreateInfo m_VertexShaderStageInfo;
	vk::PipelineShaderStageCreateInfo m_FragmentShaderStageInfo;
	std::vector<vk::PipelineShaderStageCreateInfo> m_PipelineShaderStageInfo;
	std::vector<vk::VertexInputBindingDescription> m_VertexInputBindings;
	std::vector<vk::VertexInputAttributeDescription> m_VertexAttributes;
	vk::PipelineVertexInputStateCreateInfo m_PipelineVertexInputInfo;
	vk::PipelineInputAssemblyStateCreateInfo m_PipelineInputAsemblyInfo;
	vk::PipelineTessellationStateCreateInfo m_PipelineTesselationStateInfo;
	vk::PipelineViewportStateCreateInfo m_PipelineViewportInfo;
	vk::PipelineRasterizationStateCreateInfo m_PipelineRasterizationInfo;
	vk::PipelineMultisampleStateCreateInfo m_PipelineMultisampleInfo;
	vk::PipelineDepthStencilStateCreateInfo m_PipelineDepthStencilInfo;
	vk::PipelineColorBlendAttachmentState m_PipelineColorBlendAttachmentState;
	vk::PipelineColorBlendStateCreateInfo m_PipelineBlendStateInfo;
	std::vector<vk::DynamicState> m_DynamicStates;
	vk::PipelineDynamicStateCreateInfo m_PipelineDynamicStateInfo;
	vk::GraphicsPipelineCreateInfo m_PipelineCreateInfo;
	SwapChainDependencies m_SwapchainDependencies;
	std::vector<FramebufferSupports> m_FramebufferSupports;
};

class VulkanApp
{
public:
	VulkanApp(
		LPTSTR WindowClassName,
		LPTSTR WindowTitle,
		Window::WindowStyle windowStyle,
		int width,
		int height,
		const vk::InstanceCreateInfo& instanceCreateInfo,
		const std::vector<const char*>& deviceExtensions,
		const ShaderData& shaderData,
		std::function<void(VulkanApp*)> imageLoadCallback
	);

	uint32_t createTexture(const Bitmap& image);
	
	Window m_Window;
	vk::UniqueInstance m_Instance;
	Device m_Device;
	std::vector<vk::Image> m_Images;
	std::vector<Texture2D> m_Textures;
	std::vector<VmaAllocation> m_ImageAllocations;
	std::vector<VmaAllocationInfo> m_ImageAllocationInfos;
	std::vector<Vertex> m_Vertices;
	std::vector<uint32_t> m_Indices;
};