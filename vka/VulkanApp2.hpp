#pragma once

#undef max
#undef min
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "VulkanFunctions.hpp"
#include "vulkan/vulkan.hpp"
#include "GLFW/glfw3.h"
#include "fileIO.h"
#include "InstanceState.hpp"
#include "DeviceState.hpp"
#include "SurfaceState.hpp"
#include "ShaderState.hpp"
#include "PipelineState.hpp"
#include "RenderState.hpp"
#include "Image2D.hpp"
#include "Bitmap.hpp"
#include "Sprite.hpp"
#include "Camera.hpp"
#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include "Allocator.hpp"
#include "profiler.hpp"
#include "mymath.hpp"
#include "CircularQueue.hpp"
#include "TimeHelper.hpp"
#include "Input.hpp"
#include "nlohmann/json.hpp"
#include "Vertex.hpp"

#include <iostream>
#include <map>
#include <functional>
#include <limits>
#include <thread>
#include <variant>
#include <chrono>
#include <ratio>

#undef max
#undef min

namespace vka
{
	using SpriteCount = SpriteIndex;
	using json = nlohmann::json;
	using UpdateFuncPtr = std::function<SpriteCount(TimePoint_ms)>;
	using RenderFuncPtr = std::function<void()>;
	
	static void ResizeCallback(GLFWwindow* window, int width, int height);

	struct VulkanApp
	{
		entt::ResourceCache<Image2D> m_Images;
		entt::ResourceCache<Sprite> m_Sprites;
		
		GLFWwindow* m_Window;
		TimePoint_ms m_StartupTimePoint;
		TimePoint_ms m_CurrentSimulationTime;
		bool m_GameLoop = false;

		InitState m_InitState;
		InstanceState m_InstanceState;
		DeviceState m_DeviceState;
		SurfaceState m_SurfaceState;
		ShaderState m_ShaderState;
		RenderState m_RenderState;
		PipelineState m_PipelineState;

		void RenderSprite(
			uint32_t textureIndex,
			glm::mat4 transform,
			glm::vec4 color)
		{
			Sprite sprite;
			auto& supports = m_Supports[m_RenderState.nextImage];
			glm::mat4 mvp =  m_RenderState.vp * transform;

			memcpy((char*)m_RenderState.mapped + m_RenderState.copyOffset, &mvp, sizeof(glm::mat4));
			m_RenderState.copyOffset += m_MatrixBufferOffsetAlignment;

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

			uint32_t dynamicOffset = static_cast<uint32_t>(m_RenderState.spriteIndex * m_MatrixBufferOffsetAlignment);

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
			m_RenderState.spriteIndex++;
		}

		void GameThread()
		{
			while(m_GameLoop)
			{
				auto currentTime = NowMilliseconds();
				std::unique_lock<std::mutex> resizeLock(m_RecreateSurfaceMutex);
				m_RecreateSurfaceCondition.wait(resizeLock, [this](){ return !m_ResizeNeeded; });
				resizeLock.unlock();
				// Update simulation to be in sync with actual time
				SpriteCount spriteCount = 0;
				while (currentTime - m_CurrentSimulationTime > UpdateDuration)
				{
					m_CurrentSimulationTime += UpdateDuration;

					spriteCount = m_LoopCallbacks.UpdateCallback(m_CurrentSimulationTime);
				}
				if (!spriteCount)
				{
					continue;
				}
				// render a frame
				if (!BeginRender(spriteCount))
				{
					// TODO: probably need to handle some specific errors here
					continue;
				}
				m_LoopCallbacks.RenderCallback();
				EndRender();
			}
		}

		void Run(UpdateFuncPtr updateCallback, RenderFuncPtr renderCallback)
		{
			m_UpdateCallback = updateCallback;
			m_RenderCallback = renderCallback;
			m_GameLoop = true;
			m_StartupTimePoint = NowMilliseconds();
			m_CurrentSimulationTime = m_StartupTimePoint;

			std::thread gameLoop(&VulkanApp::GameThread, this);

			// Event loop
			while (!glfwWindowShouldClose(m_Window))
			{
				// join threads if resize needed, then restart game thread
				while (m_ResizeNeeded)
				{
					auto size = GetWindowSize();
					resizeWindow(size);
					m_ResizeNeeded = false;
					m_RecreateSurfaceCondition.notify_all();
				}
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
					auto title = m_InitState.WindowTitle + std::string(": ") + helper::uitostr(size_t(frameDurationCount)) + std::string(" microseconds");
					glfwSetWindowTitle(m_Window, title.c_str());
				}
				frameCount++;
		}

		auto readSpriteSheet(const std::string& path)
		{
			std::ifstream i(path);
			json j;
			i >> j;

			return j["frames"];
		}

		Texture2D createImage2D(const Bitmap& bitmap)
		{
			ImageIndex textureIndex = static_cast<ImageIndex>(m_Images.size());
			auto& image2D = m_Images.emplace_back();
			auto& imageView = m_ImageViews.emplace_back();
			auto& imageAlloc = m_ImageAllocations.emplace_back();

			auto& image2D = m_Images.emplace_back();
			image2D.index = textureIndex;
			image2D.width = bitmap.m_Width;
			image2D.height = bitmap.m_Height;
			image2D.size = bitmap.m_Size;

			auto imageResult = CreateImageFromBitmap(bitmap);
			image2D = std::move(imageResult.image);
			imageAlloc = std::move(imageResult.allocation);

			auto stagingBufferResult = CreateBuffer(image2D.size,
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
		}

		SpriteSheet createSpriteMapTexture(const Bitmap& bitmap, const std::string& path)
		{
			auto& frames = readSpriteSheet(path);
						
			for (auto& frame : frames)
			{
				float left   = 	frame["frame"]["x"];
				float top 	 = 	frame["frame"]["y"];
				float width = frame["frame"]["w"];
				float height = frame["frame"]["h"];
				float right  = 	left + width;
				float bottom = 	top + height;

				glm::vec2 LeftTopPos, LeftBottomPos, RightBottomPos, RightTopPos;
				LeftTopPos     = { left,		top };
				LeftBottomPos  = { left,		bottom };
				RightBottomPos = { right,		bottom };
				RightTopPos    = { right,		top };

				float leftUV = left / width;
				float rightUV = right / width;
				float topUV = top / height;
				float bottomUV = bottom / height;

				glm::vec2 LeftTopUV, LeftBottomUV, RightBottomUV, RightTopUV;
				LeftTopUV     = { leftUV, topUV };
				LeftBottomUV  = { leftUV, bottomUV };
				RightBottomUV = { rightUV, bottomUV };
				RightTopUV    = { rightUV, topUV };

				Vertex LeftTop, LeftBottom, RightBottom, RightTop;
				LeftTop 	= { LeftTopPos,		LeftTopUV 		};
				LeftBottom  = { LeftBottomPos,	LeftBottomUV 	};
				RightBottom = { RightBottomPos,	RightBottomUV 	};
				RightTop 	= { RightTopPos,	RightTopUV 		};

				// push back vertices
				m_Vertices.push_back(LeftTop);
				m_Vertices.push_back(LeftBottom);
				m_Vertices.push_back(RightBottom);
				m_Vertices.push_back(RightTop);
			}

			// initialize the image2D object
			auto image2D = createImage2D(bitmap);
			auto sheet = SpriteSheet();

			sheet.textureIndex = image2D.index;
			return sheet;
		}

		ImageIndex createImage2D(const Bitmap& bitmap)
		{
			ImageIndex textureIndex = static_cast<ImageIndex>(m_Images.size());
			// create vke::Image2D
			auto& image2D = m_Images.emplace_back();
			auto& imageView = m_ImageViews.emplace_back();
			auto& imageAlloc = m_ImageAllocations.emplace_back();
			auto& image2D = m_Images.emplace_back();

			auto imageResult = CreateImageFromBitmap(bitmap);
			image2D = std::move(imageResult.image);
			imageAlloc = std::move(imageResult.allocation);

			float left = 	static_cast<float>(bitmap.m_Left);
			float top = 	static_cast<float>(bitmap.m_Top);
			float width = 	static_cast<float>(bitmap.m_Width);
			float height = 	static_cast<float>(bitmap.m_Height);
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
			LeftTop 	= { LeftTopPos,		LeftTopUV 		};
			LeftBottom  = { LeftBottomPos,	LeftBottomUV 	};
			RightBottom = { RightBottomPos,	RightBottomUV 	};
			RightTop 	= { RightTopPos,	RightTopUV 		};

			// push back vertices
			m_Vertices.push_back(LeftTop);
			m_Vertices.push_back(LeftBottom);
			m_Vertices.push_back(RightBottom);
			m_Vertices.push_back(RightTop);

			// auto verticesPerTexture = 4U;

			// initialize the image2D object
			image2D.index = textureIndex;
			image2D.width = bitmap.m_Width;
			image2D.height = bitmap.m_Height;
			image2D.size = bitmap.m_Size;

			auto stagingBufferResult = CreateBuffer(image2D.size,
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

		void init(InitState initData)
		{
			m_InitState = initData;
			auto glfwInitError = glfwInit();
			if (!glfwInitError)
			{
				std::runtime_error("Error initializing GLFW.");
				exit(glfwInitError);
			}
			glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
			m_Window = glfwCreateWindow(initData.width, initData.height, initData.WindowTitle.c_str(), NULL, NULL);
			if (m_Window == NULL)
			{
				std::runtime_error("Error creating GLFW window.");
				exit(-1);
			}
			glfwSetWindowUserPointer(m_Window, this);
			glfwSetWindowSizeCallback(m_Window, ResizeCallback);
			glfwSetKeyCallback(m_Window, KeyCallback);
			glfwSetCharCallback(m_Window, CharacterCallback);
			glfwSetCursorPosCallback(m_Window, CursorPositionCallback);
			glfwSetMouseButtonCallback(m_Window, MouseButtonCallback);
			m_RenderState.camera.setSize({static_cast<float>(initData.width), static_cast<float>(initData.height)});

			// load vulkan dll, entry point, and instance/global function pointers
			if (!LoadVulkanDLL(m_InstanceState))
			{
				std::runtime_error("Error loading vulkan DLL.");
				exit(-1);
			}

			LoadVulkanEntryPoint(m_InstanceState);

			LoadVulkanGlobalFunctions();

			CreateInstance(m_InstanceState, initData.instanceCreateInfo);

			LoadVulkanInstanceFunctions(m_InstanceState);

			InitPhysicalDevice(m_DeviceState, m_Instance.get());

			SelectGraphicsQueue(m_DeviceState);

			CreateLogicalDevice(m_DeviceState, initData.deviceExtensions);
			
			LoadVulkanDeviceFunctions(m_DeviceState);

			GetGraphicsQueue(m_DeviceState);

			CreateAllocator(m_DeviceState);

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
			m_FragmentDescriptorSetLayout = createFragmentSetLayout(m_LogicalDevice.get(), static_cast<uint32_t>(m_Images.size()), m_Sampler.get());
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
			m_RenderState.nextImage = nextImage.value;

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

			m_RenderState.mapped = m_LogicalDevice->mapMemory(
				supports.m_MatrixStagingMemory->memory, 
				supports.m_MatrixStagingMemory->offsetInDeviceMemory,
				supports.m_MatrixStagingMemory->size);

			m_RenderState.copyOffset = 0;
			m_RenderState.vp = m_Camera.getMatrix();

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

			m_RenderState.spriteIndex = 0;
			return true;
		}

		void EndRender()
		{
			auto& supports = m_Supports[m_RenderState.nextImage];
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

			std::scoped_lock<std::mutex> resizeLock(m_RecreateSurfaceMutex);
			// Present image
			auto presentInfo = vk::PresentInfoKHR(
					1U,
					&supports.m_ImageRenderCompleteSemaphore.get(),
					1U,
					&m_Swapchain.get(),
					&m_RenderState.nextImage);
			auto presentResult = vkQueuePresentKHR(m_GraphicsQueue.operator VkQueue(), &presentInfo.operator const VkPresentInfoKHR &());
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
				// Fragment: image2D uniform buffer (array)
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
			std::scoped_lock<std::mutex> resizeLock(m_RecreateSurfaceMutex);
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
	};

	static VulkanApp* GetUserPointer(GLFWwindow* window)
	{
		return reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
	}

	// Window resize callback
	static void ResizeCallback(GLFWwindow* window, int width, int height)
	{
		GetUserPointer(window)->m_ResizeNeeded = true;
	};

	static void PushBackInput(GLFWwindow* window, InputMessage&& msg)
	{
		auto& app = *GetUserPointer(window);
		msg.time = NowMilliseconds();
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
			ImageIndex textureIndex;
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