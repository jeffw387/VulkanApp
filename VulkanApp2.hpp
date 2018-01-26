#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "VulkanFunctions.hpp"
#include "vulkan/vulkan.hpp"
#include "Window.hpp"
#include "fileIO.h"
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#include "vk_mem_alloc.h"
#include "Texture2D.hpp"
#include "Bitmap.hpp"
#include "Sprite.hpp"
#include "Camera.hpp"
#include "ECS.hpp"
#include "gsl/gsl"
#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include <iostream>
#include <map>

namespace vka
{
	using SpriteCount = size_t;
	using ImageIndex = uint32_t;
	struct VulkanApp;
	struct ShaderData
	{
		const char* vertexShaderPath;
		const char* fragmentShaderPath;
	};

	struct Vertex
	{
		glm::vec2 Position;
		glm::vec2 UV;
		static std::vector<vk::VertexInputBindingDescription> vertexBindingDescriptions()
		{
			auto bindingDescriptions = std::vector<vk::VertexInputBindingDescription>{ vk::VertexInputBindingDescription(0U, sizeof(Vertex), vk::VertexInputRate::eVertex) };
			return bindingDescriptions;
		}

		static std::vector<vk::VertexInputAttributeDescription> vertexAttributeDescriptions()
		{
			// Binding both attributes to one buffer, interleaving Position and UV
			auto attrib0 = vk::VertexInputAttributeDescription(0U, 0U, vk::Format::eR32G32Sfloat, offsetof(Vertex, Position));
			auto attrib1 = vk::VertexInputAttributeDescription(1U, 0U, vk::Format::eR32G32Sfloat, offsetof(Vertex, UV));
			return std::vector<vk::VertexInputAttributeDescription>{ attrib0, attrib1 };
		}
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

	static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugBreakCallback(
		VkDebugReportFlagsEXT flags, 
		VkDebugReportObjectTypeEXT objType, 
		uint64_t obj,
		size_t location,
		int32_t messageCode,
		const char* pLayerPrefix,
		const char* pMessage, 
		void* userData)
	{
		if (flags & VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_DEBUG_BIT_EXT)
			std::cerr << "(Debug Callback)";
		if (flags & VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_ERROR_BIT_EXT)
			std::cerr << "(Error Callback)";
		if (flags & VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
			return false;
			//std::cerr << "(Information Callback)";
		if (flags & VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
			std::cerr << "(Performance Warning Callback)";
		if (flags & VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_WARNING_BIT_EXT)
			std::cerr << "(Warning Callback)";
		std::cerr << pLayerPrefix << pMessage <<"\n";

		return false;
	}

struct VulkanApp
{
	struct Supports
	{
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

	struct UpdateData
	{
		uint32_t nextImage;
		glm::mat4 vp;
		vk::DeviceSize copyOffset;
		void* mapped;
		uint32_t spriteIndex;
	};
	
	static constexpr size_t BufferCount = 3U;
	UpdateData m_UpdateData;
	Window m_Window;
	Camera2D m_Camera;
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
	vk::FormatProperties m_SurfaceFormatProperties;
	vk::SurfaceCapabilitiesKHR m_SurfaceCapabilities;
	std::vector<vk::PresentModeKHR> m_SurfacePresentModes;
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
	std::array<Supports, 3> m_Supports;
	vk::UniqueSwapchainKHR m_Swapchain;
	std::vector<vk::Image> m_SwapImages;
	std::array<vk::UniqueImageView, BufferCount> m_SwapViews;
	std::array<vk::UniqueFramebuffer, BufferCount> m_Framebuffers;
	vk::UniquePipeline m_Pipeline;

	bool LoadVulkanDLL()
	{
		m_VulkanLibrary = LoadLibrary(L"vulkan-1.dll");

		if (m_VulkanLibrary == nullptr)
			return false;
		return true;
	}

	bool LoadVulkanEntryPoint()
	{
#define VK_EXPORTED_FUNCTION( fun )                                                   \
    if( !(fun = (PFN_##fun)GetProcAddress( m_VulkanLibrary, #fun )) ) {               \
      std::cout << "Could not load exported function: " << #fun << "!" << std::endl;  \
    }

#include "VulkanFunctions.inl"

		return true;
	}

	bool LoadVulkanGlobalFunctions()
	{
#define VK_GLOBAL_LEVEL_FUNCTION( fun )                                                   \
    if( !(fun = (PFN_##fun)vkGetInstanceProcAddr( nullptr, #fun )) ) {                    \
      std::cout << "Could not load global level function: " << #fun << "!" << std::endl;  \
    }

#include "VulkanFunctions.inl"

		return true;
	}

	bool LoadVulkanInstanceFunctions()
	{
#define VK_INSTANCE_LEVEL_FUNCTION( fun )                                                   \
    if( !(fun = (PFN_##fun)vkGetInstanceProcAddr( m_Instance.get(), #fun )) ) {             \
      std::cout << "Could not load instance level function: " << #fun << "!" << std::endl;  \
    }

#include "VulkanFunctions.inl"

		return true;
	}

	bool LoadVulkanDeviceFunctions()
	{
#define VK_DEVICE_LEVEL_FUNCTION( fun )                                                   \
    if( !(fun = (PFN_##fun)vkGetDeviceProcAddr( m_LogicalDevice.get(), #fun )) ) {        \
      std::cout << "Could not load device level function: " << #fun << "!" << std::endl;  \
    }

#include "VulkanFunctions.inl"

		return true;
	}

	struct BufferCreateResult
	{
		vk::UniqueBuffer buffer;
		UniqueVmaAllocation allocation;
		VmaAllocationInfo allocationInfo;
	};
	auto CreateBuffer(
		vk::DeviceSize bufferSize, 
		vk::BufferUsageFlags bufferUsage, 
		VmaMemoryUsage memoryUsage,
		VmaAllocationCreateFlags memoryCreateFlags)
	{
		BufferCreateResult result;
		auto bufferCreateInfo = vk::BufferCreateInfo(
			vk::BufferCreateFlags(),
			bufferSize,
			bufferUsage,
			vk::SharingMode::eExclusive,
			1U,
			&m_GraphicsQueueFamilyID);
		auto allocationCreateInfo = VmaAllocationCreateInfo {};
		allocationCreateInfo.usage = memoryUsage;
		allocationCreateInfo.flags = memoryCreateFlags;
		VkBuffer buffer;
		VmaAllocation allocation;
		vmaCreateBuffer(m_Allocator.get(), 
			&bufferCreateInfo.operator const VkBufferCreateInfo &(),
			&allocationCreateInfo,
			&buffer,
			&allocation,
			&result.allocationInfo);

		result.buffer = vk::UniqueBuffer(buffer, vk::BufferDeleter(m_LogicalDevice.get()));
		VmaAllocationDeleter allocationDeleter = VmaAllocationDeleter(m_Allocator.get());
		result.allocation = UniqueVmaAllocation(allocation, allocationDeleter);

		return result;
	}

	TemporaryCommandStructure CreateTemporaryCommandBuffer()
	{
		TemporaryCommandStructure result;
		// get command pool and buffer
		result.pool = m_LogicalDevice->createCommandPoolUnique(
			vk::CommandPoolCreateInfo(
				vk::CommandPoolCreateFlagBits::eTransient,
				m_GraphicsQueueFamilyID));
		auto buffer = m_LogicalDevice->allocateCommandBuffers(
			vk::CommandBufferAllocateInfo(
				result.pool.get(),
				vk::CommandBufferLevel::ePrimary,
				1U))[0];
		auto bufferDeleter = vk::CommandBufferDeleter(m_LogicalDevice.get(), result.pool.get());
		result.buffer = vk::UniqueCommandBuffer(buffer, bufferDeleter);
		return result;
	}

	void CopyToBuffer(
		vk::Buffer source, 
		vk::Buffer destination, 
		vk::DeviceSize size, 
		vk::DeviceSize sourceOffset, 
		vk::DeviceSize destinationOffset,
		std::vector<vk::Semaphore> waitSemaphores = {},
		std::vector<vk::Semaphore> signalSemaphores = {}
	)
	{
		auto fence = m_LogicalDevice->createFenceUnique(vk::FenceCreateInfo());
		auto command = CreateTemporaryCommandBuffer();
		command.buffer->begin(vk::CommandBufferBeginInfo(
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
		command.buffer->copyBuffer(source, destination, {vk::BufferCopy(sourceOffset, destinationOffset, size)});
		command.buffer->end();
		m_GraphicsQueue.submit( {vk::SubmitInfo(
			gsl::narrow<uint32_t>(waitSemaphores.size()),
			waitSemaphores.data(),
			nullptr,
			1U,
			&command.buffer.get(),
			gsl::narrow<uint32_t>(signalSemaphores.size()),
			signalSemaphores.data()) }, fence.get());

		// wait for buffer to finish execution
		m_LogicalDevice->waitForFences(
			{fence.get()}, 
			static_cast<vk::Bool32>(true), 
			std::numeric_limits<uint64_t>::max());
	}


	auto acquireImage()
	{
		auto renderFinishedFence = m_LogicalDevice->createFence(
			vk::FenceCreateInfo(vk::FenceCreateFlags()));
		auto nextImage = m_LogicalDevice->acquireNextImageKHR(
			m_Swapchain.get(), 
			std::numeric_limits<uint64_t>::max(), 
			vk::Semaphore(),
			renderFinishedFence);
		m_Supports[nextImage.value].m_ImagePresentCompleteFence = vk::UniqueFence(renderFinishedFence, vk::FenceDeleter(m_LogicalDevice.get()));
		return nextImage;
	}

	bool BeginRender(SpriteCount spriteCount)
	{
		auto nextImage = acquireImage();
		if (nextImage.result != vk::Result::eSuccess)
		{
			return false;
		}
		m_UpdateData.nextImage = nextImage.value;

		auto& supports = m_Supports[nextImage.value];

		// Sync host to device for the next image
		m_LogicalDevice->waitForFences(
			{supports.m_ImagePresentCompleteFence.get()}, 
			static_cast<vk::Bool32>(true), 
			std::numeric_limits<uint64_t>::max());

		const size_t minimumCount = 1;
		size_t instanceCount = std::max(minimumCount, spriteCount);
		// (re)create matrix buffer if it is smaller than required
		if (instanceCount > supports.m_BufferCapacity)
		{
			supports.m_BufferCapacity = instanceCount * 2;

			auto newBufferSize = supports.m_BufferCapacity * m_UniformBufferOffsetAlignment;

			// create staging buffer
			auto stagingBufferResult = CreateBuffer(newBufferSize, 
				vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eUniformBuffer,
				VMA_MEMORY_USAGE_CPU_ONLY,
				VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
			supports.m_MatrixStagingBuffer = std::move(stagingBufferResult.buffer);
			supports.m_MatrixStagingMemory = std::move(stagingBufferResult.allocation);
			supports.m_MatrixStagingMemoryInfo = stagingBufferResult.allocationInfo;

			// create matrix buffer
			auto matrixBufferResult = CreateBuffer(newBufferSize,
				vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer,
				VMA_MEMORY_USAGE_GPU_ONLY,
				VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
			supports.m_MatrixBuffer = std::move(matrixBufferResult.buffer);
			supports.m_MatrixMemory = std::move(matrixBufferResult.allocation);
			supports.m_MatrixMemoryInfo = matrixBufferResult.allocationInfo;
		}

		vmaMapMemory(m_Allocator.get(), supports.m_MatrixStagingMemory.get(), &m_UpdateData.mapped);

		m_UpdateData.copyOffset = 0;
		m_UpdateData.vp = m_Camera.getMatrix();

		// TODO: benchmark whether staging buffer use is faster than alternatives
		m_LogicalDevice->updateDescriptorSets(
			{ 
				vk::WriteDescriptorSet(
					supports.m_VertexDescriptorSet.get(),
					0U,
					0U,
					1U,
					vk::DescriptorType::eUniformBufferDynamic,
					nullptr,
					&vk::DescriptorBufferInfo(
						supports.m_MatrixBuffer.get(),
						0Ui64,
						m_UniformBufferOffsetAlignment),
					nullptr) 
			},
			{ }
		);

		/////////////////////////////////////////////////////////////
		// set up data that is constant between all command buffers
		//

		// Dynamic viewport info
		auto viewport = vk::Viewport(
			0.f,
			0.f,
			static_cast<float>(m_SurfaceExtent.width),
			static_cast<float>(m_SurfaceExtent.height),
			0.f,
			1.f);

		// Dynamic scissor info
		auto scissorRect = vk::Rect2D(
			vk::Offset2D(0, 0),
			m_SurfaceExtent);

		// clear values for the color and depth attachments
		auto clearValue = vk::ClearValue(vk::ClearColorValue(std::array<float, 4>{0.f, 0.f, 0.f, 0.f}));

		// reset command buffer
		supports.m_CommandBuffer->reset(vk::CommandBufferResetFlags());

		// Render pass info
		auto renderPassInfo = vk::RenderPassBeginInfo(
			m_RenderPass.get(),
			m_Framebuffers[nextImage.value].get(),
			scissorRect,
			1U,
			&clearValue);

		// record the command buffer
		supports.m_CommandBuffer->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
		supports.m_CommandBuffer->setViewport(0U, { viewport });
		supports.m_CommandBuffer->setScissor(0U, { scissorRect });
		supports.m_CommandBuffer->beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);
		supports.m_CommandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, m_Pipeline.get());
		supports.m_CommandBuffer->bindVertexBuffers(
			0U, 
			{ m_VertexBuffer.get() }, 
			{ 0U });
		supports.m_CommandBuffer->bindIndexBuffer(
			m_IndexBuffer.get(), 
			0U, 
			vk::IndexType::eUint32);

		// bind sampler and images uniforms
		supports.m_CommandBuffer->bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,
			m_PipelineLayout.get(),
			1U,
			{ m_FragmentDescriptorSet.get() },
			{});

		m_UpdateData.spriteIndex = 0;
		return true;
	}

	void RenderSprite(const Sprite& sprite)
	{
		auto& supports = m_Supports[m_UpdateData.nextImage];
		glm::mat4 mvp =  m_UpdateData.vp * sprite.transform;

		memcpy((char*)m_UpdateData.mapped + m_UpdateData.copyOffset, &mvp, sizeof(glm::mat4));
		m_UpdateData.copyOffset += m_UniformBufferOffsetAlignment;

		FragmentPushConstants pushRange;
		pushRange.textureID = sprite.textureIndex;
		pushRange.r = sprite.color.r;
		pushRange.g = sprite.color.g;
		pushRange.b = sprite.color.b;
		pushRange.a = sprite.color.a;

		supports.m_CommandBuffer->pushConstants<FragmentPushConstants>(
			m_PipelineLayout.get(),
			vk::ShaderStageFlagBits::eFragment,
			0U,
			{ pushRange });

		uint32_t dynamicOffset = gsl::narrow<uint32_t>(m_UpdateData.spriteIndex * m_UniformBufferOffsetAlignment);

		// bind dynamic matrix uniform
		supports.m_CommandBuffer->bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,
			m_PipelineLayout.get(),
			0U,
			{ supports.m_VertexDescriptorSet.get() },
			{ dynamicOffset });

		auto firstIndex = sprite.textureIndex * IndicesPerQuad;
		// draw the sprite
		supports.m_CommandBuffer->drawIndexed(IndicesPerQuad, 1U, firstIndex, 0U, 0U);
		m_UpdateData.spriteIndex++;
	}

	void EndRender()
	{
		auto& supports = m_Supports[m_UpdateData.nextImage];
		// copy matrices from staging buffer to gpu buffer
		vmaUnmapMemory(m_Allocator.get(), supports.m_MatrixStagingMemory.get());

		CopyToBuffer(
			supports.m_MatrixStagingBuffer.get(),
			supports.m_MatrixBuffer.get(),
			supports.m_MatrixMemoryInfo.size,
			supports.m_MatrixStagingMemoryInfo.offset,
			supports.m_MatrixMemoryInfo.offset,
			{},
		{
			supports.m_MatrixBufferStagingCompleteSemaphore.get() 
		});

		// Finish recording draw command buffer
		supports.m_CommandBuffer->endRenderPass();
		supports.m_CommandBuffer->end();

		// Submit draw command buffer
		m_GraphicsQueue.submit(
			{
				vk::SubmitInfo(
					1U,
					&supports.m_MatrixBufferStagingCompleteSemaphore.get(),
					&vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput),
					1U,
					&supports.m_CommandBuffer.get(),
					1U,
					&supports.m_ImageRenderCompleteSemaphore.get())
			}, vk::Fence());

		// Present image
		auto result = m_GraphicsQueue.presentKHR(
			vk::PresentInfoKHR(
				1U,
				&supports.m_ImageRenderCompleteSemaphore.get(),
				1U,
				&m_Swapchain.get(),
				&m_UpdateData.nextImage));

		if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
		{
			resizeWindow(m_Window.GetClientSize());
		}
		else if (result != vk::Result::eSuccess)
		{
			throw std::runtime_error("failed to present swap chain image!");
		}
	}

	void Run(LoopCallbacks callbacks)
	{
		while (true)
		{
			//loop
			if (!m_Window.PollEvents())
			{
				m_LogicalDevice->waitIdle();
				return;
			}
			auto spriteCount = callbacks.BeforeRenderCallback();
			if (!BeginRender(spriteCount))
			{
				// TODO: probably need to handle some specific errors here
				continue;
			}
			callbacks.RenderCallback(this);
			EndRender();
			callbacks.AfterRenderCallback();
		}
	}

	void createVulkanImageForBitmap(const Bitmap& bitmap,
		VmaAllocationInfo& imageAllocInfo,
		vk::UniqueImage& image2D,
		UniqueVmaAllocation& imageAlloc)
	{
		auto imageInfo = vk::ImageCreateInfo(
			vk::ImageCreateFlags(),
			vk::ImageType::e2D,
			vk::Format::eR8G8B8A8Srgb,
			vk::Extent3D(bitmap.m_Width, bitmap.m_Height, 1U),
			1U,
			1U,
			vk::SampleCountFlagBits::e1,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
			vk::SharingMode::eExclusive,
			1U,
			&m_GraphicsQueueFamilyID,
			vk::ImageLayout::eUndefined);

		auto imageAllocCreateInfo = VmaAllocationCreateInfo{};
		imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		VkImage image;
		VmaAllocation imageAllocation;

		vmaCreateImage(m_Allocator.get(),
			&imageInfo.operator const VkImageCreateInfo &(),
			&imageAllocCreateInfo,
			&image,
			&imageAllocation,
			&imageAllocInfo);

		auto imageDeleter = vk::ImageDeleter(m_LogicalDevice.get());
		image2D = vk::UniqueImage(vk::Image(image), imageDeleter);
		imageAlloc = UniqueVmaAllocation(imageAllocation, VmaAllocationDeleter(m_Allocator.get()));
	}

	TextureIndex createTexture(const Bitmap & bitmap)
	{
		TextureIndex textureIndex = static_cast<uint32_t>(m_Textures.size());
		// create vke::Image2D
		auto& image2D = m_Images.emplace_back();
		auto& imageView = m_ImageViews.emplace_back();
		auto& imageAlloc = m_ImageAllocations.emplace_back();
		auto& imageAllocInfo = m_ImageAllocationInfos.emplace_back();
		auto& texture = m_Textures.emplace_back();

		createVulkanImageForBitmap(bitmap, imageAllocInfo, image2D, imageAlloc);

		float left = static_cast<float>(bitmap.m_Left);
		float top = static_cast<float>(bitmap.m_Top);
		float width = static_cast<float>(bitmap.m_Width);
		float height = static_cast<float>(bitmap.m_Height);
		float right = left + width;
		float bottom = top - height;

		glm::vec2 LeftTopPos, LeftBottomPos, RightBottomPos, RightTopPos;
		LeftTopPos = { left,		top };
		LeftBottomPos = { left,		bottom };
		RightBottomPos = { right,	bottom };
		RightTopPos = { right,	top };

		glm::vec2 LeftTopUV, LeftBottomUV, RightBottomUV, RightTopUV;
		LeftTopUV = { 0.0f, 0.0f };
		LeftBottomUV = { 0.0f, 1.0f };
		RightBottomUV = { 1.0f, 1.0f };
		RightTopUV = { 1.0f, 0.0f };

		Vertex LeftTop, LeftBottom, RightBottom, RightTop;
		LeftTop = { LeftTopPos,		LeftTopUV };
		LeftBottom = { LeftBottomPos,	LeftBottomUV };
		RightBottom = { RightBottomPos,	RightBottomUV };
		RightTop = { RightTopPos,		RightTopUV };

		// push back vertices
		m_Vertices.push_back(LeftTop);
		m_Vertices.push_back(LeftBottom);
		m_Vertices.push_back(RightBottom);
		m_Vertices.push_back(RightTop);

		auto verticesPerTexture = 4U;
		uint32_t indexOffset = verticesPerTexture * textureIndex;

		// push back indices
		m_Indices.push_back(indexOffset + 0);
		m_Indices.push_back(indexOffset + 1);
		m_Indices.push_back(indexOffset + 2);
		m_Indices.push_back(indexOffset + 2);
		m_Indices.push_back(indexOffset + 3);
		m_Indices.push_back(indexOffset + 0);

		// initialize the texture object
		texture.index = textureIndex;
		texture.width = bitmap.m_Width;
		texture.height = bitmap.m_Height;
		texture.size = imageAllocInfo.size;

		auto stagingBufferResult = CreateBuffer(texture.size,
			vk::BufferUsageFlagBits::eTransferSrc,
			VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_ONLY,
			VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
		// copy from host to staging buffer
		void* stagingBufferData;
		vmaMapMemory(m_Allocator.get(), stagingBufferResult.allocation.get(), &stagingBufferData);
		memcpy(stagingBufferData, bitmap.m_Data.data(), static_cast<size_t>(bitmap.m_Size));
		vmaUnmapMemory(m_Allocator.get(), stagingBufferResult.allocation.get());

		// get command pool and buffer
		auto command = CreateTemporaryCommandBuffer();

		// begin recording command buffer
		command.buffer->begin(vk::CommandBufferBeginInfo(
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

		// transition image layout
		auto memoryBarrier = vk::ImageMemoryBarrier(
			vk::AccessFlags(),
			vk::AccessFlagBits::eTransferWrite,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eTransferDstOptimal,
			m_GraphicsQueueFamilyID,
			m_GraphicsQueueFamilyID,
			image2D.get(),
			vk::ImageSubresourceRange(
				vk::ImageAspectFlagBits::eColor,
				0U,
				1U,
				0U,
				1U));
		command.buffer->pipelineBarrier(
			vk::PipelineStageFlagBits::eTopOfPipe,
			vk::PipelineStageFlagBits::eTransfer,
			vk::DependencyFlags(),
			{ },
			{ },
			{ memoryBarrier });

		// copy from host to device (to image)
		command.buffer->copyBufferToImage(stagingBufferResult.buffer.get(), image2D.get(), vk::ImageLayout::eTransferDstOptimal,
			{
				vk::BufferImageCopy(0U, 0U, 0U,
				vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0U, 0U, 1U),
				vk::Offset3D(),
				vk::Extent3D(bitmap.m_Width, bitmap.m_Height, 1U))
			});

		// transition image for shader access
		auto memoryBarrier2 = vk::ImageMemoryBarrier(
			vk::AccessFlagBits::eTransferWrite,
			vk::AccessFlagBits::eShaderRead,
			vk::ImageLayout::eTransferDstOptimal,
			vk::ImageLayout::eShaderReadOnlyOptimal,
			m_GraphicsQueueFamilyID,
			m_GraphicsQueueFamilyID,
			image2D.get(),
			vk::ImageSubresourceRange(
				vk::ImageAspectFlagBits::eColor,
				0U,
				1U,
				0U,
				1U));
		command.buffer->pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eFragmentShader,
			vk::DependencyFlags(),
			{ },
			{ },
			{ memoryBarrier2 });

		command.buffer->end();

		// create fence, submit command buffer
		auto imageLoadFence = m_LogicalDevice->createFenceUnique(vk::FenceCreateInfo());
		m_GraphicsQueue.submit(
			vk::SubmitInfo(0U, nullptr, nullptr,
				1U, &command.buffer.get(),
				0U, nullptr),
			imageLoadFence.get());

		// create image view
		imageView = m_LogicalDevice->createImageViewUnique(
			vk::ImageViewCreateInfo(
				vk::ImageViewCreateFlags(),
				image2D.get(),
				vk::ImageViewType::e2D,
				vk::Format::eR8G8B8A8Srgb,
				vk::ComponentMapping(),
				vk::ImageSubresourceRange(
					vk::ImageAspectFlagBits::eColor,
					0U,
					1U,
					0U,
					1U)));

		// wait for command buffer to be executed
		m_LogicalDevice->waitForFences({ imageLoadFence.get() }, true, std::numeric_limits<uint64_t>::max());

		auto fenceStatus = m_LogicalDevice->getFenceStatus(imageLoadFence.get());

		return textureIndex;
	}

	vk::UniqueShaderModule createShaderModule(const std::string& path)
	{
		// read from file
		auto shaderBinary = fileIO::readFile(path);
		// create shader module
		auto shaderInfo = vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(),
			shaderBinary.size(),
			reinterpret_cast<const uint32_t*>(shaderBinary.data()));
		return m_LogicalDevice->createShaderModuleUnique(shaderInfo);
	}

	vk::UniqueDescriptorPool createVertexDescriptorPool()
	{
		auto poolSizes = std::vector<vk::DescriptorPoolSize>
		{
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1U)
		};

		return m_LogicalDevice->createDescriptorPoolUnique(
			vk::DescriptorPoolCreateInfo(
				vk::DescriptorPoolCreateFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet),
				BufferCount,
				gsl::narrow<uint32_t>(poolSizes.size()),
				poolSizes.data()));
	}

	vk::UniqueDescriptorPool createFragmentDescriptorPool(const vk::Device& logicalDevice, uint32_t textureCount)
	{
		auto poolSizes = std::vector<vk::DescriptorPoolSize>
		{
			vk::DescriptorPoolSize(vk::DescriptorType::eSampler, 1U),
			vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, textureCount)
		};

		return logicalDevice.createDescriptorPoolUnique(
			vk::DescriptorPoolCreateInfo(
				vk::DescriptorPoolCreateFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet),
				1U,
				gsl::narrow<uint32_t>(poolSizes.size()),
				poolSizes.data()));
	}

	vk::UniqueDescriptorSetLayout createVertexSetLayout(const vk::Device& logicalDevice)
	{
		auto vertexLayoutBindings = std::vector<vk::DescriptorSetLayoutBinding>
		{
			// Vertex: matrix uniform buffer (dynamic)
			vk::DescriptorSetLayoutBinding(
				0U,
				vk::DescriptorType::eUniformBufferDynamic,
				1U,
				vk::ShaderStageFlagBits::eVertex)
		};

		return logicalDevice.createDescriptorSetLayoutUnique(
			vk::DescriptorSetLayoutCreateInfo(
				vk::DescriptorSetLayoutCreateFlags(),
				gsl::narrow<uint32_t>(vertexLayoutBindings.size()),
				vertexLayoutBindings.data()));
	}

	vk::UniqueDescriptorSetLayout createFragmentSetLayout(
		const vk::Device& logicalDevice,
		const uint32_t textureCount,
		const vk::Sampler sampler)
	{
		auto fragmentLayoutBindings = std::vector<vk::DescriptorSetLayoutBinding>
		{
			// Fragment: sampler uniform buffer
			vk::DescriptorSetLayoutBinding(
				0U,
				vk::DescriptorType::eSampler,
				1U,
				vk::ShaderStageFlagBits::eFragment,
				&sampler),
			// Fragment: texture uniform buffer (array)
			vk::DescriptorSetLayoutBinding(
				1U,
				vk::DescriptorType::eSampledImage,
				textureCount,
				vk::ShaderStageFlagBits::eFragment)
		};

		return logicalDevice.createDescriptorSetLayoutUnique(
			vk::DescriptorSetLayoutCreateInfo(
				vk::DescriptorSetLayoutCreateFlags(),
				gsl::narrow<uint32_t>(fragmentLayoutBindings.size()),
				fragmentLayoutBindings.data()));
	}


	void createRenderPass()
	{
		// create render pass
		auto colorAttachmentDescription = vk::AttachmentDescription(
			vk::AttachmentDescriptionFlags(),
			m_SurfaceFormat,
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::ePresentSrcKHR
		);
		auto colorAttachmentRef = vk::AttachmentReference(
			0,
			vk::ImageLayout::eColorAttachmentOptimal);

		auto subpassDescription = vk::SubpassDescription(
			vk::SubpassDescriptionFlags(),
			vk::PipelineBindPoint::eGraphics,
			0,
			nullptr,
			1U,
			&colorAttachmentRef,
			nullptr,
			nullptr,
			0,
			nullptr
		);

		std::vector<vk::SubpassDependency> dependencies =
		{
			vk::SubpassDependency(
				0U,
				0U,
				vk::PipelineStageFlagBits::eBottomOfPipe,
				vk::PipelineStageFlagBits::eColorAttachmentOutput,
				vk::AccessFlagBits::eMemoryRead,
				vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
				vk::DependencyFlagBits::eByRegion
			),
			vk::SubpassDependency(
				0U,
				0U,
				vk::PipelineStageFlagBits::eColorAttachmentOutput,
				vk::PipelineStageFlagBits::eBottomOfPipe,
				vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
				vk::AccessFlagBits::eMemoryRead,
				vk::DependencyFlagBits::eByRegion
			)
		};

		auto renderPassInfo = vk::RenderPassCreateInfo(
			vk::RenderPassCreateFlags(),
			1U,
			&colorAttachmentDescription,
			1U,
			&subpassDescription,
			gsl::narrow<uint32_t>(dependencies.size()),
			dependencies.data());
		m_RenderPass = m_LogicalDevice->createRenderPassUnique(renderPassInfo);
	}

	void createPipelineCreateInfo()
	{
		m_FragmentSpecializations.emplace_back(vk::SpecializationMapEntry(
			0U,
			0U,
			sizeof(glm::uint32)));

		m_VertexSpecializationInfo = vk::SpecializationInfo();
		m_FragmentSpecializationInfo = vk::SpecializationInfo(
			gsl::narrow<uint32_t>(m_FragmentSpecializations.size()),
			m_FragmentSpecializations.data(),
			gsl::narrow<uint32_t>(sizeof(glm::uint32)),
			&m_TextureCount
		);

		m_VertexShaderStageInfo = vk::PipelineShaderStageCreateInfo(
			vk::PipelineShaderStageCreateFlags(),
			vk::ShaderStageFlagBits::eVertex,
			m_VertexShader.get(),
			"main",
			&m_VertexSpecializationInfo);

		m_FragmentShaderStageInfo = vk::PipelineShaderStageCreateInfo(
			vk::PipelineShaderStageCreateFlags(),
			vk::ShaderStageFlagBits::eFragment,
			m_FragmentShader.get(),
			"main",
			&m_FragmentSpecializationInfo);

		m_PipelineShaderStageInfo = std::vector<vk::PipelineShaderStageCreateInfo>
		{
			m_VertexShaderStageInfo, 
			m_FragmentShaderStageInfo 
		};

		m_VertexBindings = Vertex::vertexBindingDescriptions();
		m_VertexAttributes = Vertex::vertexAttributeDescriptions();
		m_PipelineVertexInputInfo = vk::PipelineVertexInputStateCreateInfo(
			vk::PipelineVertexInputStateCreateFlags(),
			gsl::narrow<uint32_t>(m_VertexBindings.size()),
			m_VertexBindings.data(),
			gsl::narrow<uint32_t>(m_VertexAttributes.size()),
			m_VertexAttributes.data());

		m_PipelineInputAssemblyInfo = vk::PipelineInputAssemblyStateCreateInfo(
			vk::PipelineInputAssemblyStateCreateFlags(),
			vk::PrimitiveTopology::eTriangleList);

		m_PipelineTesselationStateInfo = vk::PipelineTessellationStateCreateInfo(
			vk::PipelineTessellationStateCreateFlags());

		m_PipelineViewportInfo = vk::PipelineViewportStateCreateInfo(
			vk::PipelineViewportStateCreateFlags(),
			1U,
			nullptr,
			1U,
			nullptr);

		m_PipelineRasterizationInfo = vk::PipelineRasterizationStateCreateInfo(
			vk::PipelineRasterizationStateCreateFlags(),
			false,
			false,
			vk::PolygonMode::eFill,
			vk::CullModeFlagBits::eNone,
			vk::FrontFace::eCounterClockwise,
			false,
			0.f,
			0.f,
			0.f,
			1.f);

		m_PipelineMultisampleInfo = vk::PipelineMultisampleStateCreateInfo();

		m_PipelineDepthStencilInfo = vk::PipelineDepthStencilStateCreateInfo();

		m_PipelineColorBlendAttachmentState = vk::PipelineColorBlendAttachmentState(
			true,
			vk::BlendFactor::eSrcAlpha,
			vk::BlendFactor::eOneMinusSrcAlpha,
			vk::BlendOp::eAdd,
			vk::BlendFactor::eZero,
			vk::BlendFactor::eZero,
			vk::BlendOp::eAdd,
			vk::ColorComponentFlagBits::eR |
			vk::ColorComponentFlagBits::eG |
			vk::ColorComponentFlagBits::eB |
			vk::ColorComponentFlagBits::eA);

		m_PipelineBlendStateInfo = vk::PipelineColorBlendStateCreateInfo(
			vk::PipelineColorBlendStateCreateFlags(),
			0U,
			vk::LogicOp::eClear,
			1U,
			&m_PipelineColorBlendAttachmentState);

		m_DynamicStates = std::vector<vk::DynamicState>{ vk::DynamicState::eViewport, vk::DynamicState::eScissor };

		m_PipelineDynamicStateInfo = vk::PipelineDynamicStateCreateInfo(
			vk::PipelineDynamicStateCreateFlags(),
			gsl::narrow<uint32_t>(m_DynamicStates.size()),
			m_DynamicStates.data());

		m_PipelineCreateInfo = vk::GraphicsPipelineCreateInfo(
			vk::PipelineCreateFlags(),
			gsl::narrow<uint32_t>(m_PipelineShaderStageInfo.size()),
			m_PipelineShaderStageInfo.data(),
			&m_PipelineVertexInputInfo,
			&m_PipelineInputAssemblyInfo,
			&m_PipelineTesselationStateInfo,
			&m_PipelineViewportInfo,
			&m_PipelineRasterizationInfo,
			&m_PipelineMultisampleInfo,
			&m_PipelineDepthStencilInfo,
			&m_PipelineBlendStateInfo,
			&m_PipelineDynamicStateInfo,
			m_PipelineLayout.get(),
			m_RenderPass.get()
		);
	}

	void CreateVertexBuffer()
	{
		// create vertex buffers
		auto vertexBufferSize = sizeof(Vertex) * m_Vertices.size();
		auto vertexStagingResult = CreateBuffer(vertexBufferSize,
			vk::BufferUsageFlagBits::eVertexBuffer |
			vk::BufferUsageFlagBits::eTransferSrc,
			VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_ONLY,
			VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
		auto vertexResult = CreateBuffer(vertexBufferSize,
			vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
			VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY,
			0);
		// copy data to vertex buffer
		void* vertexStagingData;
		vmaMapMemory(m_Allocator.get(), vertexStagingResult.allocation.get(), &vertexStagingData);
		memcpy(vertexStagingData, m_Vertices.data(), vertexBufferSize);
		vmaUnmapMemory(m_Allocator.get(), vertexStagingResult.allocation.get());
		CopyToBuffer(vertexStagingResult.buffer.get(),
			vertexResult.buffer.get(),
			vertexBufferSize,
			vertexStagingResult.allocationInfo.offset,
			vertexResult.allocationInfo.offset);
		// transfer ownership to VulkanApp
		auto vertexBufferDeleter = vk::BufferDeleter(m_LogicalDevice.get());
		m_VertexBuffer = vk::UniqueBuffer(vertexResult.buffer.release(), vertexBufferDeleter);
		m_VertexMemoryInfo = vertexResult.allocationInfo;
	}

	void CreateIndexBuffer()
	{
		//create index buffers
		auto indexBufferSize = sizeof(Index) * m_Indices.size();
		auto indexStagingResult = CreateBuffer(indexBufferSize,
			vk::BufferUsageFlagBits::eTransferSrc |
			vk::BufferUsageFlagBits::eIndexBuffer,
			VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_ONLY,
			VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
		auto indexResult = CreateBuffer(indexBufferSize,
			vk::BufferUsageFlagBits::eTransferDst |
			vk::BufferUsageFlagBits::eIndexBuffer,
			VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY,
			0);

		// copy data to index buffer
		void* indexStagingData;
		vmaMapMemory(m_Allocator.get(), indexStagingResult.allocation.get(), &indexStagingData);
		memcpy(indexStagingData, m_Indices.data(), indexBufferSize);
		vmaUnmapMemory(m_Allocator.get(), indexStagingResult.allocation.get());
		CopyToBuffer(indexStagingResult.buffer.get(),
			indexResult.buffer.get(),
			indexBufferSize,
			indexStagingResult.allocationInfo.offset,
			indexResult.allocationInfo.offset);
		// transfer ownership to VulkanApp
		auto indexBufferDeleter = vk::BufferDeleter(m_LogicalDevice.get());
		m_IndexBuffer = vk::UniqueBuffer(indexResult.buffer.release(), indexBufferDeleter);
		m_IndexMemoryInfo = indexResult.allocationInfo;
	}

	void createSwapChain()
	{
		vk::Bool32 presentSupport;
		vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, m_GraphicsQueueFamilyID, m_Surface.get(), &presentSupport);
		if (!presentSupport)
		{
			std::runtime_error("Presentation to this surface isn't supported.");
			exit(1);
		}
		m_Swapchain = m_LogicalDevice->createSwapchainKHRUnique(
			vk::SwapchainCreateInfoKHR(
				vk::SwapchainCreateFlagsKHR(),
				m_Surface.get(),
				BufferCount,
				m_SurfaceFormat,
				m_SurfaceColorSpace,
				m_SurfaceExtent,
				1U,
				vk::ImageUsageFlagBits::eColorAttachment,
				vk::SharingMode::eExclusive,
				1U,
				&m_GraphicsQueueFamilyID,
				vk::SurfaceTransformFlagBitsKHR::eIdentity,
				vk::CompositeAlphaFlagBitsKHR::eOpaque,
				m_PresentMode,
				0U//,
				/*m_Swapchain.get()*/
				));
	}

	void createSwapchainDependencies()
	{
		//for (auto& framebuffer : m_Framebuffers)
		//{
		//	framebuffer.reset();
		//}
		//for (auto& swapView : m_SwapViews)
		//{
		//	swapView.reset();
		//}
		//
		createSwapChain();
		m_SwapImages = m_LogicalDevice->getSwapchainImagesKHR(m_Swapchain.get());

		for (auto i = 0U; i < BufferCount; i++)
		{
			// create swap image view
			auto viewInfo = vk::ImageViewCreateInfo(
				vk::ImageViewCreateFlags(),
				m_SwapImages[i],
				vk::ImageViewType::e2D,
				m_SurfaceFormat,
				vk::ComponentMapping(),
				vk::ImageSubresourceRange(
					vk::ImageAspectFlagBits::eColor,
					0U,
					1U,
					0U,
					1U));
			m_SwapViews[i] = m_LogicalDevice->createImageViewUnique(viewInfo);

			// create framebuffer
			auto fbInfo = vk::FramebufferCreateInfo(
				vk::FramebufferCreateFlags(),
				m_RenderPass.get(),
				1U,
				&m_SwapViews[i].get(),
				m_SurfaceExtent.width,
				m_SurfaceExtent.height,
				1U);
			m_Framebuffers[i] = m_LogicalDevice->createFramebufferUnique(fbInfo);
		}

	}

	void resizeWindow(ClientSize size)
	{
		m_SurfaceExtent = vk::Extent2D{ gsl::narrow_cast<uint32_t>(size.width), gsl::narrow_cast<uint32_t>(size.height) };
		
	}

	void CreateAndUpdateFragmentDescriptorSet()
	{
		// create fragment descriptor set
		m_FragmentDescriptorSet = std::move(
			m_LogicalDevice->allocateDescriptorSetsUnique(
				vk::DescriptorSetAllocateInfo(
					m_FragmentLayoutDescriptorPool.get(),
					1U,
					&m_FragmentDescriptorSetLayout.get()))[0]);

		// update fragment descriptor set
			// binding 0 (sampler)
		m_LogicalDevice->updateDescriptorSets(
			{
				vk::WriteDescriptorSet(
					m_FragmentDescriptorSet.get(),
					0U,
					0U,
					1U,
					vk::DescriptorType::eSampler,
					&vk::DescriptorImageInfo(m_Sampler.get()))
			},
			{}
		);
			// binding 1 (images)
		auto imageInfos = std::vector<vk::DescriptorImageInfo>();
		imageInfos.reserve(m_TextureCount);
		for (auto i = 0U; i < m_TextureCount; i++)
		{
			imageInfos.emplace_back(vk::DescriptorImageInfo(
				nullptr,
				m_ImageViews[i].get(),
				vk::ImageLayout::eShaderReadOnlyOptimal));
		}
		m_LogicalDevice->updateDescriptorSets(
			{
				vk::WriteDescriptorSet(
					m_FragmentDescriptorSet.get(),
					1U,
					0U,
					gsl::narrow<uint32_t>(imageInfos.size()),
					vk::DescriptorType::eSampledImage,
					imageInfos.data())
			},
			{}
		);
	}

	void InitializeSupportStruct(Supports& support)
	{
		support.m_CommandPool = m_LogicalDevice->createCommandPoolUnique(
			vk::CommandPoolCreateInfo(
				vk::CommandPoolCreateFlagBits::eResetCommandBuffer |
				vk::CommandPoolCreateFlagBits::eTransient,
				m_GraphicsQueueFamilyID));
		support.m_CommandBuffer = std::move(m_LogicalDevice->allocateCommandBuffersUnique(
			vk::CommandBufferAllocateInfo(
				support.m_CommandPool.get(),
				vk::CommandBufferLevel::ePrimary,
				1U))[0]);
		support.m_VertexLayoutDescriptorPool = createVertexDescriptorPool();
		support.m_BufferCapacity = 0U;
		support.m_ImageRenderCompleteSemaphore = m_LogicalDevice->createSemaphoreUnique(
			vk::SemaphoreCreateInfo(
				vk::SemaphoreCreateFlags()));
		support.m_MatrixBufferStagingCompleteSemaphore = m_LogicalDevice->createSemaphoreUnique(
			vk::SemaphoreCreateInfo(
				vk::SemaphoreCreateFlags()));
		support.m_VertexLayoutDescriptorPool = createVertexDescriptorPool();
		support.m_VertexDescriptorSet = vk::UniqueDescriptorSet(m_LogicalDevice->allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				support.m_VertexLayoutDescriptorPool.get(),
				1U,
				&m_VertexDescriptorSetLayout.get()))[0],
			vk::DescriptorSetDeleter(m_LogicalDevice.get(),
				support.m_VertexLayoutDescriptorPool.get()));
	}

	// Window resize callback
	static void resizeFunc(void* ptr, ClientSize size)
	{
		((VulkanApp*)ptr)->resizeWindow(size);
	};

	void init(InitData initData)
	{
		m_Window = Window(
			initData.WindowClassName, 
			initData.WindowTitle, 
			initData.windowStyle, 
			resizeFunc,
			this, 
			initData.width, 
			initData.height);
			m_Camera = Camera2D();
			m_Camera.setSize({static_cast<float>(initData.width), static_cast<float>(initData.height)});

			// load vulkan dll, entry point, and instance/global function pointers
			if (!LoadVulkanDLL())
			{
				exit(1);
			}
			LoadVulkanEntryPoint();
			LoadVulkanGlobalFunctions();
			m_Instance = vk::createInstanceUnique(initData.instanceCreateInfo);
			LoadVulkanInstanceFunctions();

			// select physical device
			auto devices = m_Instance->enumeratePhysicalDevices();
			m_PhysicalDevice = devices[0];

			m_UniformBufferOffsetAlignment = m_PhysicalDevice.getProperties().limits.minUniformBufferOffsetAlignment;

			// find graphics queue
			auto gpuQueueProperties = m_PhysicalDevice.getQueueFamilyProperties();
			auto graphicsQueueInfo = std::find_if(gpuQueueProperties.begin(), gpuQueueProperties.end(),
				[&](vk::QueueFamilyProperties q)
			{
				return q.queueFlags & vk::QueueFlagBits::eGraphics;
			});

			// this gets the position of the queue, which is also its family ID
			m_GraphicsQueueFamilyID = static_cast<uint32_t>(graphicsQueueInfo - gpuQueueProperties.begin());
			// priority of 0 since we only have one queue
			auto graphicsPriority = 0.f;
			auto graphicsQueueCreateInfo = vk::DeviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), m_GraphicsQueueFamilyID, 1U, &graphicsPriority);

			// create Logical Device
			auto gpuFeatures = m_PhysicalDevice.getFeatures();
			m_LogicalDevice = m_PhysicalDevice.createDeviceUnique(
				vk::DeviceCreateInfo(vk::DeviceCreateFlags(),
					1,
					&graphicsQueueCreateInfo,
					0,
					nullptr,
					static_cast<uint32_t>(initData.deviceExtensions.size()),
					initData.deviceExtensions.data(),
					&gpuFeatures)
			);
			// Load device-dependent vulkan function pointers
			LoadVulkanDeviceFunctions();

			// get the queue from the logical device
			m_GraphicsQueue = m_LogicalDevice->getQueue(m_GraphicsQueueFamilyID, 0);

			// initialize debug reporting
			if (vkCreateDebugReportCallbackEXT)
			{
				m_DebugBreakpointCallbackData = m_Instance->createDebugReportCallbackEXTUnique(
					vk::DebugReportCallbackCreateInfoEXT(
						vk::DebugReportFlagBitsEXT::ePerformanceWarning |
						vk::DebugReportFlagBitsEXT::eError,
						//vk::DebugReportFlagBitsEXT::eDebug |
						//vk::DebugReportFlagBitsEXT::eInformation |
						//vk::DebugReportFlagBitsEXT::eWarning,
						reinterpret_cast<PFN_vkDebugReportCallbackEXT>(&debugBreakCallback)
					));
			}

			// create the allocator
			auto makeAllocator = [&]()
			{
				VmaAllocatorCreateInfo allocatorInfo = {};
				allocatorInfo.physicalDevice = m_PhysicalDevice.operator VkPhysicalDevice();
				allocatorInfo.device = m_LogicalDevice->operator VkDevice();
				allocatorInfo.frameInUseCount = 1U;
				VmaVulkanFunctions vulkanFunctions = {};
				vulkanFunctions.vkAllocateMemory = vkAllocateMemory;
				vulkanFunctions.vkBindBufferMemory = vkBindBufferMemory;
				vulkanFunctions.vkBindImageMemory = vkBindImageMemory;
				vulkanFunctions.vkCreateBuffer = vkCreateBuffer;
				vulkanFunctions.vkCreateImage = vkCreateImage;
				vulkanFunctions.vkDestroyBuffer = vkDestroyBuffer;
				vulkanFunctions.vkDestroyImage = vkDestroyImage;
				vulkanFunctions.vkFreeMemory = vkFreeMemory;
				vulkanFunctions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
				vulkanFunctions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR;
				vulkanFunctions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
				vulkanFunctions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
				vulkanFunctions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
				vulkanFunctions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
				vulkanFunctions.vkMapMemory = vkMapMemory;
				vulkanFunctions.vkUnmapMemory = vkUnmapMemory;
				allocatorInfo.pVulkanFunctions = &vulkanFunctions;

				VmaAllocator alloc;
				vmaCreateAllocator(&allocatorInfo, &alloc);
				m_Allocator = UniqueVmaAllocator(alloc);
			};
			makeAllocator();

			// allow client app to load images
			initData.imageLoadCallback(this);
			m_TextureCount = gsl::narrow<uint32_t>(m_Images.size());

			CreateVertexBuffer();

			CreateIndexBuffer();

			// create surface
			auto inst = m_Window.getHINSTANCE();
			auto hwnd = m_Window.getHWND();
			auto surfaceCreateInfo = vk::Win32SurfaceCreateInfoKHR(vk::Win32SurfaceCreateFlagsKHR(), inst, hwnd);
			m_Surface = m_Instance->createWin32SurfaceKHRUnique(surfaceCreateInfo);
			// get surface format from supported list
			auto surfaceFormats = m_PhysicalDevice.getSurfaceFormatsKHR(m_Surface.get());
			if (surfaceFormats.size() == 1 && surfaceFormats[0].format == vk::Format::eUndefined)
				m_SurfaceFormat = vk::Format::eB8G8R8A8Unorm;
			else
				m_SurfaceFormat = surfaceFormats[0].format;
			// get surface color space
			m_SurfaceColorSpace = surfaceFormats[0].colorSpace;

			m_SurfaceFormatProperties = m_PhysicalDevice.getFormatProperties(vk::Format::eR8G8B8A8Unorm);
			m_SurfaceCapabilities = m_PhysicalDevice.getSurfaceCapabilitiesKHR(m_Surface.get());
			m_SurfacePresentModes = m_PhysicalDevice.getSurfacePresentModesKHR(m_Surface.get());

			if (!(m_SurfaceFormatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachmentBlend))
			{
				std::runtime_error("Blend not supported by surface format.");
			}
			// get surface extent
			if (m_SurfaceCapabilities.currentExtent.width == -1 ||
				m_SurfaceCapabilities.currentExtent.height == -1)
			{
				auto size = m_Window.GetClientSize();
				m_SurfaceExtent = vk::Extent2D
				{
					static_cast<uint32_t>(size.width),
					static_cast<uint32_t>(size.height)
				};
			}
			else
			{
				m_SurfaceExtent = m_SurfaceCapabilities.currentExtent;
			}

			createRenderPass();

			// create sampler
			m_Sampler = m_LogicalDevice.get().createSamplerUnique(
				vk::SamplerCreateInfo(
					vk::SamplerCreateFlags(),
					vk::Filter::eLinear,
					vk::Filter::eLinear,
					vk::SamplerMipmapMode::eLinear,
					vk::SamplerAddressMode::eRepeat,
					vk::SamplerAddressMode::eRepeat,
					vk::SamplerAddressMode::eRepeat,
					0.f,
					1U,
					16.f,
					0U,
					vk::CompareOp::eNever));

			// create shader modules
			m_VertexShader = createShaderModule(initData.shaderData.vertexShaderPath);
			m_FragmentShader = createShaderModule(initData.shaderData.fragmentShaderPath);

			// create descriptor set layouts
			m_VertexDescriptorSetLayout = createVertexSetLayout(m_LogicalDevice.get());
			m_FragmentDescriptorSetLayout = createFragmentSetLayout(m_LogicalDevice.get(), gsl::narrow<uint32_t>(m_Textures.size()), m_Sampler.get());
			m_DescriptorSetLayouts.push_back(m_VertexDescriptorSetLayout.get());
			m_DescriptorSetLayouts.push_back(m_FragmentDescriptorSetLayout.get());

			// setup push constant ranges
			m_PushConstantRanges.emplace_back(vk::PushConstantRange(
				vk::ShaderStageFlagBits::eFragment,
				0U,
				sizeof(FragmentPushConstants)));

			// create fragment layout descriptor pool
			m_FragmentLayoutDescriptorPool = createFragmentDescriptorPool(
				m_LogicalDevice.get(),
				m_TextureCount);

			CreateAndUpdateFragmentDescriptorSet();

			// default present mode: immediate
			m_PresentMode = vk::PresentModeKHR::eImmediate;

			// use mailbox present mode if supported
			auto mailboxPresentMode = std::find(
				m_SurfacePresentModes.begin(), 
				m_SurfacePresentModes.end(), 
				vk::PresentModeKHR::eMailbox);
			if (mailboxPresentMode != m_SurfacePresentModes.end())
			{
				m_PresentMode = *mailboxPresentMode;
			}

			// create pipeline layout
			auto pipelineLayoutInfo = vk::PipelineLayoutCreateInfo(
				vk::PipelineLayoutCreateFlags(),
				gsl::narrow<uint32_t>(m_DescriptorSetLayouts.size()),
				m_DescriptorSetLayouts.data(),
				gsl::narrow<uint32_t>(m_PushConstantRanges.size()),
				m_PushConstantRanges.data());
			m_PipelineLayout = m_LogicalDevice->createPipelineLayoutUnique(pipelineLayoutInfo);

			// init all the swapchain dependencies
			createSwapchainDependencies();

			// create pipeline
			createPipelineCreateInfo();
			m_Pipeline = m_LogicalDevice->createGraphicsPipelineUnique(vk::PipelineCache(), 
				m_PipelineCreateInfo);

			// init each supports struct
			for (auto& support : m_Supports)
			{
				InitializeSupportStruct(support);
			}
	}
};

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

}