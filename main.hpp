#pragma once
#include <algorithm>
#include <iostream>
#include <map>
#include <optional>
#include <variant>
#include <memory>
#include <array>
#include <thread>

#include "vka/UniqueVulkan.hpp"
#include "vka/vulkanHelperFunctions.hpp"
#include "vka/VulkanInitializers.hpp"
#include "vka/VulkanFunctionLoader.hpp"
#include "vka/Image2D.hpp"
#include "vka/GLTF.hpp"
#include "vka/Quad.hpp"
#include "vka/Camera.hpp"
#include "vka/Input.hpp"
#include "vka/Buffer.hpp"
#include "VulkanState.hpp"
#include "TimeHelper.hpp"
#include "ECSComponents.hpp"
#include "entt/entt.hpp"
#include "collision.hpp"
#include "gsl.hpp"


struct Models {
	struct Cylinder {
		static constexpr auto path = entt::HashedString("content/models/cylinder.gltf");
	};
	struct Cube {
		static constexpr auto path = entt::HashedString("content/models/cube.gltf");
	};
	struct Triangle {
		static constexpr auto path = entt::HashedString("content/models/triangle.gltf");
	};
	struct IcosphereSub2 {
		static constexpr auto path = entt::HashedString("content/models/icosphereSub2.gltf");
	};
	struct Pentagon {
		static constexpr auto path = entt::HashedString("content/models/pentagon.gltf");
	};
};

static VkDeviceSize SelectUniformBufferOffset(VkDeviceSize elementSize, VkDeviceSize minimumOffsetAlignment)
{
	if (elementSize <= minimumOffsetAlignment)
		return minimumOffsetAlignment;
	auto mult = (VkDeviceSize)std::ceil((double)elementSize / (double)minimumOffsetAlignment);
	return mult * minimumOffsetAlignment;
}

struct Materials
{
	static constexpr auto White = entt::HashedString("White");
	static constexpr auto Red = entt::HashedString("Red");
	static constexpr auto Green = entt::HashedString("Green");
	static constexpr auto Blue = entt::HashedString("Blue");
};

class ClientApp
{
public:
	const char* appName = "VulkanApp";
	const char* engineName = "VulkanEngine";
	std::vector<const char*> instanceLayers = {
		"VK_LAYER_LUNARG_standard_validation",
		"VK_LAYER_LUNARG_assistant_layer"
	};

	std::vector<const char*> instanceExtensions = {
		"VK_KHR_surface",
		"VK_KHR_win32_surface",
		"VK_EXT_debug_report"
	};

	std::vector<const char*> deviceExtensions = {
		"VK_KHR_swapchain"
	};

	struct {
		std::vector<const char*> images = {

		};
		struct {
			const char* vertex = "shaders/2D/vert.spv";
			const char* fragment = "shaders/2D/frag.spv";
		} shader2D;
		struct {
			const char* vertex = "shaders/3D/vert.spv";
			const char* fragment = "shaders/3D/frag.spv";
		} shader3D;

	} paths;

	struct Material
	{
		glm::vec4 color;
	};


	std::map<uint64_t, Material> materials
	{
		{Materials::White, Material{glm::vec4(1.f)}}, // white
		{Materials::Red, Material{glm::vec4(1.f, 0.f, 0.f, 1.f)}}, // red
		{Materials::Green, Material{glm::vec4(0.f, 1.f, 0.f, 1.f)}}, // green
		{Materials::Blue, Material{glm::vec4(0.f, 0.f, 1.f, 1.f)}} // blue
	};

	GLFWwindow* window;
	VS vs;
	using proto = entt::DefaultPrototype;
	entt::DefaultRegistry ecs;
	proto player{ ecs };
	proto triangle{ ecs };
	proto cube{ ecs };
	proto cylinder{ ecs };
	proto icosphereSub2{ ecs };
	proto pentagon{ ecs };
	std::map<uint64_t, vka::Quad> quads;
	std::map<uint64_t, vka::glTF> models;
	vka::InputState is;
	vka::Camera2D camera;

	TimePoint_ms startupTimePoint;
	TimePoint_ms currentSimulationTime;

	bool exitInputThread = false;
	bool exitUpdateThread = false;

	void SetupPrototypes();

	void InputThread();

	void UpdateThread();

	void Update(TimePoint_ms updateTime);

	void Draw();

	void cleanup();

	void createInstance();

	void cleanupInstance();

	void selectPhysicalDevice();

	void createDevice();

	void createAllocator();

	void cleanupAllocator();

	void cleanupDevice();

	void createWindow(const char * title, uint32_t width, uint32_t height);

	void cleanupWindow();

	void createSurface();

	void cleanupSurface();

	void createUtilityResources();

	void cleanupUtilityResources();

	void chooseSwapExtent();

	void createRenderPass();

	void cleanupRenderPass();

	void createSwapchain();

	void cleanupSwapchain();

	void recreateSwapchain();

	void createSampler();

	void cleanupSampler();

	void createDescriptorSetLayouts();

	void cleanupDescriptorSetLayouts();

	void createStaticDescriptorPool();

	void cleanupStaticDescriptorPool();

	void createPushRanges();

	void createPipelineLayout();

	void cleanupPipelineLayout();

	void createSpecializationData();

	void createShader2DModules();

	void createShader3DModules();

	void cleanupShaderModules();

	void setupPipeline2D();

	void setupPipeline3D();

	void createPipelines();

	void cleanupPipelines();

	void loadImages();

	void loadQuads();

	void loadModels();

	auto createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties, bool dedicatedAllocation);

	void createVertexBuffer2D();

	void createVertexBuffers3D();

	void cleanupVertexBuffers();

	void createStaticUniformBuffer();

	void cleanupStaticUniformBuffer();

	void createFrameResources();

	void cleanupFrameResources();

	void initVulkan();

	template <typename T, typename iterT>
	void CopyDataToBuffer(
		iterT begin,
		iterT end,
		vka::Buffer dst,
		VkDeviceSize dataElementOffset);

	template <typename T, typename iterT>
	void CopyDataToBuffer(
		iterT begin,
		iterT end,
		VkCommandBuffer cmd,
		vka::Buffer staging,
		vka::Buffer dst,
		VkDeviceSize dataElementOffset,
		VkFence fence,
		VkSemaphore semaphore);
};

static void PushBackInput(GLFWwindow * window, vka::InputMessage && msg);

static ClientApp * GetUserPointer(GLFWwindow * window);

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

static void CharacterCallback(GLFWwindow* window, unsigned int codepoint);

static void CursorPositionCallback(GLFWwindow* window, double xpos, double ypos);

static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
