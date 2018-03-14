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
	
	static void ResizeCallback(GLFWwindow* window, int width, int height);

	struct VulkanApp
	{
		ApplicationState m_AppState;
		InitState m_InitState;
		InputState m_InputState;
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

			supports.renderCommandBuffer->pushConstants<FragmentPushConstants>(
				m_PipelineLayout.get(),
				vk::ShaderStageFlagBits::eFragment,
				0U,
				{ pushRange });

			uint32_t dynamicOffset = static_cast<uint32_t>(m_RenderState.spriteIndex * m_MatrixBufferOffsetAlignment);

			// bind dynamic matrix uniform
			supports.renderCommandBuffer->bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				m_PipelineLayout.get(),
				0U,
				{ supports.vertexDescriptorSet.get() },
				{ dynamicOffset });

			auto vertexOffset = textureIndex * IndicesPerQuad;
			// draw the sprite
			supports.renderCommandBuffer->
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

		void Run()
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
			deviceState.logicalDevice->waitIdle();
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
					auto title = m_InitState.windowTitle + std::string(": ") + helper::uitostr(size_t(frameDurationCount)) + std::string(" microseconds");
					glfwSetwindowTitle(m_Window, title.c_str());
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

		void init(
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
			m_InitState.windowTitle = windowTitle;
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

			InitPhysicalDevice(m_DeviceState, m_Instance.get());

			SelectGraphicsQueue(m_DeviceState);

			CreateLogicalDevice(m_DeviceState, deviceExtensions);
			
			LoadVulkanDeviceFunctions(m_DeviceState);

			GetGraphicsQueue(m_DeviceState);

			CreateAllocator(m_DeviceState);

			InitCopyStructures(m_InitState, m_DeviceState);

			// allow client app to load images
			m_InitState.imageLoadCallback(this);

			CreateVertexAndIndexBuffers(
				m_RenderState, 
				m_InitState,
				m_DeviceState, 
				m_SpriteVector);

			CreateSampler(m_RenderState, m_DeviceState);

			CreateShaderModules(m_ShaderState, m_DeviceState);
			CreateVertexSetLayout(m_ShaderState, m_DeviceState);
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
				std::make_optional<vk::Extent2D>());
			SelectPresentMode(m_SurfaceState);
			CheckForPresentationSupport(m_DeviceState, m_SurfaceState);

			InitializeSupportStructs(m_RenderState, m_DeviceState, m_ShaderState);
			CreateRenderPass(m_RenderState, m_DeviceState, m_SurfaceState);
			CreateSwapchainWithDependencies(m_RenderState, m_DeviceState, m_SurfaceState)

			CreatePipelineLayout(m_PipelineState, m_DeviceState, m_ShaderState);
			CreatePipeline(m_PipelineState, m_DeviceState, m_RenderState);
		}

	private:
		auto acquireImage()
		{
			auto renderFinishedFence = deviceState.logicalDevice->createFence(
				vk::FenceCreateInfo(vk::FenceCreateFlags()));
			auto nextImage = deviceState.logicalDevice->acquireNextImageKHR(
				renderState.swapChain.get(), 
				std::numeric_limits<uint64_t>::max(), 
				vk::Semaphore(),
				renderFinishedFence);
			renderState.supports[nextImage.value].imagePresentCompleteFence = vk::UniqueFence(renderFinishedFence, vk::FenceDeleter(deviceState.logicalDevice.get()));
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

			auto& supports = supports[nextImage.value];

			// Sync host to device for the next image
			deviceState.logicalDevice->waitForFences(
				{supports.imagePresentCompleteFence.get()}, 
				static_cast<vk::Bool32>(true), 
				std::numeric_limits<uint64_t>::max());

			const size_t minimumCount = 1;
			size_t instanceCount = std::max(minimumCount, spriteCount);
			// (re)create matrix buffer if it is smaller than required
			if (instanceCount > supports.bufferCapacity)
			{
				supports.bufferCapacity = instanceCount * 2;

				auto newBufferSize = supports.bufferCapacity * deviceState.matrixBufferOffsetAlignment;

				// create staging buffer
				supports.matrixStagingBuffer = CreateBuffer(newBufferSize,
					vk::BufferUsageFlagBits::eTransferSrc,
					vk::MemoryPropertyFlagBits::eHostCoherent |
					vk::MemoryPropertyFlagBits::eHostVisible,
					true);
					
				// create matrix buffer
				supports.matrixBuffer = CreateBuffer(newBufferSize,
					vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer,
					vk::MemoryPropertyFlagBits::eDeviceLocal,
					true);
			}

			m_RenderState.mapped = deviceState.logicalDevice->mapMemory(
				supports.matrixStagingMemory->memory, 
				supports.matrixStagingMemory->offsetInDeviceMemory,
				supports.matrixStagingMemory->size);

			m_RenderState.copyOffset = 0;
			m_RenderState.vp = m_RenderState.camera.getMatrix();

			deviceState.logicalDevice->waitForFences({ supports.renderBufferExecuted.get() }, 
			vk::Bool32(true),
			std::numeric_limits<uint64_t>::max());
			deviceState.logicalDevice->resetFences({ supports.renderBufferExecuted.get() });

			// TODO: benchmark whether staging buffer use is faster than alternatives
			auto descriptorBufferInfo = vk::DescriptorBufferInfo(
					supports.matrixBuffer.get(),
					0U,
					deviceState.matrixBufferOffsetAlignment);
			deviceState.logicalDevice->updateDescriptorSets(
				{ 
					vk::WriteDescriptorSet(
						supports.vertexDescriptorSet.get(),
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
				m_RenderPass.get(),
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
				{ m_ShaderState.vertexBuffer.get() }, 
				{ 0U });
			supports.renderCommandBuffer->bindIndexBuffer(
				m_ShaderState.indexBuffer.get(), 
				0U, 
				vk::IndexType::eUint16);

			// bind sampler and images uniforms
			supports.renderCommandBuffer->bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				m_PipelineState.pipelineLayout.get(),
				1U,
				{ m_ShaderState.fragmentDescriptorSet.get() },
				{});

			m_RenderState.spriteIndex = 0;
			return true;
		}

		void EndRender()
		{
			auto& supports = m_RenderState.supports[m_RenderState.nextImage];
			// copy matrices from staging buffer to gpu buffer
			deviceState.logicalDevice->unmapMemory(supports.matrixStagingMemory->memory);

			CopyToBuffer(
				supports.copyCommandBuffer.get(),
				supports.matrixStagingBuffer.get(),
				supports.matrixBuffer.get(),
				supports.matrixMemory->size,
				0U,
				0U,
				std::optional<vk::Fence>(),
				{},
				{ supports.matrixBufferStagingCompleteSemaphore.get() });

			// Finish recording draw command buffer
			supports.renderCommandBuffer->endRenderPass();
			supports.renderCommandBuffer->end();

			// Submit draw command buffer
			auto stageFlags = vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput);
			m_DeviceState.graphicsQueue.submit(
				{
					vk::SubmitInfo(
						1U,
						&supports.matrixBufferStagingCompleteSemaphore.get(),
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
					&m_RenderState.swapChain.get(),
					&m_RenderState.nextImage);
			auto presentResult = vkQueuePresentKHR(deviceState.graphicsQueue.operator VkQueue(), &presentInfo.operator const VkPresentInfoKHR &());
		}

		vk::Extent2D GetWindowSize()
		{
			int width = 0;
			int height = 0;
			glfwGetWindowSize(m_AppState.window, &width, &height);
			return vk::Extent2D(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
		}

		void resizeWindow(vk::Extent2D size)
		{
			std::scoped_lock<std::mutex> resizeLock(m_RecreateSurfaceMutex);
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