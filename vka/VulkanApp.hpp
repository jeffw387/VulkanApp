#pragma once

#undef max
#undef min
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "VulkanFunctionLoader.hpp"
#include "windows.h"
#include "GLFW/glfw3.h"
#include "fileIO.hpp"
#include "Input.hpp"
#include "Instance.hpp"
#include "Device.hpp"
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
#include <iostream>
#include <mutex>
#include <optional>
#include <algorithm>

#undef max
#undef min

#ifdef NO_VALIDATION
constexpr bool ReleaseMode = true;
#endif

namespace vka
{
	using json = nlohmann::json;
	using HashType = entt::HashedString::hash_type;
	
	static void ResizeCallback(GLFWwindow* window, int width, int height);



	struct VulkanApp
	{
		GLFWwindow* window;
        TimePoint_ms startupTimePoint;
        TimePoint_ms currentSimulationTime;
        bool gameLoop = true;
        std::mutex loopStateMutex;
        LibraryHandle VulkanLibrary;
		Camera2D camera;

		VkCommandPool copyCommandPool;
		VkCommandBuffer copyCommandBuffer;
		VkFence copyFence;
		std::optional<Instance> instance;
		std::optional<Device> device;

		InputState m_InputState;

		void Run(std::string vulkanInitJsonPath)
		{
			json vulkanInitData = json::parse(fileIO::readFile(vulkanInitJsonPath));
			VulkanLibrary = LoadVulkanLibrary();
			LoadExportedEntryPoints(VulkanLibrary);
			LoadGlobalLevelEntryPoints();

			auto glfwInitSuccess = glfwInit();
			if (!glfwInitSuccess)
			{
				std::runtime_error("Error initializing GLFW.");
				exit(1);
			}

			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
			int width = vulkanInitData["DefaultWindowSize"]["Width"];
			int height = vulkanInitData[["DefaultWindowSize"]["Height"];
			std::string appName = vulkanInitData["ApplicationName"];
			window = glfwCreateWindow(width, height, appName.c_str(), NULL, NULL);
			if (window == NULL)
			{
				std::runtime_error("Error creating GLFW window.");
				exit(1);
			}
			glfwSetWindowUserPointer(window, this);
			glfwSetFramebufferSizeCallback(window, ResizeCallback);
			glfwSetKeyCallback(window, KeyCallback);
			glfwSetCharCallback(window, CharacterCallback);
			glfwSetCursorPosCallback(window, CursorPositionCallback);
			glfwSetMouseButtonCallback(window, MouseButtonCallback);
			camera.setSize({static_cast<float>(width), 
				static_cast<float>(height)});


			std::vector<std::string> globalLayers;
			std::vector<std::string> instanceExtensions;

			if (!ReleaseMode)
			{
				for (const std::string& layer : vulkanInitData["GlobalLayers"]["Debug"])
				{
					globalLayers.push_back(layer);
				}
				for (const std::string& extension : vulkanInitData["InstanceExtensions"]["Debug"])
				{
					instanceExtensions.push_back(extension);
				}
			}
			for (const std::string& layer : vulkanInitData["GlobalLayers"]["Constant"])
			{
				globalLayers.push_back(layer);
			}
			for (const std::string& extension : vulkanInitData["InstanceExtensions"]["Constant"])
			{
				instanceExtensions.push_back(extension);
			}

			std::vector<const char*> globalLayersCstrings;
			std::vector<const char*> instanceExtensionsCstrings;

			auto stringConvert = [](const std::string& s){ return s.c_str(); };

			std::transform(globalLayers.begin(),
				globalLayers.end(),
				std::back_inserter(globalLayersCstrings),
				stringConvert);

			std::transform(instanceExtensions.begin(),
				instanceExtensions.end(),
				std::back_inserter(instanceExtensionsCstrings),
				stringConvert);

			int appVersionMajor = vulkanInitData["ApplicationVersion"]["Major"];
			int appVersionMinor = vulkanInitData["ApplicationVersion"]["Minor"];
			int appVersionPatch = vulkanInitData["ApplicationVersion"]["Patch"];
			std::string engineName = vulkanInitData["EngineName"];
			int engineVersionMajor = vulkanInitData["EngineVersion"]["Major"];
			int engineVersionMinor = vulkanInitData["EngineVersion"]["Minor"];
			int engineVersionPatch = vulkanInitData["EngineVersion"]["Patch"];
			int apiVersionMajor = vulkanInitData["APIVersion"]["Major"];
			int apiVersionMinor = vulkanInitData["APIVersion"]["Minor"];
			int apiVersionPatch = vulkanInitData["APIVersion"]["Patch"];
			VkApplicationInfo applicationInfo = {};
			applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			applicationInfo.pApplicationName = appName.c_str();
			applicationInfo.applicationVersion = VK_MAKE_VERSION(appVersionMajor, appVersionMinor, appVersionPatch);
			applicationInfo.pEngineName = engineName.c_str();
			applicationInfo.engineVersion = VK_MAKE_VERSION(engineVersionMajor, engineVersionMinor, engineVersionPatch);
			applicationInfo.apiVersion = VK_MAKE_VERSION(apiVersionMajor, apiVersionMinor, apiVersionPatch);

			instance = Instance();
			LoadInstanceLevelEntryPoints(m_InstanceState);

			InitDebugCallback(m_InstanceState);

			InitPhysicalDevice(m_DeviceState, m_InstanceState.instance.get());

			SelectGraphicsQueue(m_DeviceState);

			CreateLogicalDevice(m_DeviceState, initState.deviceExtensions);
			LoadDeviceLevelEntryPoints(m_DeviceState);
			
			GetGraphicsQueue(m_DeviceState);

			CreateAllocator(m_DeviceState);

			InitCopyStructures(m_CopyState, m_DeviceState);

			// allow client app to load images
			m_InitState.imageLoadCallback();

			FinalizeImageOrder(m_RenderState);

			FinalizeSpriteOrder(m_RenderState);

			CreateVertexBuffer(
				m_RenderState,
				m_DeviceState,
				m_CopyState);

			CreateSampler(m_RenderState, m_DeviceState);

			CreateShaderModules(m_ShaderState, m_DeviceState, m_InitState);
			CreateFragmentSetLayout(m_ShaderState, m_DeviceState, m_RenderState);
			SetupPushConstants(m_ShaderState);
			CreateFragmentDescriptorPool(m_ShaderState, 
				m_DeviceState, 
				m_RenderState);
			CreateAndUpdateFragmentDescriptorSet(m_ShaderState, m_DeviceState, m_RenderState);

			int fbWidth = 0;
			int fbHeight = 0;
			glfwGetFramebufferSize(m_AppState.window, &fbWidth, &fbHeight);
			m_SurfaceState.surfaceExtent.width = fbWidth;
			m_SurfaceState.surfaceExtent.height = fbHeight;

			CreateSurface(
				m_SurfaceState, 
				m_AppState, 
				m_InstanceState, 
				m_DeviceState);
			SelectPresentMode(m_SurfaceState);
			CheckForPresentationSupport(m_DeviceState, m_SurfaceState);

			InitializeSupportStructs(m_RenderState, m_DeviceState, m_ShaderState);
			SetupImageFencePool(m_RenderState, m_DeviceState);
			SetClearColor(m_RenderState, 0.f, 0.f, 0.f, 0.f);
			CreateRenderPass(m_RenderState, m_DeviceState, m_SurfaceState);
			CreateSwapchain(m_RenderState, m_DeviceState, m_SurfaceState);

			CreatePipelineLayout(m_PipelineState, m_DeviceState, m_ShaderState);
			CreatePipeline(m_PipelineState, m_DeviceState, m_RenderState, m_ShaderState);

			SetLoopState(LoopState::Run);
			m_AppState.startupTimePoint = NowMilliseconds();
			m_AppState.currentSimulationTime = m_AppState.startupTimePoint;

			std::thread gameLoopThread(&VulkanApp::GameThread, this);

			// Event loop
			while (!glfwWindowShouldClose(m_AppState.window))
			{
				glfwWaitEvents();
				if (m_AppState.gameLoop == LoopState::DeviceLost)
				{
					gameLoopThread.join();
					return LoopState::DeviceLost;
				}
			}
			SetLoopState(LoopState::Exit);
			gameLoopThread.join();
			vkDeviceWaitIdle(m_DeviceState.device.get());
			return LoopState::Exit;
		}

		void LoadImage2D(const HashType imageID, const Bitmap& bitmap)
		{
			m_RenderState.images[imageID] = CreateImage2D(
				m_DeviceState.device.get(), 
				m_CopyState.copyCommandBuffer, 
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

		void RenderSpriteInstance(
			uint64_t spriteIndex,
			glm::mat4 transform,
			glm::vec4 color)
		{
			// TODO: may need to change sprites.at() access to array index access for performance
			auto sprite = m_RenderState.sprites.at(spriteIndex);
			auto& supports = m_RenderState.supports[m_RenderState.nextImage];
			auto& commandBuffer = supports.renderCommandBuffer;
			auto vp = m_RenderState.camera.getMatrix();
			auto m = transform;
			auto mvp = vp * m;

			VertexPushConstants pushRange;
			pushRange.imageOffset = sprite.imageOffset;
			pushRange.color = color;
			pushRange.mvp = mvp;

			vkCmdPushConstants(commandBuffer, 
				m_PipelineState.pipelineLayout.get(),
				VK_SHADER_STAGE_VERTEX_BIT,
				0, sizeof(VertexPushConstants), &pushRange);

			// draw the sprite
			vkCmdDraw(commandBuffer, VerticesPerQuad, 1, sprite.vertexOffset, 0);
		}

		virtual void LoadImages() = 0;
		virtual void Update(TimePoint_ms) = 0;
		virtual void Draw() = 0;

	private:
	
		void SetLoopState(LoopState state)
		{
			std::scoped_lock<std::mutex> loopStateLock(m_AppState.loopStateMutex);
			m_AppState.gameLoop = state;
		}

		void GameThread()
		{
			while(m_AppState.gameLoop != LoopState::Exit)
			{
				auto currentTime = NowMilliseconds();
				// Update simulation to be in sync with actual time
				size_t spriteCount = 0;
				while (currentTime - m_AppState.currentSimulationTime > UpdateDuration)
				{
					m_AppState.currentSimulationTime += UpdateDuration;

					m_InitState.updateCallback(m_AppState.currentSimulationTime);
				}
				// render a frame
				if (!BeginRender())
				{
					// TODO: probably need to handle some specific errors here
					continue;
				}
				m_InitState.renderCallback();
				EndRender();
				if (m_AppState.gameLoop == LoopState::DeviceLost)
				{
					break;
				}
			}
		}

		VkResult AcquireImage()
		{
			auto device = m_DeviceState.device.get();
			auto swapchain = m_RenderState.swapchain.get();

			VkFence imageReadyFence = m_RenderState.fencePool.back();
			m_RenderState.fencePool.pop_back();

			auto acquireResult = vkAcquireNextImageKHR(device, swapchain, 
				0, VK_NULL_HANDLE, 
				imageReadyFence, &m_RenderState.nextImage);
			
			m_RenderState.supports[m_RenderState.nextImage].imageReadyFence = imageReadyFence;
			return acquireResult;
		}

		bool BeginRender()
		{
			// try to acquire an image from the swapchain
			auto acquireResult = AcquireImage();
			if (acquireResult != VK_SUCCESS)
			{
				return false;
			}

			auto device = m_DeviceState.device.get();
			auto nextImage = m_RenderState.nextImage;
			auto& supports = m_RenderState.supports[nextImage];
			auto commandBuffer = supports.renderCommandBuffer;
			auto bufferReadyFence = supports.renderBufferExecuted.get();

			vkWaitForFences(device, 
				1, 
				&bufferReadyFence, 
				true, std::numeric_limits<uint64_t>::max());

			vkResetFences(device, 1, &bufferReadyFence);

			VkViewport viewport = {};
			viewport.x = 0.f;
			viewport.y = 0.f;
			viewport.width = static_cast<float>(m_SurfaceState.surfaceExtent.width);
			viewport.height = static_cast<float>(m_SurfaceState.surfaceExtent.height);
			viewport.minDepth = 0.f;
			viewport.maxDepth = 1.f;

			VkRect2D scissorRect = {};
			scissorRect.offset.x = 0;
			scissorRect.offset.y = 0;
			scissorRect.extent = m_SurfaceState.surfaceExtent;
			
			// record the command buffer
			vkResetCommandBuffer(commandBuffer, (VkCommandBufferResetFlags)0);
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.pNext = nullptr;
			beginInfo.flags = (VkCommandBufferUsageFlags)0;
			beginInfo.pInheritanceInfo = nullptr;
			vkBeginCommandBuffer(commandBuffer, &beginInfo);

			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
			vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
			
			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.pNext = nullptr;
			renderPassBeginInfo.renderPass = m_RenderState.renderPass.get();
			renderPassBeginInfo.framebuffer = m_RenderState.framebuffers[nextImage].get();
			renderPassBeginInfo.renderArea = scissorRect;
			renderPassBeginInfo.clearValueCount = 1;
			renderPassBeginInfo.pClearValues = &m_RenderState.clearValue;
			vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineState.pipeline.get());
			
			VkDeviceSize vertexBufferOffset = 0;
			VkBuffer vertexBuffer = m_RenderState.vertexBuffer.buffer.get();
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, 
				&vertexBuffer, 
				&vertexBufferOffset);

			// bind sampler and images uniforms
			vkCmdBindDescriptorSets(commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_PipelineState.pipelineLayout.get(),
				0,
				1, &m_ShaderState.fragmentDescriptorSet,
				0, nullptr);

			return true;
		}

		void EndRender()
		{
			auto device = m_DeviceState.device.get();
			auto& supports = m_RenderState.supports[m_RenderState.nextImage];
			auto imageReadyFence = supports.imageReadyFence;

			// Finish recording draw command buffer
			vkCmdEndRenderPass(supports.renderCommandBuffer);
			vkEndCommandBuffer(supports.renderCommandBuffer);

			// Submit draw command buffer
			VkSemaphore imageRenderComplete = supports.imageRenderCompleteSemaphore.get();
			auto stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.pNext = nullptr;
			submitInfo.waitSemaphoreCount = 0;
			submitInfo.pWaitSemaphores = nullptr;
			submitInfo.pWaitDstStageMask = nullptr;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &supports.renderCommandBuffer;
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &imageRenderComplete;

			vkWaitForFences(device, 1, &imageReadyFence, true, std::numeric_limits<uint64_t>::max());
			vkResetFences(device, 1, &imageReadyFence);
			m_RenderState.fencePool.push_back(imageReadyFence);
			supports.imageReadyFence = VK_NULL_HANDLE;

			auto drawSubmitResult = vkQueueSubmit(m_DeviceState.graphicsQueue, 1, 
				&submitInfo, supports.renderBufferExecuted.get());

			// Present image
			VkSwapchainKHR swapchain = m_RenderState.swapchain.get();
			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.pNext = nullptr;
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = &imageRenderComplete;
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &swapchain;
			presentInfo.pImageIndices = &m_RenderState.nextImage;
			presentInfo.pResults = nullptr;

			auto presentResult = vkQueuePresentKHR(
				m_DeviceState.graphicsQueue, 
				&presentInfo);
			switch (presentResult)
			{
				case VK_SUCCESS:
					// do nothing
					return;
				case VK_SUBOPTIMAL_KHR:
				case VK_ERROR_OUT_OF_DATE_KHR:
					// recreate swapchain
					DestroySwapchain(m_RenderState, m_DeviceState);
					CreateSwapchain(m_RenderState, m_DeviceState, m_SurfaceState);
					return;
				case VK_ERROR_OUT_OF_HOST_MEMORY:
					std::runtime_error("Out of host memory! Exiting application.");
					exit(1);
				case VK_ERROR_OUT_OF_DEVICE_MEMORY:
					std::runtime_error("Out of device memory! Exiting application.");
					exit(1);
				case VK_ERROR_DEVICE_LOST:
					SetLoopState(LoopState::DeviceLost);
					return;
				case VK_ERROR_SURFACE_LOST_KHR:
					std::runtime_error("Surface lost! Exiting application.");
					exit(1);
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
		// TODO: may need to sync access to extent here
		auto& surfaceExtent = GetUserPointer(window)->m_SurfaceState.surfaceExtent;
		surfaceExtent.width = static_cast<uint32_t>(width);
		surfaceExtent.height = static_cast<uint32_t>(height);
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