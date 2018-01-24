#pragma once

#include <algorithm>
#include <iostream>
#include <map>
#include <optional>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "VulkanFunctions.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan_ext.h"
#include "Window.hpp"
#include "fileIO.h"
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#include "vk_mem_alloc.h"
#include "Bitmap.hpp"
#include "Texture2D.hpp"
#include "Sprite.hpp"
#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include "ECS.hpp"
#include "Camera.hpp"

//#include "get_matches.hpp"
namespace vka {
using SpriteCount = size_t;
using ImageIndex = uint32_t;

class VulkanApp;



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
using Index = uint32_t;
constexpr Index IndicesPerQuad = 6U;

struct FragmentPushConstants
{
	glm::uint32 textureID;
	glm::float32 r, g, b, a;
};

struct LoopCallbacks
{
	std::function<SpriteCount()> BeforeRenderCallback;
	std::function<void(VulkanApp*)> RenderCallback;
	std::function<void()> AfterRenderCallback;
};

struct VmaAllocatorDeleter
{
	using pointer = VmaAllocator;
	void operator()(VmaAllocator allocator)
	{
		vmaDestroyAllocator(allocator);
	}
};
using UniqueVmaAllocator = std::unique_ptr<VmaAllocator, VmaAllocatorDeleter>;

struct VmaAllocationDeleter
{
	using pointer = VmaAllocation;
	VmaAllocationDeleter() noexcept = default;
	explicit VmaAllocationDeleter(VmaAllocator allocator) :
		m_Allocator(allocator)
	{
	}
	void operator()(VmaAllocation allocation)
	{
		vmaFreeMemory(m_Allocator, allocation);
	}
	VmaAllocator m_Allocator = nullptr;
};
using UniqueVmaAllocation = std::unique_ptr<VmaAllocation, VmaAllocationDeleter>;

struct TemporaryCommandStructure
{
	vk::UniqueCommandPool pool;
	vk::UniqueCommandBuffer buffer;
};

class SwapChainDependencies
{
public:
	SwapChainDependencies() = default;
	SwapChainDependencies(SwapChainDependencies&&) noexcept = default;
	SwapChainDependencies& operator=(SwapChainDependencies&&) noexcept = default;
	SwapChainDependencies(
		const vk::Device & logicalDevice,
		vk::UniqueSwapchainKHR swapChain,
		const uint32_t bufferCount,
		const vk::RenderPass & renderPass,
		const vk::Format & surfaceFormat,
		const vk::Extent2D & surfaceExtent,
		const vk::GraphicsPipelineCreateInfo& pipelineCreateInfo
	) noexcept;

	vk::UniqueSwapchainKHR m_Swapchain;
	std::vector<vk::Image> m_SwapImages;
	std::vector<vk::UniqueImageView> m_SwapViews;
	std::vector<vk::UniqueFramebuffer> m_Framebuffers;
	vk::UniquePipeline m_Pipeline;
};

class FramebufferSupports
{
public:
	FramebufferSupports() = default;
	FramebufferSupports(FramebufferSupports&&) noexcept = default;
	FramebufferSupports& operator=(FramebufferSupports&&) noexcept = default;

	vk::UniqueCommandPool m_CommandPool;
	vk::UniqueCommandBuffer m_CommandBuffer;
	vk::UniqueBuffer m_MatrixStagingBuffer;
	UniqueVmaAllocation m_MatrixStagingMemory;
	VmaAllocationInfo m_MatrixStagingMemoryInfo;
	vk::UniqueBuffer m_MatrixBuffer;
	UniqueVmaAllocation m_MatrixMemory;
	VmaAllocationInfo m_MatrixMemoryInfo;
	size_t m_BufferCapacity = 0U;
	vk::UniqueDescriptorPool m_VertexLayoutDescriptorPool;
	vk::UniqueDescriptorSet m_VertexDescriptorSet;
	// create this fence in acquireImage() and wait for it before using buffers in BeginRender()
	vk::UniqueFence m_ImagePresentCompleteFence;
	// These semaphores are created at startup
	// this semaphore is checked before rendering begins (at submission of draw command buffer)
	vk::UniqueSemaphore m_MatrixBufferStagingCompleteSemaphore;
	// this semaphore is checked at image presentation time
	vk::UniqueSemaphore m_ImageRenderCompleteSemaphore;
};

struct InitData
{
	char const* WindowClassName;
	char const* WindowTitle;
	Window::WindowStyle windowStyle;
	int width; int height;
	const vk::InstanceCreateInfo instanceCreateInfo;
	const std::vector<const char*> deviceExtensions;
	const ShaderData shaderData;
	std::function<void(VulkanApp*)> imageLoadCallback;
};

class VulkanApp
{
public:
	class Text
	{
	public:
		using FontID = size_t;
		using FontSize = size_t;
		using GlyphID = FT_UInt;
		using CharCode = FT_ULong;

		struct FT_LibraryDeleter
		{
			using pointer = FT_Library;
			void operator()(FT_Library library)
			{
				FT_Done_FreeType(library);
			}
		};
		using UniqueFTLibrary = std::unique_ptr<FT_Library, FT_LibraryDeleter>;

		struct FT_GlyphDeleter
		{
			using pointer = FT_Glyph;
			void operator()(FT_Glyph glyph) noexcept
			{
				FT_Done_Glyph(glyph);
			}
		};
		using UniqueFTGlyph = std::unique_ptr<FT_Glyph, FT_GlyphDeleter>;

		struct GlyphData
		{
			UniqueFTGlyph glyph;
			TextureIndex textureIndex;
		};

		using GlyphMap = std::map<CharCode, GlyphData>;

		struct SizeData
		{
			GlyphMap glyphMap;
			FT_F26Dot6 spaceAdvance;
		};

		struct FontData
		{
			FT_Face face;
			std::string path;
			std::map<FontSize, SizeData> glyphsBySize;
		};

		struct TextGroup
		{
			std::vector<Entity> characters;
		};

		struct InitInfo
		{
			Text::FontID fontID;
			Text::FontSize fontSize;
			std::string text;
			glm::vec4 textColor;
			int baseline_x;
			int baseline_y;
			float depth;
		};

		UniqueFTLibrary m_FreeTypeLibrary;
		std::map<FontID, FontData> m_FontMaps;
		VulkanApp* m_VulkanApp = nullptr;

		Text() = default;
		Text(VulkanApp* app);
		Text::SizeData & getSizeData(Text::FontID fontID, Text::FontSize size);
		Bitmap getFullBitmap(FT_Glyph glyph);
		std::optional<FT_Glyph> getGlyph(Text::FontID fontID, Text::FontSize size, Text::CharCode charcode);
		auto getAdvance(Text::FontID fontID, Text::FontSize size, Text::CharCode left);
		auto getAdvance(Text::FontID fontID, Text::FontSize size, Text::CharCode left, Text::CharCode right);
		void LoadFont(Text::FontID fontID, Text::FontSize fontSize, uint32_t DPI, const char * fontPath);
		std::vector<Sprite> createTextGroup(const Text::InitInfo & initInfo);
	};

	VulkanApp() = default;
	void CreateVertexBuffer();
	void CreateIndexBuffer();
	VulkanApp(InitData initData);

	auto acquireImage();

	bool LoadVulkanDLL();

	bool LoadVulkanEntryPoint();

	bool LoadVulkanGlobalFunctions();

	bool LoadVulkanInstanceFunctions();

	bool LoadVulkanDeviceFunctions();

	auto CreateBuffer(vk::DeviceSize bufferSize, vk::BufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags memoryCreateFlags);

	TemporaryCommandStructure CreateTemporaryCommandBuffer();

	void CopyToBuffer(vk::Buffer source, 
		vk::Buffer destination, 
		vk::DeviceSize size, 
		vk::DeviceSize sourceOffset, 
		vk::DeviceSize destinationOffset, 
		std::vector<vk::Semaphore> waitSemaphores, 
		std::vector<vk::Semaphore> signalSemaphores);

	bool BeginRender(SpriteCount spriteCount);
	void EndRender();
	void Run(LoopCallbacks);
	void RenderSprite(const Sprite & sprite);
	uint32_t createTexture(const Bitmap& image);
	void resizeWindow(ClientSize size);
	vk::UniqueSwapchainKHR createSwapChain();
	vk::UniqueShaderModule createShaderModule(const vk::Device& device, const char* path);
	vk::UniqueDescriptorPool createVertexDescriptorPool(const vk::Device & logicalDevice, uint32_t bufferCount);
	vk::UniqueDescriptorPool createFragmentDescriptorPool(const vk::Device & logicalDevice, uint32_t textureCount);
	vk::UniqueDescriptorSetLayout createVertexSetLayout(const vk::Device& logicalDevice);
	vk::UniqueDescriptorSetLayout createFragmentSetLayout(
		const vk::Device & logicalDevice,
		const uint32_t textureCount,
		const vk::Sampler sampler);
	void LoadFont(VulkanApp::Text::FontID fontID, VulkanApp::Text::FontSize fontSize, uint32_t DPI, const char * fontPath);


	Window m_Window;
	Camera2D m_Camera;
	Text m_Text;
	HMODULE m_VulkanLibrary;
	vk::UniqueInstance m_Instance;
	vk::PhysicalDevice m_PhysicalDevice;
	vk::DeviceSize m_UniformBufferOffsetAlignment;
	uint32_t m_GraphicsQueueFamilyID;
	vk::UniqueDevice m_LogicalDevice;
	vk::Queue m_GraphicsQueue;
	vk::UniqueDebugReportCallbackEXT m_DebugBreakpointCallbackData;
	UniqueVmaAllocator m_Allocator;
	std::vector<vk::UniqueImage> m_Images;
	std::vector<UniqueVmaAllocation> m_ImageAllocations;
	std::vector<vk::UniqueImageView> m_ImageViews;
	std::vector<Texture2D> m_Textures;
	std::vector<VmaAllocationInfo> m_ImageAllocationInfos;
	std::vector<Vertex> m_Vertices;
	vk::UniqueBuffer m_VertexBuffer;
	UniqueVmaAllocation m_VertexMemory;
	VmaAllocationInfo m_VertexMemoryInfo;
	std::vector<uint32_t> m_Indices;
	vk::UniqueBuffer m_IndexBuffer;
	UniqueVmaAllocation m_IndexMemory;
	VmaAllocationInfo m_IndexMemoryInfo;
	vk::UniqueSurfaceKHR m_Surface;
	vk::Extent2D m_SurfaceExtent;
	vk::Format m_SurfaceFormat;
	vk::ColorSpaceKHR m_SurfaceColorSpace;
	vk::UniqueRenderPass m_RenderPass;
	vk::UniqueSampler m_Sampler;
	vk::UniqueShaderModule m_VertexShader;
	vk::UniqueShaderModule m_FragmentShader;
	vk::UniqueDescriptorSetLayout m_VertexDescriptorSetLayout;
	vk::UniqueDescriptorSetLayout m_FragmentDescriptorSetLayout;
	std::vector<vk::DescriptorSetLayout> m_DescriptorSetLayouts;
	std::vector<vk::PushConstantRange> m_PushConstantRanges;
	vk::UniqueDescriptorPool m_FragmentLayoutDescriptorPool;
	vk::UniqueDescriptorSet m_FragmentDescriptorSet;
	vk::PresentModeKHR m_PresentMode;
	static const uint32_t BufferCount = 3U;
	uint32_t m_TextureCount = 0U;
	vk::UniquePipelineLayout m_PipelineLayout;
	std::vector<vk::SpecializationMapEntry> m_VertexSpecializations;
	std::vector<vk::SpecializationMapEntry> m_FragmentSpecializations;
	vk::SpecializationInfo m_VertexSpecializationInfo;
	vk::SpecializationInfo m_FragmentSpecializationInfo;
	vk::PipelineShaderStageCreateInfo m_VertexShaderStageInfo;
	vk::PipelineShaderStageCreateInfo m_FragmentShaderStageInfo;
	std::vector<vk::PipelineShaderStageCreateInfo> m_PipelineShaderStageInfo;
	std::vector<vk::VertexInputBindingDescription> m_VertexBindings;
	std::vector<vk::VertexInputAttributeDescription> m_VertexAttributes;
	vk::PipelineVertexInputStateCreateInfo m_PipelineVertexInputInfo;
	vk::PipelineInputAssemblyStateCreateInfo m_PipelineInputAssemblyInfo;
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
	void createVulkanImageForBitmap(const Bitmap & bitmap, VmaAllocationInfo & imageAllocInfo, vk::UniqueImage & image2D, UniqueVmaAllocation & imageAlloc);
	struct UpdateData
	{
		glm::mat4 vp;
		vk::DeviceSize copyOffset;
		ImageIndex nextImage;
		SpriteCount spriteIndex;
		void* mapped;
	} m_UpdateData;
};

}