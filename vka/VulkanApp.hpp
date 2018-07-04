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

struct VertexPushConstants
{
	glm::mat4 mvp;
};

struct FragmentPushConstants
{
	glm::uint32 imageOffset;
	glm::vec3 padding;
	glm::vec4 color;
};

struct PushConstants
{
	VertexPushConstants vertexPushConstants;
	FragmentPushConstants fragmentPushConstants;
};

class VulkanApp
{
	friend class FrameRender;

  public:
	GLFWwindow *window;
	TimePoint_ms startupTimePoint;
	TimePoint_ms currentSimulationTime;
	bool gameLoop = true;
	LibraryHandle VulkanLibrary;
	Camera2D camera;

	std::optional<Instance> instanceOptional;
	VkDebugReportCallbackEXTUnique debugCallbackUnique;
	VkDevice device;
	std::optional<Device> deviceOptional;

	std::map<std::string, VkRenderPass> renderPasses;
	std::map<std::string, VkSampler> samplers;
	std::map<std::string, VkShaderModule> shaderModules;
	std::map<std::string, VkDescriptorSetLayout> descriptorSetLayouts;
	std::map<std::string, VkPipelineLayout> pipelineLayouts;
	std::map<std::string, VkPipeline> pipelines;

	VkRenderPass renderPass;

	VkSampler sampler2D;
	VkShaderModule vertex2D;
	VkDescriptorSetLayout fragment2DdescriptorSetLayout;
	VkShaderModule fragment2D;
	VkPipelineLayout pipeline2Dlayout;
	VkPipeline pipeline2D;

	VkShaderModule vertex3D;
	VkShaderModule fragment3D;
	VkPipelineLayout pipeline3Dlayout;
	VkPipeline pipeline3D;

	VkCommandPool utilityCommandPool;
	VkCommandBuffer utilityCommandBuffer;
	VkFence utilityCommandFence;

	std::map<uint64_t, Sprite> sprites2D;
	std::map<uint64_t, Model> models3D;
	std::vector<IndexType> vertexIndices3D;
	std::vector<PositionType> vertexPositions3D;
	std::vector<NormalType> vertexNormals3D;
	UniqueAllocatedBuffer indexBuffer3D;
	UniqueAllocatedBuffer positionBuffer3D;
	UniqueAllocatedBuffer normalBuffer3D;

	std::map<uint64_t, UniqueImage2D> images2D;
	std::vector<Quad> quads2D;
	UniqueAllocatedBuffer vertexBuffer2D;

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

	virtual void LoadModels() = 0;
	virtual void LoadImages() = 0;
	virtual void Update(TimePoint_ms) = 0;
	virtual void Draw() = 0;

	void Run(std::string vulkanInitJsonPath, std::string vertexShaderPath, std::string fragmentShaderPath);

	void LoadModelFromFile(std::string path, entt::HashedString fileName);

	void LoadImage2D(const HashType imageID, const Bitmap &bitmap);

	void CreateSprite(const HashType imageID, const HashType spriteName, const Quad quad);

	void RenderSpriteInstance(
		uint64_t spriteIndex,
		glm::mat4 transform,
		glm::vec4 color);

	void RenderModelInstance(
		uint64_t modelIndex,
		glm::mat4 transform,
		glm::vec4 color);

  private:
	void FinalizeImageOrder();

	void FinalizeSpriteOrder();

	void Create2DVertexBuffer();

	void Create3DVertexBuffers();

	void SetClearColor(float r, float g, float b, float a);

	void GameThread();

	void UpdateCameraSize();
};

static void FrameRender(const VkDevice& device,
	uint32_t& nextImage,
	const VkSwapchainKHR& swapchain,
	Pool<VkFence>& fencePool,
	std::function<VkFence()> fenceCreateFunc,
	const std::vector<VkCommandBuffer>& renderCommandBuffers,
	const std::vector<VkFence>& renderCommandBufferExecutedFences,
	std::function<VkFramebuffer(uint32_t)> framebufferGetFunc,
	std::function<void()> drawFunc,
	const std::vector<VkSemaphore>& imageRenderedSemaphores,
	const VkRenderPass& renderPass,
	const VkDescriptorSet& fragment2DdescriptorSet,
	const VkPipelineLayout& pipeline2Dlayout,
	const VkPipelineLayout& pipeline3Dlayout,
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
	auto imagePresentedFence = fencePool.unpoolOrCreate(fenceCreateFunc);
	auto acquireResult = vkAcquireNextImageKHR(device, swapchain,
		0, VK_NULL_HANDLE,
		imagePresentedFence, &nextImage);

	auto successResult = HandleRenderErrors(acquireResult);
	if (successResult == RenderResults::Return)
	{
		return;
	}

	// frame-dependent resources
	auto renderCommandBuffer = renderCommandBuffers[nextImage];
	auto renderCommandBufferExecutedFence = renderCommandBufferExecutedFences[nextImage];
	auto framebuffer = framebufferGetFunc(nextImage);
	auto imageRenderedSemaphore = imageRenderedSemaphores[nextImage];

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
		pipeline2Dlayout,
		0,
		1, &fragment2DdescriptorSet,
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

} // namespace vka