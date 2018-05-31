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
#include "mymath.hpp"
#include "CircularQueue.hpp"
#include "TimeHelper.hpp"
#include "nlohmann/json.hpp"
#include "Vertex.hpp"
#include "Results.hpp"
#include "Debug.hpp"
#include "Pool.hpp"
#include "entt.hpp"

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
#include <array>

#undef max
#undef min

#ifdef NO_VALIDATION
	constexpr bool ReleaseMode = true;
#else
	constexpr bool ReleaseMode = false;
#endif

namespace vka
{
	using json = nlohmann::json;
	using HashType = entt::HashedString::hash_type;

	struct VulkanApp
	{
		GLFWwindow* window;
        TimePoint_ms startupTimePoint;
        TimePoint_ms currentSimulationTime;
        bool gameLoop = true;
        LibraryHandle VulkanLibrary;
		Camera2D camera;

		std::optional<Instance> instanceOptional;
		VkDevice device;
		std::optional<Device> deviceOptional;

		VkCommandPool copyCommandPool;
		VkCommandBuffer copyCommandBuffer;
		VkFence copyCommandFence;

		std::map<uint64_t, UniqueImage2D> images;
        std::map<uint64_t, Sprite> sprites;
        std::vector<Quad> quads;

        UniqueAllocatedBuffer vertexBufferUnique;
		VkDescriptorSet fragmentDescriptorSet;

		VkCommandPool renderCommandPool;
		std::vector<VkCommandBuffer> renderCommandBuffers;
		std::array<VkFence, Surface::BufferCount> renderCommandBufferExecutedFences;
		Pool<VkFence> imagePresentedFencePool;
		std::array<VkFence, Surface::BufferCount> imagePresentedFences;
		std::array<VkSemaphore, Surface::BufferCount> imageRenderedSemaphores;
        uint32_t nextImage;

        VkClearValue clearValue;

		CircularQueue<InputMessage, 500> inputBuffer;
        
        InputBindMap inputBindMap;
        InputStateMap inputStateMap;

        double cursorX;
        double cursorY;

		virtual void LoadImages() = 0;
		virtual void Update(TimePoint_ms) = 0;
		virtual void Draw() = 0;

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
			int height = vulkanInitData["DefaultWindowSize"]["Height"];
			std::string appName = vulkanInitData["ApplicationName"];
			window = glfwCreateWindow(width, height, appName.c_str(), NULL, NULL);
			if (window == NULL)
			{
				std::runtime_error("Error creating GLFW window.");
				exit(1);
			}
			glfwSetWindowUserPointer(window, this);
			glfwSetKeyCallback(window, KeyCallback);
			glfwSetCharCallback(window, CharacterCallback);
			glfwSetCursorPosCallback(window, CursorPositionCallback);
			glfwSetMouseButtonCallback(window, MouseButtonCallback);
			camera.setSize({static_cast<float>(width), 
				static_cast<float>(height)});

			std::vector<std::string> globalLayers;
			std::vector<std::string> instanceExtensions;
			std::vector<std::string> deviceExtensions;

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
				for (const std::string& extension : vulkanInitData["DeviceExtensions"]["Debug"])
				{
					deviceExtensions.push_back(extension);
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
			for (const std::string& extension : vulkanInitData["DeviceExtensions"]["Constant"])
			{
				deviceExtensions.push_back(extension);
			}

			std::vector<const char*> globalLayersCstrings;
			std::vector<const char*> instanceExtensionsCstrings;
			std::vector<const char*> deviceExtensionsCstrings;

			auto stringConvert = [](const std::string& s){ return s.c_str(); };

			std::transform(globalLayers.begin(),
				globalLayers.end(),
				std::back_inserter(globalLayersCstrings),
				stringConvert);

			std::transform(instanceExtensions.begin(),
				instanceExtensions.end(),
				std::back_inserter(instanceExtensionsCstrings),
				stringConvert);

			std::transform(deviceExtensions.begin(),
				deviceExtensions.end(),
				std::back_inserter(deviceExtensionsCstrings),
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

			VkInstanceCreateInfo instanceCreateInfo = {};
			instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			instanceCreateInfo.pApplicationInfo = &applicationInfo;
			instanceCreateInfo.enabledLayerCount = globalLayersCstrings.size();
			instanceCreateInfo.ppEnabledLayerNames = globalLayersCstrings.data();
			instanceCreateInfo.enabledExtensionCount = instanceExtensionsCstrings.size();
			instanceCreateInfo.ppEnabledExtensionNames = instanceExtensionsCstrings.data();

			instanceOptional = Instance(instanceCreateInfo);
			LoadInstanceLevelEntryPoints(instanceOptional->GetInstance());

			InitDebugCallback(instanceOptional->GetInstance());

			std::string vertexShaderPath = vulkanInitData["VertexShaderPath"];
			std::string fragmentShaderPath = vulkanInitData["FragmentShaderPath"];

			deviceOptional = std::move(Device(instanceOptional->GetInstance(),
				window,
				deviceExtensionsCstrings,
				vertexShaderPath,
				fragmentShaderPath));
			device = deviceOptional->GetDevice();

			LoadDeviceLevelEntryPoints(device);

			auto graphicsQueueID = deviceOptional->GetGraphicsQueueID();
			copyCommandPool = deviceOptional->CreateCommandPool(graphicsQueueID, true, false);
			auto renderCommandBuffers = deviceOptional->AllocateCommandBuffers(copyCommandPool, 1);
			copyCommandBuffer = renderCommandBuffers.at(0);

			copyCommandFence = deviceOptional->CreateFence(false);

			LoadImages();
			
			FinalizeImageOrder();
			FinalizeSpriteOrder();

			CreateVertexBuffer();

			auto imageInfos = std::vector<VkDescriptorImageInfo>();
			auto imageCount = images.size();
			imageInfos.reserve(imageCount);
			for (auto& imagePair : images)
			{
				VkDescriptorImageInfo imageInfo = {};
				imageInfo.sampler = VK_NULL_HANDLE;
				imageInfo.imageView = imagePair.second.view.get();
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfos.push_back(imageInfo);
			}

			auto& fragmentDescriptorSetManager = deviceOptional.value().fragmentDescriptorSetOptional.value();
			auto fragmentDescriptorSets = fragmentDescriptorSetManager.AllocateSets(1);
			fragmentDescriptorSet = fragmentDescriptorSets.at(1);

			VkWriteDescriptorSet samplerDescriptorWrite = {};
			samplerDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			samplerDescriptorWrite.pNext = nullptr;
			samplerDescriptorWrite.dstSet = fragmentDescriptorSet;
			samplerDescriptorWrite.dstBinding = 1;
			samplerDescriptorWrite.dstArrayElement = 0;
			samplerDescriptorWrite.descriptorCount = imageCount;
			samplerDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			samplerDescriptorWrite.pImageInfo = imageInfos.data();
			samplerDescriptorWrite.pBufferInfo = nullptr;
			samplerDescriptorWrite.pTexelBufferView = nullptr;

			vkUpdateDescriptorSets(device, 1, &samplerDescriptorWrite, 0, nullptr);

			renderCommandPool = deviceOptional->CreateCommandPool(graphicsQueueID, false, true);	
			renderCommandBuffers = deviceOptional->AllocateCommandBuffers(renderCommandPool, Surface::BufferCount);		
			for (auto i = 0; i < Surface::BufferCount; ++i)
			{
				renderCommandBufferExecutedFences[i] = deviceOptional->CreateFence(true);
				imagePresentedFencePool.pool(deviceOptional->CreateFence(false));
				imageRenderedSemaphores[i] = deviceOptional->CreateSemaphore();
			}			

			SetClearColor(0.f, 0.f, 0.f, 0.f);

			startupTimePoint = NowMilliseconds();
			currentSimulationTime = startupTimePoint;

			std::thread gameLoopThread(&VulkanApp::GameThread, this);

			// Event loop
			while (!glfwWindowShouldClose(window))
			{
				glfwWaitEvents();
			}
			gameLoop = false;
			gameLoopThread.join();
			vkDeviceWaitIdle(device);
		}

		void LoadImage2D(const HashType imageID, const Bitmap& bitmap)
		{
			images[imageID] = CreateImage2D(
				device, 
				copyCommandBuffer, 
				deviceOptional->GetAllocator(),
				bitmap,
				deviceOptional->GetGraphicsQueueID(),
				deviceOptional->GetGraphicsQueue());
		}

		void CreateSprite(const HashType imageID, const HashType spriteName, const Quad quad)
		{
			Sprite sprite;
			sprite.imageID = imageID;
			sprite.quad = quad;
			sprites[spriteName] = sprite;
		}

		void RenderSpriteInstance(
			uint64_t spriteIndex,
			glm::mat4 transform,
			glm::vec4 color)
		{
			// TODO: may need to change sprites.at() access to array index access for performance
			auto sprite = sprites.at(spriteIndex);
			auto& renderCommandBuffer = renderCommandBuffers.at(nextImage);
			auto vp = camera.getMatrix();
			auto m = transform;
			auto mvp = vp * m;

			VertexPushConstants pushRange;
			pushRange.imageOffset = sprite.imageOffset;
			pushRange.color = color;
			pushRange.mvp = mvp;

			vkCmdPushConstants(renderCommandBuffer, 
				deviceOptional->GetPipelineLayout(),
				VK_SHADER_STAGE_VERTEX_BIT,
				0, sizeof(VertexPushConstants), &pushRange);

			// draw the sprite
			vkCmdDraw(renderCommandBuffer, VerticesPerQuad, 1, sprite.vertexOffset, 0);
		}

	private:
	
		void FinalizeImageOrder()
		{
			auto imageOffset = 0;
			for (auto& image : images)
			{
				image.second.imageOffset = imageOffset;
				++imageOffset;
			}
		}

		void FinalizeSpriteOrder()
		{
			auto spriteOffset = 0;
			for (auto& [spriteID, sprite] : sprites)
			{
				quads.push_back(sprite.quad);
				sprite.vertexOffset = spriteOffset;
				sprite.imageOffset = images[sprite.imageID].imageOffset;
				++spriteOffset;
			}
		}

		void CreateVertexBuffer()
		{
			auto graphicsQueueFamilyID = deviceOptional->GetGraphicsQueueID();
			auto graphicsQueue = deviceOptional->GetGraphicsQueue();

			if (quads.size() == 0)
			{
				std::runtime_error("Error: no vertices loaded.");
			}
			// create vertex buffers
			constexpr auto quadSize = sizeof(Quad);
			size_t vertexBufferSize = quadSize * quads.size();
			auto vertexStagingBufferUnique = CreateBufferUnique(
				device,
				deviceOptional->GetAllocator(),
				vertexBufferSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				graphicsQueueFamilyID,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				true);
			
			vertexBufferUnique = CreateBufferUnique(
				device,
				deviceOptional->GetAllocator(),
				vertexBufferSize, 
				VK_BUFFER_USAGE_TRANSFER_DST_BIT |
					VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				graphicsQueueFamilyID,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				true);

			void* vertexStagingData = nullptr;
			auto memoryMapResult = vkMapMemory(device, 
				vertexStagingBufferUnique.allocation.get().memory,
				vertexStagingBufferUnique.allocation.get().offsetInDeviceMemory,
				vertexStagingBufferUnique.allocation.get().size,
				0,
				&vertexStagingData);
			
			memcpy(vertexStagingData, quads.data(), vertexBufferSize);
			vkUnmapMemory(device, vertexStagingBufferUnique.allocation.get().memory);

			VkBufferCopy bufferCopy = {};
			bufferCopy.srcOffset = 0;
			bufferCopy.dstOffset = 0;
			bufferCopy.size = vertexBufferSize;

			CopyToBuffer(
				copyCommandBuffer,
				graphicsQueue,
				vertexStagingBufferUnique.buffer.get(),
				vertexBufferUnique.buffer.get(),
				bufferCopy,
				copyCommandFence);
			
			// wait for vertex buffer copy to finish
			vkWaitForFences(device, 1, &copyCommandFence, (VkBool32)true, 
				std::numeric_limits<uint64_t>::max());
			vkResetFences(device, 1, &copyCommandFence);
		}

		void SetClearColor(float r, float g, float b, float a)
    {
        VkClearColorValue clearColor;
        clearColor.float32[0] = r;
        clearColor.float32[1] = g;
        clearColor.float32[2] = b;
        clearColor.float32[3] = a;
        clearValue.color = clearColor;
    }

		void GameThread()
		{
			try
			{
				while(gameLoop != false)
				{
					auto currentTime = NowMilliseconds();
					// Update simulation to be in sync with actual time
					size_t spriteCount = 0;
					while (currentTime - currentSimulationTime > UpdateDuration)
					{
						currentSimulationTime += UpdateDuration;

						Update(currentSimulationTime);
					}
					// render a frame
					if (!BeginRender())
					{
						// TODO: probably need to handle some specific errors here
						continue;
					}
					Draw();
					EndRender();
				}
			}
			catch (Results::ErrorDeviceLost)
			{
			}
			catch (Results::ErrorSurfaceLost result)
			{
			}
			catch (Results::Suboptimal)
			{
			}
			catch (Results::ErrorOutOfDate)
			{
			}
		}

		VkResult AcquireImage()
		{
			auto swapchain = deviceOptional->GetSwapchain();

			auto imagePresentedFenceOptional = imagePresentedFencePool.unpool();

			auto acquireResult = vkAcquireNextImageKHR(device, swapchain, 
				0, VK_NULL_HANDLE, 
				imagePresentedFenceOptional.value(), &nextImage);
			
			imagePresentedFences[nextImage] = imagePresentedFenceOptional.value();
			return acquireResult;
		}

		bool BeginRender()
		{
			auto acquireResult = AcquireImage();
			if (acquireResult != VK_SUCCESS)
			{
				return false;
			}

			auto renderCommandBuffer = renderCommandBuffers[nextImage];
			auto renderCommandBufferExecutedFence = renderCommandBufferExecutedFences[nextImage];

			auto extent = deviceOptional->GetExtent();

			VkViewport viewport = {};
			viewport.x = 0.f;
			viewport.y = 0.f;
			viewport.width = static_cast<float>(extent.width);
			viewport.height = static_cast<float>(extent.height);
			viewport.minDepth = 0.f;
			viewport.maxDepth = 1.f;

			VkRect2D scissorRect = {};
			scissorRect.offset.x = 0;
			scissorRect.offset.y = 0;
			scissorRect.extent = extent;

			// Wait for this frame's render command buffer to finish executing
			vkWaitForFences(device,
				1,
				&renderCommandBufferExecutedFence,
				true, std::numeric_limits<uint64_t>::max());

			vkResetFences(device, 1, &renderCommandBufferExecutedFence);
			
			// record the command buffer
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.pNext = nullptr;
			beginInfo.flags = 0;
			beginInfo.pInheritanceInfo = nullptr;
			vkBeginCommandBuffer(renderCommandBuffer, &beginInfo);

			vkCmdSetViewport(renderCommandBuffer, 0, 1, &viewport);
			vkCmdSetScissor(renderCommandBuffer, 0, 1, &scissorRect);
			
			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.pNext = nullptr;
			renderPassBeginInfo.renderPass = deviceOptional->GetRenderPass();
			renderPassBeginInfo.framebuffer = deviceOptional->GetFramebuffer(nextImage);
			renderPassBeginInfo.renderArea = scissorRect;
			renderPassBeginInfo.clearValueCount = 1;
			renderPassBeginInfo.pClearValues = &clearValue;
			vkCmdBeginRenderPass(renderCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			
			vkCmdBindPipeline(renderCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, deviceOptional->GetPipeline());
			
			VkDeviceSize vertexBufferOffset = 0;
			VkBuffer vertexBuffer = vertexBufferUnique.buffer.get();
			vkCmdBindVertexBuffers(renderCommandBuffer, 0, 1, 
				&vertexBuffer, 
				&vertexBufferOffset);

			// bind sampler and images uniforms
			vkCmdBindDescriptorSets(renderCommandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				deviceOptional->GetPipelineLayout(),
				0,
				1, &fragmentDescriptorSet,
				0, nullptr);

			return true;
		}

		void EndRender()
		{
			auto& renderCommandBuffer = renderCommandBuffers[nextImage];
			auto& renderBufferExecuted = renderCommandBufferExecutedFences[nextImage];
			auto& imagePresentedFence = imagePresentedFences[nextImage];
			auto& imageRenderedSemaphore = imageRenderedSemaphores[nextImage];
			auto graphicsQueue = deviceOptional->GetGraphicsQueue();

			// Finish recording draw command buffer
			vkCmdEndRenderPass(renderCommandBuffer);
			vkEndCommandBuffer(renderCommandBuffer);

			// Submit draw command buffer
			auto stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.pNext = nullptr;
			submitInfo.waitSemaphoreCount = 0;
			submitInfo.pWaitSemaphores = nullptr;
			submitInfo.pWaitDstStageMask = nullptr;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &renderCommandBuffer;
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &imageRenderedSemaphore;

			vkWaitForFences(device, 1, &imagePresentedFence, true, std::numeric_limits<uint64_t>::max());
			vkResetFences(device, 1, &imagePresentedFence);
			imagePresentedFencePool.pool(imagePresentedFence);
			imagePresentedFence = VK_NULL_HANDLE;

			auto drawSubmitResult = vkQueueSubmit(graphicsQueue, 1, 
				&submitInfo, renderBufferExecuted);

			// Present image
			VkSwapchainKHR swapchain = deviceOptional->GetSwapchain();
			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.pNext = nullptr;
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = &imageRenderedSemaphore;
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &swapchain;
			presentInfo.pImageIndices = &nextImage;
			presentInfo.pResults = nullptr;

			auto presentResult = vkQueuePresentKHR(
				graphicsQueue, 
				&presentInfo);
			switch (presentResult)
			{
				case VK_SUCCESS:
					// do nothing
					return;
				case VK_SUBOPTIMAL_KHR:
				case VK_ERROR_OUT_OF_DATE_KHR:
					// recreate swapchain
					throw Results::ErrorOutOfDate();
					return;
				case VK_ERROR_OUT_OF_HOST_MEMORY:
					throw Results::ErrorOutOfHostMemory();
				case VK_ERROR_OUT_OF_DEVICE_MEMORY:
					throw Results::ErrorOutOfDeviceMemory();
				case VK_ERROR_DEVICE_LOST:
					throw Results::ErrorDeviceLost();
				case VK_ERROR_SURFACE_LOST_KHR:
					throw Results::ErrorSurfaceLost();
			}
		}
	};

	static VulkanApp* GetUserPointer(GLFWwindow* window)
	{
		return reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
	}

	static void PushBackInput(GLFWwindow* window, InputMessage&& msg)
	{
		auto& app = *GetUserPointer(window);
		msg.time = NowMilliseconds();
		app.inputBuffer.pushLast(std::move(msg));
	}

	static void SetCursorPosition(GLFWwindow* window, double x, double y)
	{
		auto& app = *GetUserPointer(window);
		app.cursorX = x;
		app.cursorY = y;
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