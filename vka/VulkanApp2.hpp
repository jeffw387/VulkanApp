#pragma once

#undef max
#undef min
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "VulkanFunctions.hpp"
#include "windows.h"
#include "GLFW/glfw3.h"
#include "ApplicationState.hpp"
#include "InitState.hpp"
#include "InstanceState.hpp"
#include "Input.hpp"
#include "DeviceState.hpp"
#include "CopyState.hpp"
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
		InitState m_InitState;
		InputState m_InputState;
		InstanceState m_InstanceState;
		DeviceState m_DeviceState;
		CopyState m_CopyState;
		SurfaceState m_SurfaceState;
		ShaderState m_ShaderState;
		RenderState m_RenderState;
		PipelineState m_PipelineState;

		void RenderSpriteInstance(
			uint64_t spriteIndex,
			glm::mat4 transform,
			glm::vec4 color)
		{
			// TODO: may need to change sprites.at() access to array index access for performance
			auto sprite = m_RenderState.sprites.at(spriteIndex);
			auto& swapchainStruct = m_RenderState.swapchains[m_RenderState.currentSwapchain];
			auto& supports = swapchainStruct.supports[swapchainStruct.nextImage];
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

		void Run()
		{
			m_AppState.gameLoop = true;
			m_AppState.startupTimePoint = NowMilliseconds();
			m_AppState.currentSimulationTime = m_AppState.startupTimePoint;

			std::thread gameLoopThread(&VulkanApp::GameThread, this);

			// Event loop
			while (!glfwWindowShouldClose(m_AppState.window))
			{
				glfwWaitEvents();
			}
			m_AppState.gameLoop = false;
			gameLoopThread.join();
			vkDeviceWaitIdle(m_DeviceState.device.get());
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

		void Init(
			std::string windowTitle,
			int width,
			int height,
			VkInstanceCreateInfo instanceCreateInfo,
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
			m_InitState.vertexShaderPath = vertexShaderPath;
			m_InitState.fragmentShaderPath = fragmentShaderPath;
			m_InitState.deviceExtensions = deviceExtensions;

			if (!LoadVulkanLibrary(m_AppState))
			{
				exit(1);
			}
			LoadExportedEntryPoints(m_AppState);
			LoadGlobalLevelEntryPoints();

			SetDefaultExtent(m_SurfaceState, width, height);
			auto glfwInitSuccess = glfwInit();
			if (!glfwInitSuccess)
			{
				std::runtime_error("Error initializing GLFW.");
				exit(1);
			}

			glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
			m_AppState.window = glfwCreateWindow(width, height, windowTitle.c_str(), NULL, NULL);
			if (m_AppState.window == NULL)
			{
				std::runtime_error("Error creating GLFW window.");
				exit(1);
			}
			glfwSetWindowUserPointer(m_AppState.window, this);
			glfwSetFramebufferSizeCallback(m_AppState.window, ResizeCallback);
			glfwSetKeyCallback(m_AppState.window, KeyCallback);
			glfwSetCharCallback(m_AppState.window, CharacterCallback);
			glfwSetCursorPosCallback(m_AppState.window, CursorPositionCallback);
			glfwSetMouseButtonCallback(m_AppState.window, MouseButtonCallback);
			m_RenderState.camera.setSize({static_cast<float>(width), static_cast<float>(height)});

			CreateInstance(m_InstanceState, instanceCreateInfo);
			LoadInstanceLevelEntryPoints(m_InstanceState);

			InitDebugCallback(m_InstanceState);

			InitPhysicalDevice(m_DeviceState, m_InstanceState.instance.get());

			SelectGraphicsQueue(m_DeviceState);

			CreateLogicalDevice(m_DeviceState, deviceExtensions);
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
		}

	private:
	void GameThread()
		{
			while(m_AppState.gameLoop)
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
			}
		}

		VkResult AcquireImage()
		{
			auto device = m_DeviceState.device.get();
			auto& swapchainStruct = m_RenderState.swapchains[m_RenderState.currentSwapchain];
			auto swapchain = swapchainStruct.swapchain.get();

			VkFence imageReadyFence = m_RenderState.fencePool.back();
			m_RenderState.fencePool.pop_back();

			auto acquireResult = vkAcquireNextImageKHR(device, swapchain, 
				0, VK_NULL_HANDLE, 
				imageReadyFence, &swapchainStruct.nextImage);
			
			swapchainStruct.supports[swapchainStruct.nextImage].imageReadyFence = imageReadyFence;
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
			auto& swapchainStruct = m_RenderState.swapchains[m_RenderState.currentSwapchain];
			auto nextImage = swapchainStruct.nextImage;
			auto& supports = swapchainStruct.supports[nextImage];
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
			renderPassBeginInfo.framebuffer = swapchainStruct.framebuffers[nextImage].get();
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
			auto& swapchainStruct = m_RenderState.swapchains[m_RenderState.currentSwapchain];
			auto& supports = swapchainStruct.supports[swapchainStruct.nextImage];
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
			VkSwapchainKHR swapchain = swapchainStruct.swapchain.get();
			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.pNext = nullptr;
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = &imageRenderComplete;
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &swapchain;
			presentInfo.pImageIndices = &swapchainStruct.nextImage;
			presentInfo.pResults = nullptr;

			auto presentResult = vkQueuePresentKHR(
				m_DeviceState.graphicsQueue, 
				&presentInfo);
			switch (presentResult)
			{
				case VK_SUCCESS:
					// do nothing
					break;
				case VK_SUBOPTIMAL_KHR:
				case VK_ERROR_OUT_OF_DATE_KHR:
					// recreate swapchain
					CreateSwapchain(m_RenderState, m_DeviceState, m_SurfaceState);
					break;
				case VK_ERROR_OUT_OF_HOST_MEMORY:
					std::runtime_error("Out of host memory! Exiting application.");
				case VK_ERROR_OUT_OF_DEVICE_MEMORY:
					std::runtime_error("Out of device memory! Exiting application.");
				case VK_ERROR_DEVICE_LOST:
					std::runtime_error("Device lost! Exiting application.");
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