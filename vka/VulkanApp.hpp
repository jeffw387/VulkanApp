#pragma once
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
#include "GLTF.hpp"
#include "gsl.hpp"

#include <iostream>
#include <map>
#include <functional>
#include <memory>
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
#include <cstring>

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
	using IndexType = uint8_t;
	using PositionType = glm::vec3;
	using NormalType = glm::vec3;
	constexpr size_t MaxLights = 3U;
	constexpr size_t BufferCount = 3U;

	struct FragmentPushConstants
	{
		glm::vec4 color;
	};

	class VulkanApp
	{
		friend class FrameRender;

	public:
		GLFWwindow * window;
		TimePoint_ms startupTimePoint;
		TimePoint_ms currentSimulationTime;
		bool gameLoop = true;
		LibraryHandle VulkanLibrary;
		Camera2D camera;

		VkInstance instance;
		std::optional<Instance> instanceOptional;
		VkDebugReportCallbackEXTUnique debugCallbackUnique;
		VkPhysicalDevice physicalDevice;
		VkSurfaceKHRUnique surfaceUnique;
		VkSurfaceKHR surface;
		VkExtent2D surfaceExtent;
		VkViewport viewport;
		VkRect2D scissorRect;
		std::optional<DeviceManager> deviceOptional;
		VkDevice device;

		struct {
			struct {
				json sampler;
				json pipelineLayout;
				json pipeline;
			} c2D;
			struct {
				json pipelineLayout;
				json pipeline;
			} c3D;
			json renderPass;
			json swapchain;
		} configs;

		VkSurfaceFormatKHR surfaceFormat;
		VkRenderPass renderPass;
		VkSwapchainKHR swapchain;

		struct {
			VkSampler sampler;
			VkDescriptorSetLayout staticDescriptorSetLayout;
			VkDescriptorPool staticDescriptorPool;
			VkDescriptorSet staticDescriptorSet;
			VkDescriptorSetLayout dynamicDescriptorSetLayout;
			VkDescriptorPool dynamicDescriptorPool;
			VkDescriptorSet dynamicDescriptorSet;
			VkShaderModule vertexShader;
			VkShaderModule fragmentShader;
			VkPipelineLayout pipelineLayout;
			VkPipeline pipeline;
			std::map<uint64_t, UniqueImage2D> images;
			std::map<uint64_t, Sprite> sprites;
			std::vector<Quad> quads;
			UniqueAllocatedBuffer vertexBuffer;
		} data2D;

		struct {
			std::map<uint64_t, Model> models;
			std::vector<IndexType> vertexIndices;
			std::vector<PositionType> vertexPositions;
			std::vector<NormalType> vertexNormals;
			UniqueAllocatedBuffer indexBuffer;
			UniqueAllocatedBuffer positionBuffer;
			UniqueAllocatedBuffer normalBuffer;
			VkDescriptorSetLayout staticDescriptorSetLayout;
			VkDescriptorPool staticDescriptorPool;
			VkDescriptorSet staticDescriptorSet;
			VkDescriptorSetLayout dynamicDescriptorSetLayout;
			VkDescriptorPool dynamicDescriptorPool;
			VkDescriptorSet dynamicDescriptorSet; 
			VkShaderModule vertexShader;
			VkShaderModule fragmentShader;
			VkPipelineLayout pipelineLayout;
			VkPipeline pipeline;
		} data3D;

		VkCommandPool utilityCommandPool;
		VkCommandBuffer utilityCommandBuffer;
		VkFence utilityCommandFence;

		struct PerImageResources
		{
			struct {
				struct {
					struct {
						UniqueAllocatedBuffer buffer;
						size_t count;
						size_t capacity;
					} dynamic;
					struct {
						UniqueAllocatedBuffer buffer;
					} fixed;
				} matrices;
				struct {
					UniqueAllocatedBuffer buffer;
				} lights;
			} uniforms;
			struct {
				VkImage image;
				VkImageView view;
				VkFramebuffer framebuffer;

			} swap;
			VkCommandBuffer renderCommandBuffer;
			VkFence renderCommandBufferExecutedFence;
			VkSemaphore imageRenderedSemaphore;
		};
		std::array<PerImageResources, BufferCount> perImageResources;
		VkDeviceSize uniformBufferAlignment;
		VkCommandPool renderCommandPool;
		Pool<VkFence> imagePresentedFencePool;
		uint32_t nextImage;
		VkClearValue clearValue;

		CircularQueue<InputMessage, 500> inputBuffer;
		InputBindMap inputBindMap;
		InputStateMap inputStateMap;
		double cursorX;
		double cursorY;

		virtual void LoadModels() = 0;
		virtual void LoadImages() = 0;
		virtual void Update(TimePoint_ms) = 0;
		virtual void Draw() = 0;

		void CleanUpSwapchain();

		void CreateSwapchain();

		void CreateSurface();

		void UpdateSurfaceSize();

		void Run(std::string vulkanInitJsonPath);

		void LoadModelFromFile(std::string path, entt::HashedString fileName);

		void CreateImage2D(const HashType imageID, const Bitmap &bitmap);

		void CreateSprite(const HashType imageID, const HashType spriteName, const Quad quad);

		void BeginRenderPass(const uint32_t& instanceCount);

		void BindPipeline2D();

		void BindPipeline3D();

		void RenderModel(const uint64_t modelIndex, const glm::mat4 modelMatrix, const glm::vec4 modelColor);

		void EndRenderPass();

		void PresentImage();

	private:
		void AcquireNextImage(VkFence& fence);

		void CreateVertexBuffers2D();

		void CreateVertexBuffers3D();

		void CreateMatrixBuffer(size_t imageIndex, VkDeviceSize newSize);

		void SetClearColor(float r, float g, float b, float a);

		void GameThread();

		void UpdateCameraSize();
};

	enum class RenderResults
	{
		Continue,
		Return
	};

	static RenderResults HandleRenderErrors(VkResult result)
	{
		switch (result)
		{
			// successes
		case VK_NOT_READY:
			return RenderResults::Return;
		case VK_SUCCESS:
			return RenderResults::Continue;

			// recoverable errors
		case VK_SUBOPTIMAL_KHR:
			throw Results::Suboptimal();
		case VK_ERROR_OUT_OF_DATE_KHR:
			throw Results::ErrorOutOfDate();
		case VK_ERROR_SURFACE_LOST_KHR:
			throw Results::ErrorSurfaceLost();

			// unrecoverable errors
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			throw Results::ErrorOutOfHostMemory();
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			throw Results::ErrorOutOfDeviceMemory();
		case VK_ERROR_DEVICE_LOST:
			throw Results::ErrorDeviceLost();
		}
		return RenderResults::Continue;
	}

	static void FrameRender(
		const VkDevice& device,
		uint32_t& nextImage,
		const VkSwapchainKHR& swapchain,
		const VkFence& imagePresentedFence,
		const VkCommandBuffer& renderCommandBuffer,
		const VkFence& renderCommandBufferExecutedFence,
		const VkFramebuffer& framebuffer,
		const VkSemaphore& imageRenderedSemaphore,
		std::function<void()> drawFunc,
		const VkRenderPass& renderPass,
		const VkDescriptorSet& fragmentDescriptorSet2D,
		const VkDescriptorSet& vertexDescriptorSet2D,
		const VkDescriptorSet& vertexDescriptorSet3D,
		const VkPipelineLayout& pipelineLayout2D,
		const VkPipelineLayout& pipelineLayout3D,
		const VkPipeline& pipeline2D,
		const VkPipeline& pipeline3D,
		const VkBuffer& vertexBuffer2D,
		const VkBuffer& indexBuffer3D,
		const VkBuffer& positionBuffer3D,
		const VkBuffer& normalBuffer3D,
		const VkQueue& graphicsQueue,
		const VkExtent2D& extent,
		const VkClearValue& clearValue)
	{
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

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = nullptr;
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.framebuffer = framebuffer;
		renderPassBeginInfo.renderArea = scissorRect;
		renderPassBeginInfo.clearValueCount = 1;
		renderPassBeginInfo.pClearValues = &clearValue;
		vkCmdBeginRenderPass(renderCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(renderCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline2D);

		vkCmdSetViewport(renderCommandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(renderCommandBuffer, 0, 1, &scissorRect);

		VkDeviceSize vertexBufferOffset = 0;
		vkCmdBindVertexBuffers(renderCommandBuffer, 0, 1,
			&vertexBuffer2D,
			&vertexBufferOffset);

		// bind sampler and images uniforms
		vkCmdBindDescriptorSets(renderCommandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout2D,
			0,
			1, &fragmentDescriptorSet2D,
			0, nullptr);

		drawFunc();

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

		auto drawSubmitResult = vkQueueSubmit(graphicsQueue, 1,
			&submitInfo, renderCommandBufferExecutedFence);

		// Present image
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

		HandleRenderErrors(presentResult);
		fencePool.pool(imagePresentedFence);
	}

	static VulkanApp *GetUserPointer(GLFWwindow *window)
	{
		return reinterpret_cast<VulkanApp *>(glfwGetWindowUserPointer(window));
	}

	static void PushBackInput(GLFWwindow *window, InputMessage &&msg)
	{
		auto &app = *GetUserPointer(window);
		msg.time = NowMilliseconds();
		app.inputBuffer.pushLast(std::move(msg));
	}

	static void SetCursorPosition(GLFWwindow *window, double x, double y)
	{
		auto &app = *GetUserPointer(window);
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

	template<typename ...Ts, typename ViewT>
	inline void VulkanApp::RenderModelInstances(const ViewT & view)
	{
		auto count = view.size();
		for (const auto& entity : view)
		{
			const auto&[t, c, m] = view.get<Ts...>(entity);
			const glm::mat4& transform = t;
			const glm::vec4& color = c;
			const uint64_t& modelIndex = m;
		}
	}

} // namespace vka