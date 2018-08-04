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


static VkDeviceSize SelectUniformBufferOffset(VkDeviceSize elementSize, VkDeviceSize minimumOffsetAlignment)
{
	if (elementSize <= minimumOffsetAlignment)
		return minimumOffsetAlignment;
	auto mult = (VkDeviceSize)std::ceil((double)elementSize / (double)minimumOffsetAlignment);
	return mult * minimumOffsetAlignment;
}

class ClientApp
{
public:
	const char* appName = "VulkanApp";
	const char* engineName = "VulkanEngine";

	std::array<const char*, get(Models::COUNT)> modelPaths = {
		"content/models/cylinder.gltf",
		"content/models/cube.gltf",
		"content/models/triangle.gltf",
		"content/models/icosphereSub2.gltf",
		"content/models/pentagon.gltf"
	};

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

	void PrepareECS();

	void InputThread();

	void UpdateThread();

	void Update(TimePoint_ms updateTime);

	void SortComponents();

	void BindPipeline3D(VkCommandBuffer & cmd, VkDescriptorSet & staticSet);

	void PushConstants(VkCommandBuffer & cmd, PushConstantData &pushData);

	void Draw();

	void BeginRenderPass(VkFramebuffer & framebuffer, VkCommandBuffer & cmd);

	void cleanup();

	void createInstance();

	void cleanupInstance();

	void selectPhysicalDevice();

	void selectUniformBufferOffsetAlignments();

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

	struct CreatedUniformBuffer
	{
		std::optional<vka::Buffer> stagingBuffer;
		vka::Buffer uniformBuffer;
	};
	CreatedUniformBuffer createUniformBuffer(VkDeviceSize bufferSize);

	void stageDataRecordCopy(
		VkCommandBuffer copyCommandBuffer,
		vka::Buffer buffer,
		std::function<void(void*)> copyFunc,
		std::optional<vka::Buffer> stagingBuffer);

	void createMaterialUniformBuffer();

	void cleanupMaterialUniformBuffers();

	void stageMaterialData(VkCommandBuffer cmd);

	void stageCameraData(
		VkCommandBuffer cmd,
		const CameraUniform* cameraUniformData);

	void stageLightData(
		VkCommandBuffer cmd);

	void allocateStaticSets();

	void allocateDynamicSets();

	void writeStaticSets();

	void ResizeInstanceBuffers(vka::Buffer & nextStagingBuffer, vka::Buffer & nextBuffer, unsigned long long requiredBufferSize);

	void updateInstanceBuffer();

	void writeInstanceDescriptorSet();

	void createFrameResources();

	void cleanupFrameResources();

	void initVulkan();

	void initInput();
};

static void PushBackInput(GLFWwindow * window, vka::InputMessage && msg);

static ClientApp * GetUserPointer(GLFWwindow * window);

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

static void CharacterCallback(GLFWwindow* window, unsigned int codepoint);

static void CursorPositionCallback(GLFWwindow* window, double xpos, double ypos);

static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
