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

	class VulkanApp
	{
		friend class FrameRender;
	public:
		GLFWwindow* window;
        TimePoint_ms startupTimePoint;
        TimePoint_ms currentSimulationTime;
        bool gameLoop = true;
        LibraryHandle VulkanLibrary;
		Camera2D camera;

		std::optional<Instance> instanceOptional;
		VkDebugReportCallbackEXTUnique debugCallbackUnique;
		VkDevice device;
		std::optional<Device> deviceOptional;

		VkCommandPool utilityCommandPool;
		VkCommandBuffer utilityCommandBuffer;
		VkFence utilityCommandFence;

		std::map<uint64_t, UniqueImage2D> images;
        std::map<uint64_t, Sprite> sprites;
        std::vector<Quad> quads;
		std::vector<glTF> models;
		std::vector<VkBufferUnique> indexBuffers;
		std::vector<VkBufferUnique> positionBuffers;
		std::vector<VkBufferUnique> normalBuffers;


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

		void Run(std::string vulkanInitJsonPath, std::string vertexShaderPath, std::string fragmentShaderPath);

		void LoadImage2D(const HashType imageID, const Bitmap& bitmap);

		void CreateSprite(const HashType imageID, const HashType spriteName, const Quad quad);

		void CreateMesh(const glTF & glTFData);

		void RenderSpriteInstance(
			uint64_t spriteIndex,
			glm::mat4 transform,
			glm::vec4 color);

	private:
		void FinalizeImageOrder();

		void FinalizeSpriteOrder();

		void CreateVertexBuffer();

		void SetClearColor(float r, float g, float b, float a);

		void GameThread();

		void UpdateCameraSize();

		VkFence GetFenceFromImagePresentedPool();

		void ReturnFenceToImagePresentedPool(VkFence fence);
	};

	class FrameRender
	{
		VulkanApp& app;

		VkDevice device;
		VkSwapchainKHR swapchain;
		VkRenderPass renderPass;
		VkFramebuffer framebuffer;
		VkDescriptorSet fragmentDescriptorSet;
		VkPipelineLayout pipelineLayout;
		VkPipeline pipeline;
		VkFence imagePresentedFence;
		VkFence renderCommandBufferExecutedFence;
		VkCommandBuffer renderCommandBuffer;
		VkBuffer vertexBuffer;
		VkQueue graphicsQueue;
		VkSemaphore imageRenderedSemaphore;
		VkExtent2D extent;
        VkClearValue clearValue;

	public:
		FrameRender(VulkanApp& app);
		~FrameRender();
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