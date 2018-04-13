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
#include "ApplicationState.hpp"
#include "InitState.hpp"
#include "InstanceState.hpp"
#include "Input.hpp"
#include "DeviceState.hpp"
#include "SurfaceState.hpp"
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
// #include "profiler.hpp"
#include "mymath.hpp"
#include "CircularQueue.hpp"
#include "TimeHelper.hpp"
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
#include <string>
#include <vector>

#undef max
#undef min

namespace vka
{
	using json = nlohmann::json;
	using HashType = entt::HashedString::hash_type;
	
	static void ResizeCallback(GLFWwindow* window, int width, int height);

	struct VulkanApp
	{
		ApplicationState m_AppState;
		InputState m_InputState;
		InstanceState m_InstanceState;
		DeviceState m_DeviceState;
		InitState m_InitState;
		SurfaceState m_SurfaceState;
		ShaderState m_ShaderState;
		RenderState m_RenderState;
		PipelineState m_PipelineState;

		void RenderSpriteInstance(
			uint64_t spriteIndex,
			glm::mat4 transform,
			glm::vec4 color)
		{
			auto sprite = m_RenderState.sprites.at(spriteIndex);
			auto& supports = m_RenderState.supports[m_RenderState.nextImage];
			glm::mat4 mvp =  m_RenderState.camera.getMatrix() * transform;

			FragmentPushConstants pushRange;
			pushRange.imageOffset = sprite.imageOffset;
			pushRange.color = color;
			pushRange.mvp = mvp;

			supports.renderCommandBuffer->pushConstants<FragmentPushConstants>(
				m_PipelineState.pipelineLayout.get(),
				vk::ShaderStageFlagBits::eFragment,
				0U,
				{ pushRange });

			// draw the sprite
			supports.renderCommandBuffer->
				draw(VerticesPerQuad, 1U, sprite.vertexOffset, 0U);
		}

		void GameThread()
		{
			while(m_AppState.gameLoop)
			{
				auto currentTime = NowMilliseconds();
				std::unique_lock<std::mutex> resizeLock(m_SurfaceState.recreateSurfaceMutex);
				m_SurfaceState.recreateSurfaceCondition.wait(resizeLock, [this](){ return !m_SurfaceState.surfaceNeedsRecreation; });
				resizeLock.unlock();
				// Update simulation to be in sync with actual time
				size_t spriteCount = 0;
				while (currentTime - m_AppState.currentSimulationTime > UpdateDuration)
				{
					m_AppState.currentSimulationTime += UpdateDuration;

					spriteCount = m_InitState.updateCallback(m_AppState.currentSimulationTime);
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
				m_InitState.renderCallback();
				EndRender();
			}
		}

		void Run()
		{
			m_AppState.gameLoop = true;
			m_AppState.startupTimePoint = NowMilliseconds();
			m_AppState.currentSimulationTime = m_AppState.startupTimePoint;
			m_SurfaceState.surfaceNeedsRecreation = false;

			std::thread gameLoopThread(&VulkanApp::GameThread, this);

			// Event loop
			while (!glfwWindowShouldClose(m_AppState.window))
			{
				// sync threads if resize needed
				while (m_SurfaceState.surfaceNeedsRecreation)
				{
					auto size = GetWindowSize(m_AppState);
					resizeWindow(size);
					m_SurfaceState.surfaceNeedsRecreation = false;
					m_SurfaceState.recreateSurfaceCondition.notify_all();
				}
				glfwWaitEvents();
			}
			m_AppState.gameLoop = false;
			gameLoopThread.join();
			m_DeviceState.logicalDevice->waitIdle();
		}

		static json LoadSpriteSheet(char* const path)
		{
			std::ifstream i(path);
			json j;
			i >> j;

			return j;
		}

		void LoadImage2D(const HashType imageID, const Bitmap& bitmap)
		{
			m_RenderState.images[imageID] = CreateImage2D(
				m_DeviceState.logicalDevice.get(), 
				m_InitState.copyCommandBuffer.get(), 
				m_DeviceState.allocator,
				bitmap,
				m_DeviceState.graphicsQueueFamilyID,
				m_DeviceState.graphicsQueue);
		}

		void CreateSprite(const HashType imageID, const HashType spriteName, const Quad quad)
		{
			Sprite sprite;
			sprite.imageID = imageID;
			sprite.quad = quad;
			m_RenderState.sprites[spriteName] = sprite;
		}

		void Init(
			std::string windowTitle,
			int width,
			int height,
			vk::InstanceCreateInfo instanceCreateInfo,
			std::vector<const char*> deviceExtensions,
			std::string vertexShaderPath,
			std::string fragmentShaderPath,
			ImageLoadFuncPtr imageLoadCallback,
			UpdateFuncPtr updateCallback,
			RenderFuncPtr renderCallback
		)
		{
			m_InitState.updateCallback = updateCallback;
			m_InitState.renderCallback = renderCallback;
			m_InitState.imageLoadCallback = imageLoadCallback;
			m_InitState.windowTitle = windowTitle;
			m_InitState.width = width;
			m_InitState.height = height;
			m_InitState.vertexShaderPath = vertexShaderPath;
			m_InitState.fragmentShaderPath = fragmentShaderPath;
			m_InitState.deviceExtensions = deviceExtensions;
			auto glfwInitError = glfwInit();
			if (!glfwInitError)
			{
				std::runtime_error("Error initializing GLFW.");
				exit(glfwInitError);
			}
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
			m_AppState.window = glfwCreateWindow(width, height, windowTitle.c_str(), NULL, NULL);
			if (m_AppState.window == NULL)
			{
				std::runtime_error("Error creating GLFW window.");
				exit(-1);
			}
			glfwSetWindowUserPointer(m_AppState.window, this);
			glfwSetWindowSizeCallback(m_AppState.window, ResizeCallback);
			glfwSetKeyCallback(m_AppState.window, KeyCallback);
			glfwSetCharCallback(m_AppState.window, CharacterCallback);
			glfwSetCursorPosCallback(m_AppState.window, CursorPositionCallback);
			glfwSetMouseButtonCallback(m_AppState.window, MouseButtonCallback);
			m_RenderState.camera.setSize({static_cast<float>(width), static_cast<float>(height)});

			// load vulkan dll, entry point, and instance/global function pointers
			if (!LoadVulkanDLL(m_InstanceState))
			{
				std::runtime_error("Error loading vulkan DLL.");
				exit(-1);
			}

			LoadVulkanEntryPoint(m_InstanceState);

			LoadVulkanGlobalFunctions();

			CreateInstance(m_InstanceState, instanceCreateInfo);

			LoadVulkanInstanceFunctions(m_InstanceState);

			InitPhysicalDevice(m_DeviceState, m_InstanceState.instance.get());

			SelectGraphicsQueue(m_DeviceState);

			CreateLogicalDevice(m_DeviceState, deviceExtensions);
			
			LoadVulkanDeviceFunctions(m_DeviceState);

			GetGraphicsQueue(m_DeviceState);

			CreateAllocator(m_DeviceState);

			InitCopyStructures(m_InitState, m_DeviceState);

			// allow client app to load images
			m_InitState.imageLoadCallback();

			FinalizeImageOrder(m_RenderState);

			FinalizeSpriteOrder(m_RenderState);

			CreateVertexBuffer(
				m_RenderState,
				m_DeviceState,
				m_InitState);

			CreateSampler(m_RenderState, m_DeviceState);

			CreateShaderModules(m_ShaderState, m_DeviceState, m_InitState);
			CreateFragmentSetLayout(m_ShaderState, m_DeviceState, m_RenderState);
			SetupPushConstants(m_ShaderState);
			CreateFragmentDescriptorPool(m_ShaderState, 
				m_DeviceState, 
				m_RenderState);
			CreateAndUpdateFragmentDescriptorSet(m_ShaderState, m_DeviceState, m_RenderState);

			CreateSurface(
				m_SurfaceState, 
				m_AppState, 
				m_InstanceState, 
				m_DeviceState, 
				GetWindowSize(m_AppState));
			SelectPresentMode(m_SurfaceState);
			CheckForPresentationSupport(m_DeviceState, m_SurfaceState);

			InitializeSupportStructs(m_RenderState, m_DeviceState, m_ShaderState);
			CreateRenderPass(m_RenderState, m_DeviceState, m_SurfaceState);
			CreateSwapchainWithDependencies(m_RenderState, m_DeviceState, m_SurfaceState);

			CreatePipelineLayout(m_PipelineState, m_DeviceState, m_ShaderState);
			CreatePipeline(m_PipelineState, m_DeviceState, m_RenderState, m_ShaderState);
		}

	private:
		auto acquireImage()
		{
			auto renderFinishedFence = m_DeviceState.logicalDevice->createFence(
				vk::FenceCreateInfo(vk::FenceCreateFlags()));
			auto nextImage = m_DeviceState.logicalDevice->acquireNextImageKHR(
				m_RenderState.swapchain.get(), 
				std::numeric_limits<uint64_t>::max(), 
				vk::Semaphore(),
				renderFinishedFence);
			m_RenderState.supports[nextImage.value].imagePresentCompleteFence = 
				vk::UniqueFence(renderFinishedFence, vk::FenceDeleter(m_DeviceState.logicalDevice.get()));
			return nextImage;
		}

		// TODO: is spritecount still needed?
		bool BeginRender(size_t spriteCount)
		{
			// try to acquire an image from the swapchain
			auto nextImage = acquireImage();
			if (nextImage.result != vk::Result::eSuccess)
			{
				return false;
			}
			m_RenderState.nextImage = nextImage.value;

			auto& supports = m_RenderState.supports[nextImage.value];

			// Sync host to device for the next image
			// TODO: determine if both fences are necessary
			m_DeviceState.logicalDevice->waitForFences(
				{ 
					supports.imagePresentCompleteFence.get(),
					supports.renderBufferExecuted.get() 
				}, 
				static_cast<vk::Bool32>(true), 
				std::numeric_limits<uint64_t>::max());
			m_DeviceState.logicalDevice->resetFences({ supports.renderBufferExecuted.get() });

			// Dynamic viewport info
			auto viewport = vk::Viewport(
				0.f,
				0.f,
				static_cast<float>(m_SurfaceState.surfaceExtent.width),
				static_cast<float>(m_SurfaceState.surfaceExtent.height),
				0.f,
				1.f);

			// Dynamic scissor info
			auto scissorRect = vk::Rect2D(
				vk::Offset2D(0, 0),
				m_SurfaceState.surfaceExtent);

			// clear values for the color and depth attachments
			auto clearValue = vk::ClearValue(vk::ClearColorValue(std::array<float, 4>{0.f, 0.f, 0.f, 0.f}));

			// reset command buffer
			supports.renderCommandBuffer->reset(vk::CommandBufferResetFlags());

			// Render pass info
			auto renderPassInfo = vk::RenderPassBeginInfo(
				m_RenderState.renderPass.get(),
				m_RenderState.framebuffers[nextImage.value].get(),
				scissorRect,
				1U,
				&clearValue);

			// record the command buffer
			supports.renderCommandBuffer->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
			supports.renderCommandBuffer->setViewport(0U, { viewport });
			supports.renderCommandBuffer->setScissor(0U, { scissorRect });
			supports.renderCommandBuffer->beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);
			supports.renderCommandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, m_PipelineState.pipeline.get());
			supports.renderCommandBuffer->bindVertexBuffers(
				0U,
				{ m_RenderState.vertexBuffer.buffer.get() }, 
				{ 0U });

			// bind sampler and images uniforms
			supports.renderCommandBuffer->bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				m_PipelineState.pipelineLayout.get(),
				0U,
				{ m_ShaderState.fragmentDescriptorSet.get() },
				{});

			return true;
		}

		void EndRender()
		{
			auto& supports = m_RenderState.supports[m_RenderState.nextImage];

			// Finish recording draw command buffer
			supports.renderCommandBuffer->endRenderPass();
			supports.renderCommandBuffer->end();

			// Submit draw command buffer
			auto stageFlags = vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput);
			m_DeviceState.graphicsQueue.submit(
				{
					vk::SubmitInfo(
						0U,
						nullptr,
						&stageFlags,
						1U,
						&supports.renderCommandBuffer.get(),
						1U,
						&supports.imageRenderCompleteSemaphore.get())
				}, supports.renderBufferExecuted.get());

			std::scoped_lock<std::mutex> resizeLock(m_SurfaceState.recreateSurfaceMutex);
			// Present image
			auto presentInfo = vk::PresentInfoKHR(
					1U,
					&supports.imageRenderCompleteSemaphore.get(),
					1U,
					&m_RenderState.swapchain.get(),
					&m_RenderState.nextImage);
			auto presentResult = vkQueuePresentKHR(
				m_DeviceState.graphicsQueue.operator VkQueue(), 
				&presentInfo.operator const VkPresentInfoKHR &());
		}

		vk::Extent2D GetWindowSize(const ApplicationState& appState)
		{
			int width = 0;
			int height = 0;
			glfwGetWindowSize(appState.window, &width, &height);
			return vk::Extent2D(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
		}

		void resizeWindow(vk::Extent2D size)
		{
			std::scoped_lock<std::mutex> resizeLock(m_SurfaceState.recreateSurfaceMutex);
			m_RenderState.camera.setSize(glm::vec2(static_cast<float>(size.width), static_cast<float>(size.height)));
			ResetSwapChain(m_RenderState, m_DeviceState);
			CreateSurface(m_SurfaceState, m_AppState, m_InstanceState, m_DeviceState, size);
			CreateSwapchainWithDependencies(m_RenderState, m_DeviceState, m_SurfaceState);
		}
	};

	static VulkanApp* GetUserPointer(GLFWwindow* window)
	{
		return reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
	}

	// Window resize callback
	static void ResizeCallback(GLFWwindow* window, int width, int height)
	{
		GetUserPointer(window)->m_SurfaceState.surfaceNeedsRecreation = true;
	};

	static void PushBackInput(GLFWwindow* window, InputMessage&& msg)
	{
		auto& app = *GetUserPointer(window);
		msg.time = NowMilliseconds();
		app.m_InputState.inputBuffer.pushLast(std::move(msg));
	}

	static void SetCursorPosition(GLFWwindow* window, double x, double y)
	{
		auto& app = *GetUserPointer(window);
		app.m_InputState.cursorX = x;
		app.m_InputState.cursorY = y;
	}

	// class Text
	// {
	// public:
	// 	using FontID = size_t;
	// 	using FontSize = size_t;
	// 	using GlyphID = FT_UInt;
	// 	using CharCode = FT_ULong;

	// 	struct FT_LibraryDeleter
	// 	{
	// 		using pointer = FT_Library;
	// 		void operator()(FT_Library library)
	// 		{
	// 			FT_Done_FreeType(library);
	// 		}
	// 	};
	// 	using UniqueFTLibrary = std::unique_ptr<FT_Library, FT_LibraryDeleter>;

	// 	struct FT_GlyphDeleter
	// 	{
	// 		using pointer = FT_Glyph;
	// 		void operator()(FT_Glyph glyph) noexcept
	// 		{
	// 			FT_Done_Glyph(glyph);
	// 		}
	// 	};
	// 	using UniqueFTGlyph = std::unique_ptr<FT_Glyph, FT_GlyphDeleter>;

	// 	struct GlyphData
	// 	{
	// 		UniqueFTGlyph glyph;
	// 		entt::HashedString textureIndex;
	// 	};

	// 	using GlyphMap = std::map<CharCode, GlyphData>;

	// 	struct SizeData
	// 	{
	// 		GlyphMap glyphMap;
	// 		FT_F26Dot6 spaceAdvance;
	// 	};

	// 	struct FontData
	// 	{
	// 		FT_Face face;
	// 		std::string path;
	// 		std::map<FontSize, SizeData> glyphsBySize;
	// 	};

	// 	struct TextGroup
	// 	{
	// 		std::vector<Entity> characters;
	// 	};

	// 	struct InitInfo
	// 	{
	// 		Text::FontID fontID;
	// 		Text::FontSize fontSize;
	// 		std::string text;
	// 		glm::vec4 textColor;
	// 		int baseline_x;
	// 		int baseline_y;
	// 		float depth;
	// 	};

	// 	UniqueFTLibrary m_FreeTypeLibrary;
	// 	std::map<FontID, FontData> m_FontMaps;
	// 	VulkanApp* m_VulkanApp = nullptr;

	// 	Text() = default;
	// 	Text(VulkanApp* app);
	// 	Text::SizeData & getSizeData(Text::FontID fontID, Text::FontSize size);
	// 	Bitmap getFullBitmap(FT_Glyph glyph);
	// 	std::optional<FT_Glyph> getGlyph(Text::FontID fontID, Text::FontSize size, Text::CharCode charcode);
	// 	auto getAdvance(Text::FontID fontID, Text::FontSize size, Text::CharCode left);
	// 	auto getAdvance(Text::FontID fontID, Text::FontSize size, Text::CharCode left, Text::CharCode right);
	// 	void LoadFont(Text::FontID fontID, Text::FontSize fontSize, uint32_t DPI, const char * fontPath);
	// 	std::vector<Sprite> createTextGroup(const Text::InitInfo & initInfo);
	// };

}