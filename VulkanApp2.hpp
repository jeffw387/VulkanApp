#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "VulkanFunctions.hpp"
#include "vulkan/vulkan.hpp"
#include "GLFW/glfw3.h"
#include "fileIO.h"
#include "Texture2D.hpp"
#include "Bitmap.hpp"
#include "Sprite.hpp"
#include "Camera.hpp"
#include "ECS.hpp"
#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include "Allocator.hpp"
#include "profiler.hpp"
#include "mymath.hpp"
#include "CircularQueue.hpp"

#undef max
#include <iostream>
#include <map>
#include <functional>
#include <limits>
#include <thread>
#include <variant>
#include <chrono>
#include <ratio>

namespace vka
{
	using SpriteCount = size_t;
	using ImageIndex = uint32_t;
	constexpr auto MillisecondsPerSecond = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(1));
	constexpr auto UpdateDuration = MillisecondsPerSecond / 50;
	using VertexIndex = uint16_t;
	using InputTime = std::chrono::milliseconds;
	using Clock = std::chrono::steady_clock;

	// store input time in milliseconds
	// consume x milliseconds of input at a time
	// x = milliseconds per update
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
			auto bindingDescriptions = std::vector<vk::VertexInputBindingDescription>
			{ 
				vk::VertexInputBindingDescription(0U, sizeof(Vertex), vk::VertexInputRate::eVertex) 
			};
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

	constexpr VertexIndex IndicesPerQuad = 6U;
	constexpr std::array<VertexIndex, IndicesPerQuad> IndexArray = 
	{
		0, 1, 2, 2, 3, 0
	};

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

	struct KeyMessage
	{
		int key = 0;
		int scancode = 0;
		int action = 0;
		int mods = 0;

		KeyMessage() noexcept = default;
		KeyMessage(int&& key, int&& scancode, int&& action, int&& mods) noexcept : 
			key(std::move(key)), 
			scancode(std::move(scancode)), 
			action(std::move(action)), 
			mods(std::move(mods))
		{}
	};

	struct CharMessage
	{
		unsigned int codepoint = 0;

		CharMessage() noexcept = default;
		CharMessage(int&& codepoint) :
			codepoint(std::move(codepoint))
		{}
	};

	struct CursorPosMessage
	{
		double xpos = 0.0;
		double ypos = 0.0;

		CursorPosMessage() noexcept = default;
		CursorPosMessage(double&& xpos, double&& ypos) : 
			xpos(std::move(xpos)), ypos(std::move(ypos))
		{}
	};

	struct MouseButtonMessage
	{
		int button = 0;
		int action = 0;
		int mods = 0;

		MouseButtonMessage() noexcept = default;
		MouseButtonMessage(int&& button, int&& action, int&& mods) : 
			button(std::move(button)),
			action(std::move(action)),
			mods(std::move(mods))
		{}
	};

	using InputMsgVariant = std::variant<
		KeyMessage, 
		CharMessage, 
		CursorPosMessage, 
		MouseButtonMessage>;

	struct InputMessage
	{
		InputTime time;
		InputMsgVariant variant;
	};

	static VulkanApp* GetUserPointer(GLFWwindow* window)
	{
		return (VulkanApp*)glfwGetWindowUserPointer(window);
	}

	static void PushBackInput(GLFWwindow* window, InputMessage&& msg);

	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		InputMessage msg;
		msg.variant = KeyMessage(std::move(key), std::move(scancode), std::move(action), std::move(mods));
		PushBackInput(window, std::move(msg));
	}

	static void CharacterCallback(GLFWwindow* window, unsigned int codepoint)
	{
		InputMessage msg;
		msg.variant = CharMessage(std::move(codepoint));
		PushBackInput(window, std::move(msg));
	}

	static void CursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
	{
		InputMessage msg;
		msg.variant = CursorPosMessage(std::move(xpos), std::move(ypos));
		PushBackInput(window, std::move(msg));
	}

	static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
	{
		InputMessage msg;
		msg.variant = MouseButtonMessage(std::move(button), std::move(action), std::move(mods));
		PushBackInput(window, std::move(msg));
	}

	struct InitData
	{
		char const* WindowTitle;
		int width; int height;
		vk::InstanceCreateInfo instanceCreateInfo;
		std::vector<const char*> deviceExtensions;
		ShaderData shaderData;
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
			vk::UniqueCommandBuffer m_CopyCommandBuffer;
			vk::UniqueCommandBuffer m_RenderCommandBuffer;
			vk::UniqueBuffer m_MatrixStagingBuffer;
			UniqueAllocation m_MatrixStagingMemory;
			vk::UniqueBuffer m_MatrixBuffer;
			UniqueAllocation m_MatrixMemory;
			size_t m_BufferCapacity = 0U;
			vk::UniqueDescriptorPool m_VertexLayoutDescriptorPool;
			vk::UniqueDescriptorSet m_VertexDescriptorSet;
			vk::UniqueFence m_RenderBufferExecuted;
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
		InitData m_InitData;
		UpdateData m_UpdateData;
		GLFWwindow* m_Window;
		bool m_ResizeNeeded = false;
		Clock::time_point m_StartupTimePoint;
		CircularQueue<InputMessage, 500> m_InputBuffer;
		Camera2D m_Camera;
		bool m_GameLoop = false;
		HMODULE m_VulkanLibrary;
		vk::UniqueInstance m_Instance;
		vk::PhysicalDevice m_PhysicalDevice;
		vk::DeviceSize m_UniformBufferOffsetAlignment;
		vk::DeviceSize m_MatrixBufferOffsetAlignment;
		uint32_t m_GraphicsQueueFamilyID;
		vk::UniqueDevice m_LogicalDevice;
		vk::Queue m_GraphicsQueue;
		vk::UniqueDebugReportCallbackEXT m_DebugBreakpointCallbackData;
		Allocator m_Allocator;
		vk::UniqueCommandPool m_CopyCommandPool;
		vk::UniqueCommandBuffer m_CopyCommandBuffer;
		vk::UniqueFence m_CopyCommandFence;
		std::vector<vk::UniqueImage> m_Images;
		std::vector<UniqueAllocation> m_ImageAllocations;
		std::vector<vk::UniqueImageView> m_ImageViews;
		std::vector<Texture2D> m_Textures;
		std::vector<Vertex> m_Vertices;
		vk::UniqueBuffer m_VertexBuffer;
		UniqueAllocation m_VertexMemory;
		vk::UniqueBuffer m_IndexBuffer;
		UniqueAllocation m_IndexMemory;
		bool m_SurfaceNeedsRecreation = false;
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

		void RenderSprite(
			uint32_t textureIndex,
			glm::mat4 transform,
			glm::vec4 color)
		{
			Sprite sprite;
			auto& supports = m_Supports[m_UpdateData.nextImage];
			glm::mat4 mvp =  m_UpdateData.vp * transform;

			memcpy((char*)m_UpdateData.mapped + m_UpdateData.copyOffset, &mvp, sizeof(glm::mat4));
			m_UpdateData.copyOffset += m_MatrixBufferOffsetAlignment;

			FragmentPushConstants pushRange;
			pushRange.textureID = textureIndex;
			pushRange.r = color.r;
			pushRange.g = color.g;
			pushRange.b = color.b;
			pushRange.a = color.a;

			supports.m_RenderCommandBuffer->pushConstants<FragmentPushConstants>(
				m_PipelineLayout.get(),
				vk::ShaderStageFlagBits::eFragment,
				0U,
				{ pushRange });

			uint32_t dynamicOffset = static_cast<uint32_t>(m_UpdateData.spriteIndex * m_MatrixBufferOffsetAlignment);

			// bind dynamic matrix uniform
			supports.m_RenderCommandBuffer->bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				m_PipelineLayout.get(),
				0U,
				{ supports.m_VertexDescriptorSet.get() },
				{ dynamicOffset });

			auto vertexOffset = textureIndex * IndicesPerQuad;
			// draw the sprite
			supports.m_RenderCommandBuffer->
				drawIndexed(IndicesPerQuad, 1U, 0U, vertexOffset, 0U);
			m_UpdateData.spriteIndex++;
		}

		void Run(LoopCallbacks callbacks)
		{
			m_GameLoop = true;
			std::thread gameLoop([&](){
				while(m_GameLoop)
				{
					if (m_ResizeNeeded)
					{
						auto size = GetWindowSize();
						resizeWindow(size);
						m_ResizeNeeded = false;
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
			});

			while (!glfwWindowShouldClose(m_Window))
			{
				//loop
				glfwWaitEvents();
			}
			m_GameLoop = false;
			gameLoop.join();
			m_LogicalDevice->waitIdle();
		}

		void debugprofiler()
		{
			size_t frameCount = 0;
			profiler::Describe<0>("Frame time");
			profiler::startTimer<0>();
				profiler::endTimer<0>();
				if (frameCount % 100 == 0)
				{
					auto frameDuration = profiler::getRollingAverage<0>(100);
					auto millisecondFrameDuration = std::chrono::duration_cast<std::chrono::microseconds>(frameDuration);
					auto frameDurationCount = millisecondFrameDuration.count();
					auto title = m_InitData.WindowTitle + std::string(": ") + helper::uitostr(size_t(frameDurationCount)) + std::string(" microseconds");
					glfwSetWindowTitle(m_Window, title.c_str());
				}
				frameCount++;
		}

		TextureIndex createTexture(const Bitmap & bitmap)
		{
			TextureIndex textureIndex = static_cast<uint32_t>(m_Textures.size());
			// create vke::Image2D
			auto& image2D = m_Images.emplace_back();
			auto& imageView = m_ImageViews.emplace_back();
			auto& imageAlloc = m_ImageAllocations.emplace_back();
			auto& texture = m_Textures.emplace_back();

			auto imageResult = CreateImageFromBitmap(bitmap);
			image2D = std::move(imageResult.image);
			imageAlloc = std::move(imageResult.allocation);

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

			// auto verticesPerTexture = 4U;

			// initialize the texture object
			texture.index = textureIndex;
			texture.width = bitmap.m_Width;
			texture.height = bitmap.m_Height;
			texture.size = bitmap.m_Size;

			auto stagingBufferResult = CreateBuffer(texture.size,
				vk::BufferUsageFlagBits::eTransferSrc,
				vk::MemoryPropertyFlagBits::eHostCoherent |
				vk::MemoryPropertyFlagBits::eHostVisible,
				true);
				
			// copy from host to staging buffer
			void* stagingBufferData = m_LogicalDevice->mapMemory(
				stagingBufferResult.allocation->memory,
				stagingBufferResult.allocation->offsetInDeviceMemory,
				stagingBufferResult.allocation->size);
			memcpy(stagingBufferData, bitmap.m_Data.data(), static_cast<size_t>(bitmap.m_Size));
			m_LogicalDevice->unmapMemory(stagingBufferResult.allocation->memory);

			// begin recording command buffer
			m_CopyCommandBuffer->begin(vk::CommandBufferBeginInfo(
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
			m_CopyCommandBuffer->pipelineBarrier(
				vk::PipelineStageFlagBits::eTopOfPipe,
				vk::PipelineStageFlagBits::eTransfer,
				vk::DependencyFlags(),
				{ },
				{ },
				{ memoryBarrier });

			// copy from host to device (to image)
			m_CopyCommandBuffer->copyBufferToImage(stagingBufferResult.buffer.get(), image2D.get(), vk::ImageLayout::eTransferDstOptimal,
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
			m_CopyCommandBuffer->pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eFragmentShader,
				vk::DependencyFlags(),
				{ },
				{ },
				{ memoryBarrier2 });

			m_CopyCommandBuffer->end();

			// create fence, submit command buffer
			auto imageLoadFence = m_LogicalDevice->createFenceUnique(vk::FenceCreateInfo());
			m_GraphicsQueue.submit(
				vk::SubmitInfo(0U, nullptr, nullptr,
					1U, &m_CopyCommandBuffer.get(),
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

		void init(InitData initData)
		{
			m_InitData = initData;
			auto glfwInitError = glfwInit();
			if (!glfwInitError)
			{
				std::runtime_error("Error initializing GLFW.");
				exit(glfwInitError);
			}
			m_Window = glfwCreateWindow(initData.width, initData.height, initData.WindowTitle, NULL, NULL);
			if (m_Window == NULL)
			{
				std::runtime_error("Error creating GLFW window.");
				exit(-1);
			}
			m_StartupTimePoint = Clock::now();
			glfwSetWindowUserPointer(m_Window, this);
			glfwSetWindowSizeCallback(m_Window, ResizeCallback);
			glfwSetKeyCallback(m_Window, KeyCallback);
			glfwSetCharCallback(m_Window, CharacterCallback);
			glfwSetCursorPosCallback(m_Window, CursorPositionCallback);
			glfwSetMouseButtonCallback(m_Window, MouseButtonCallback);
			m_Camera = Camera2D();
			m_Camera.setSize({static_cast<float>(initData.width), static_cast<float>(initData.height)});

			// load vulkan dll, entry point, and instance/global function pointers
			if (!LoadVulkanDLL())
			{
				std::runtime_error("Error loading vulkan DLL.");
				exit(-1);
			}
			LoadVulkanEntryPoint();
			LoadVulkanGlobalFunctions();
			m_Instance = vk::createInstanceUnique(initData.instanceCreateInfo);
			LoadVulkanInstanceFunctions();

			// select physical device
			auto devices = m_Instance->enumeratePhysicalDevices();
			m_PhysicalDevice = devices[0];

			m_UniformBufferOffsetAlignment = m_PhysicalDevice.getProperties().limits.minUniformBufferOffsetAlignment;
			auto offsetsForMatrix = sizeof(glm::mat4) / m_UniformBufferOffsetAlignment;
			if (offsetsForMatrix == 0)
			{
				offsetsForMatrix = 1;
			}
			m_MatrixBufferOffsetAlignment = offsetsForMatrix * m_UniformBufferOffsetAlignment;


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
						vk::DebugReportFlagBitsEXT::eError |
						vk::DebugReportFlagBitsEXT::eDebug |
						vk::DebugReportFlagBitsEXT::eInformation |
						vk::DebugReportFlagBitsEXT::eWarning,
						reinterpret_cast<PFN_vkDebugReportCallbackEXT>(&debugBreakCallback)
					));
			}

			// create the allocator
			m_Allocator = Allocator(m_PhysicalDevice, m_LogicalDevice.get());

			// create copy command objects
			auto copyPoolCreateInfo = vk::CommandPoolCreateInfo(
				vk::CommandPoolCreateFlagBits::eResetCommandBuffer |
				vk::CommandPoolCreateFlagBits::eTransient,
				m_GraphicsQueueFamilyID);
			m_CopyCommandPool = m_LogicalDevice->createCommandPoolUnique(copyPoolCreateInfo);
			auto copyCmdBufferInfo = vk::CommandBufferAllocateInfo(
				m_CopyCommandPool.get(),
				vk::CommandBufferLevel::ePrimary,
				1U);
			
			m_CopyCommandBuffer = std::move(
				m_LogicalDevice->allocateCommandBuffersUnique(copyCmdBufferInfo)[0]);
			m_CopyCommandFence = m_LogicalDevice->createFenceUnique(
				vk::FenceCreateInfo(/*vk::FenceCreateFlagBits::eSignaled*/));

			// allow client app to load images
			initData.imageLoadCallback(this);
			m_TextureCount = static_cast<uint32_t>(m_Images.size());

			CreateVertexBuffer();

			CreateIndexBuffer();

			// create surface
			CreateSurface(std::make_optional<vk::Extent2D>());

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
			m_FragmentDescriptorSetLayout = createFragmentSetLayout(m_LogicalDevice.get(), static_cast<uint32_t>(m_Textures.size()), m_Sampler.get());
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
				static_cast<uint32_t>(m_DescriptorSetLayouts.size()),
				m_DescriptorSetLayouts.data(),
				static_cast<uint32_t>(m_PushConstantRanges.size()),
				m_PushConstantRanges.data());
			m_PipelineLayout = m_LogicalDevice->createPipelineLayoutUnique(pipelineLayoutInfo);

			// init all the swapchain dependencies
			createSwapchainWithDependencies(m_SurfaceExtent);

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

	private:
		bool LoadVulkanDLL()
		{
			m_VulkanLibrary = LoadLibrary("vulkan-1.dll");

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

	private:
		struct BufferCreateResult
		{
			vk::UniqueBuffer buffer;
			UniqueAllocation allocation;
		};
		BufferCreateResult CreateBuffer(
			const vk::DeviceSize bufferSize, 
			const vk::BufferUsageFlags bufferUsage,
			const vk::MemoryPropertyFlags memoryFlags,
			const bool DedicatedAllocation)
		{
			BufferCreateResult result;
			auto bufferCreateInfo = vk::BufferCreateInfo(
				vk::BufferCreateFlags(),
				bufferSize,
				bufferUsage,
				vk::SharingMode::eExclusive,
				1U,
				&m_GraphicsQueueFamilyID);

			VkBuffer buffer = m_LogicalDevice->createBuffer(bufferCreateInfo);
			result.allocation = m_Allocator.AllocateForBuffer(DedicatedAllocation, buffer, memoryFlags);
			m_LogicalDevice->bindBufferMemory(buffer, 
				result.allocation->memory, 
				result.allocation->offsetInDeviceMemory);
			result.buffer = vk::UniqueBuffer(buffer, vk::BufferDeleter(m_LogicalDevice.get()));
			return result;
		}

		void CopyToBuffer(
			const vk::CommandBuffer commandBuffer,
			const vk::Buffer source, 
			const vk::Buffer destination, 
			const vk::DeviceSize size, 
			const vk::DeviceSize sourceOffset, 
			const vk::DeviceSize destinationOffset,
			const std::optional<vk::Fence> fence,
			const std::vector<vk::Semaphore> waitSemaphores = {},
			const std::vector<vk::Semaphore> signalSemaphores = {}
		)
		{
			// m_LogicalDevice->waitForFences({m_CopyCommandFence.get()}, vk::Bool32(true), std::numeric_limits<uint64_t>::max());
			commandBuffer.begin(vk::CommandBufferBeginInfo(
				vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

			commandBuffer.copyBuffer(source, destination, 
			{vk::BufferCopy(sourceOffset, destinationOffset, size)});

			commandBuffer.end();

			m_GraphicsQueue.submit( {vk::SubmitInfo(
				static_cast<uint32_t>(waitSemaphores.size()),
				waitSemaphores.data(),
				nullptr,
				1U,
				&commandBuffer,
				static_cast<uint32_t>(signalSemaphores.size()),
				signalSemaphores.data()) }, fence.value_or(vk::Fence()));
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

				auto newBufferSize = supports.m_BufferCapacity * m_MatrixBufferOffsetAlignment;

				// create staging buffer
				auto stagingBufferResult = CreateBuffer(newBufferSize,
					vk::BufferUsageFlagBits::eTransferSrc,
					vk::MemoryPropertyFlagBits::eHostCoherent |
					vk::MemoryPropertyFlagBits::eHostVisible,
					true);
					
				supports.m_MatrixStagingBuffer = std::move(stagingBufferResult.buffer);
				supports.m_MatrixStagingMemory = std::move(stagingBufferResult.allocation);

				// create matrix buffer
				auto matrixBufferResult = CreateBuffer(newBufferSize,
					vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer,
					vk::MemoryPropertyFlagBits::eDeviceLocal,
					true);
				supports.m_MatrixBuffer = std::move(matrixBufferResult.buffer);
				supports.m_MatrixMemory = std::move(matrixBufferResult.allocation);
			}

			m_UpdateData.mapped = m_LogicalDevice->mapMemory(
				supports.m_MatrixStagingMemory->memory, 
				supports.m_MatrixStagingMemory->offsetInDeviceMemory,
				supports.m_MatrixStagingMemory->size);

			m_UpdateData.copyOffset = 0;
			m_UpdateData.vp = m_Camera.getMatrix();

			m_LogicalDevice->waitForFences({ supports.m_RenderBufferExecuted.get() }, 
			vk::Bool32(true),
			std::numeric_limits<uint64_t>::max());
			m_LogicalDevice->resetFences({ supports.m_RenderBufferExecuted.get() });

			// TODO: benchmark whether staging buffer use is faster than alternatives
			auto descriptorBufferInfo = vk::DescriptorBufferInfo(
					supports.m_MatrixBuffer.get(),
					0U,
					m_MatrixBufferOffsetAlignment);
			m_LogicalDevice->updateDescriptorSets(
				{ 
					vk::WriteDescriptorSet(
						supports.m_VertexDescriptorSet.get(),
						0U,
						0U,
						1U,
						vk::DescriptorType::eUniformBufferDynamic,
						nullptr,
						&descriptorBufferInfo,
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
			supports.m_RenderCommandBuffer->reset(vk::CommandBufferResetFlags());

			// Render pass info
			auto renderPassInfo = vk::RenderPassBeginInfo(
				m_RenderPass.get(),
				m_Framebuffers[nextImage.value].get(),
				scissorRect,
				1U,
				&clearValue);

			// record the command buffer
			supports.m_RenderCommandBuffer->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
			supports.m_RenderCommandBuffer->setViewport(0U, { viewport });
			supports.m_RenderCommandBuffer->setScissor(0U, { scissorRect });
			supports.m_RenderCommandBuffer->beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);
			supports.m_RenderCommandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, m_Pipeline.get());
			auto vertexBuffer = m_VertexBuffer.get();
			supports.m_RenderCommandBuffer->bindVertexBuffers(
				0U,
				{ vertexBuffer }, 
				{ 0U });
			auto indexBuffer = m_IndexBuffer.get();
			supports.m_RenderCommandBuffer->bindIndexBuffer(
				indexBuffer, 
				0U, 
				vk::IndexType::eUint16);

			// bind sampler and images uniforms
			supports.m_RenderCommandBuffer->bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				m_PipelineLayout.get(),
				1U,
				{ m_FragmentDescriptorSet.get() },
				{});

			m_UpdateData.spriteIndex = 0;
			return true;
		}

		void EndRender()
		{
			auto& supports = m_Supports[m_UpdateData.nextImage];
			// copy matrices from staging buffer to gpu buffer
			m_LogicalDevice->unmapMemory(supports.m_MatrixStagingMemory->memory);

			CopyToBuffer(
				supports.m_CopyCommandBuffer.get(),
				supports.m_MatrixStagingBuffer.get(),
				supports.m_MatrixBuffer.get(),
				supports.m_MatrixMemory->size,
				0U,
				0U,
				std::optional<vk::Fence>(),
				{},
				{ supports.m_MatrixBufferStagingCompleteSemaphore.get() });

			// Finish recording draw command buffer
			supports.m_RenderCommandBuffer->endRenderPass();
			supports.m_RenderCommandBuffer->end();

			// Submit draw command buffer
			auto stageFlags = vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput);
			m_GraphicsQueue.submit(
				{
					vk::SubmitInfo(
						1U,
						&supports.m_MatrixBufferStagingCompleteSemaphore.get(),
						&stageFlags,
						1U,
						&supports.m_RenderCommandBuffer.get(),
						1U,
						&supports.m_ImageRenderCompleteSemaphore.get())
				}, supports.m_RenderBufferExecuted.get());

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
				m_SurfaceExtent = GetWindowSize();
				createSwapchainWithDependencies(m_SurfaceExtent);
			}
			else if (result != vk::Result::eSuccess)
			{
				throw std::runtime_error("failed to present swap chain image!");
			}
		}

		vk::Extent2D GetWindowSize()
		{
			int width = 0;
			int height = 0;
			glfwGetWindowSize(m_Window, &width, &height);
			return vk::Extent2D(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
		}

		struct ImageCreateResult
		{
			vk::UniqueImage image;
			UniqueAllocation allocation;
		};
		ImageCreateResult CreateImageFromBitmap(const Bitmap& bitmap)
		{
			ImageCreateResult result;
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


			result.image = m_LogicalDevice->createImageUnique(imageInfo);
			result.allocation = m_Allocator.AllocateForImage(true, result.image.get(), 
				vk::MemoryPropertyFlagBits::eDeviceLocal);
			m_LogicalDevice->bindImageMemory(
				result.image.get(),
				result.allocation->memory,
				result.allocation->offsetInDeviceMemory);
			
			return result;
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
					static_cast<uint32_t>(poolSizes.size()),
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
					static_cast<uint32_t>(poolSizes.size()),
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
					static_cast<uint32_t>(vertexLayoutBindings.size()),
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
					static_cast<uint32_t>(fragmentLayoutBindings.size()),
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
				static_cast<uint32_t>(dependencies.size()),
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
				static_cast<uint32_t>(m_FragmentSpecializations.size()),
				m_FragmentSpecializations.data(),
				static_cast<uint32_t>(sizeof(glm::uint32)),
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
				static_cast<uint32_t>(m_VertexBindings.size()),
				m_VertexBindings.data(),
				static_cast<uint32_t>(m_VertexAttributes.size()),
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
				static_cast<uint32_t>(m_DynamicStates.size()),
				m_DynamicStates.data());

			m_PipelineCreateInfo = vk::GraphicsPipelineCreateInfo(
				vk::PipelineCreateFlags(),
				static_cast<uint32_t>(m_PipelineShaderStageInfo.size()),
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
			auto vertSize = sizeof(Vertex);
			auto vertexBufferSize = vertSize * m_Vertices.size();
			auto vertexStagingResult = CreateBuffer(vertexBufferSize,
				vk::BufferUsageFlagBits::eTransferSrc,
				vk::MemoryPropertyFlagBits::eHostVisible |
				vk::MemoryPropertyFlagBits::eHostCoherent,
				true);
			
			auto vertexBufferResult = CreateBuffer(vertexBufferSize, 
				vk::BufferUsageFlagBits::eTransferDst |
				vk::BufferUsageFlagBits::eVertexBuffer,
				vk::MemoryPropertyFlagBits::eDeviceLocal,
				true);

			// copy data to vertex buffer
			void* vertexStagingData = m_LogicalDevice->mapMemory(
				vertexStagingResult.allocation->memory,
				vertexStagingResult.allocation->offsetInDeviceMemory,
				vertexStagingResult.allocation->size);
			
			memcpy(vertexStagingData, m_Vertices.data(), vertexBufferSize);
			m_LogicalDevice->unmapMemory(vertexStagingResult.allocation->memory);
			CopyToBuffer(
				m_CopyCommandBuffer.get(),
				vertexStagingResult.buffer.get(),
				vertexBufferResult.buffer.get(),
				vertexBufferSize,
				0U,
				0U,
				std::make_optional<vk::Fence>(m_CopyCommandFence.get()));
			
			// transfer ownership to VulkanApp
			m_VertexBuffer = std::move(vertexBufferResult.buffer);
			m_VertexMemory = std::move(vertexBufferResult.allocation);

			// wait for vertex buffer copy to finish
			m_LogicalDevice->waitForFences({ m_CopyCommandFence.get() }, 
				vk::Bool32(true), 
				std::numeric_limits<uint64_t>::max());
			m_LogicalDevice->resetFences({ m_CopyCommandFence.get() });
		}

		void CreateIndexBuffer()
		{
			//create index buffers
			auto indexBufferSize = sizeof(VertexIndex) * IndicesPerQuad;
			auto indexStagingResult = CreateBuffer(indexBufferSize,
				vk::BufferUsageFlagBits::eTransferSrc,
				vk::MemoryPropertyFlagBits::eHostVisible |
					vk::MemoryPropertyFlagBits::eHostCoherent,
					true);
			auto indexResult = CreateBuffer(indexBufferSize,
				vk::BufferUsageFlagBits::eTransferDst |
					vk::BufferUsageFlagBits::eIndexBuffer,
				vk::MemoryPropertyFlagBits::eDeviceLocal,
				true);

			// copy data to index buffer
			void* indexStagingData = m_LogicalDevice->mapMemory(
				indexStagingResult.allocation->memory,
				indexStagingResult.allocation->offsetInDeviceMemory,
				indexStagingResult.allocation->size);
			
			memcpy(indexStagingData, IndexArray.data(), indexBufferSize);
			m_LogicalDevice->unmapMemory(indexStagingResult.allocation->memory);	
			CopyToBuffer(
				m_CopyCommandBuffer.get(),
				indexStagingResult.buffer.get(),
				indexResult.buffer.get(),
				indexBufferSize,
				0U,
				0U,
				std::make_optional<vk::Fence>(m_CopyCommandFence.get()));
				
			// transfer ownership to VulkanApp
			m_IndexBuffer = std::move(indexResult.buffer);
			m_IndexMemory = std::move(indexResult.allocation);

					// wait for index buffer copy to finish
			m_LogicalDevice->waitForFences({ m_CopyCommandFence.get() }, 
				vk::Bool32(true), 
				std::numeric_limits<uint64_t>::max());
			m_LogicalDevice->resetFences({ m_CopyCommandFence.get() });
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

		void createSwapchainWithDependencies(vk::Extent2D size)
		{
			m_LogicalDevice->waitIdle();
			m_Swapchain.reset();
			CreateSurface(size);

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

		void resizeWindow(vk::Extent2D size)
		{
			m_Camera.setSize(glm::vec2(static_cast<float>(size.width), static_cast<float>(size.height)));
			createSwapchainWithDependencies(size);
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
			auto descriptorImageInfo = vk::DescriptorImageInfo(m_Sampler.get());
			m_LogicalDevice->updateDescriptorSets(
				{
					vk::WriteDescriptorSet(
						m_FragmentDescriptorSet.get(),
						0U,
						0U,
						1U,
						vk::DescriptorType::eSampler,
						&descriptorImageInfo)
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
						static_cast<uint32_t>(imageInfos.size()),
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
			auto commandBuffers = m_LogicalDevice->allocateCommandBuffersUnique(
				vk::CommandBufferAllocateInfo(
					support.m_CommandPool.get(),
					vk::CommandBufferLevel::ePrimary,
					2U));
			support.m_CopyCommandBuffer = std::move(commandBuffers[0]);
			support.m_RenderCommandBuffer = std::move(commandBuffers[1]);
			support.m_VertexLayoutDescriptorPool = createVertexDescriptorPool();
			support.m_BufferCapacity = 0U;
			support.m_RenderBufferExecuted = m_LogicalDevice->createFenceUnique(
				vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
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

		void CreateSurface(std::optional<vk::Extent2D> extent)
		{
			VkSurfaceKHR surface;
			glfwCreateWindowSurface(m_Instance.get(), m_Window, nullptr, &surface);
			m_Surface = vk::UniqueSurfaceKHR(surface, vk::SurfaceKHRDeleter(m_Instance.get()));
			
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
				if (extent)
					m_SurfaceExtent = extent.value();
				else
					m_SurfaceExtent = GetWindowSize();
			}
			else
			{
				m_SurfaceExtent = m_SurfaceCapabilities.currentExtent;
			}
		}

		// Window resize callback
		static void ResizeCallback(GLFWwindow* window, int width, int height)
		{
			auto ptr = glfwGetWindowUserPointer(window);
			((VulkanApp*)ptr)->m_ResizeNeeded = true;
		};

	};

	static void PushBackInput(GLFWwindow* window, InputMessage&& msg)
	{
		auto& app = *GetUserPointer(window);
		msg.time = std::chrono::duration_cast<InputTime>(Clock::now() - app.m_StartupTimePoint);
		app.m_InputBuffer.pushLast(std::move(msg));
	}

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