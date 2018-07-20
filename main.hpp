#pragma once
#include <algorithm>
#include <iostream>
#include <map>
#include <optional>
#include <variant>
#include <memory>
#include <array>

#include "vka/UniqueVulkan.hpp"
#include "vka/vulkanHelperFunctions.hpp"
#include "vka/VulkanInitializers.hpp"
#include "vka/VulkanFunctionLoader.hpp"
#include "vka/Image2D.hpp"
#include "vka/GLTF.hpp"
#include "VulkanState.hpp"
#include "vka/Input.hpp"
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



	using Material = glm::vec4;
	std::vector<Material> materials;

	GLFWwindow* window;
	vka::VS vs;
	entt::DefaultRegistry ecs;
	std::map<uint64_t, vka::glTF> models;
	vka::InputState is;

	void Update(TimePoint_ms updateTime);

	void Draw();

	void createInstance();

	void selectPhysicalDevice();

	void createDevice();

	void createWindow(const char * title, uint32_t width, uint32_t height);

	void createSurface();

	void chooseSwapExtent();

	void createRenderPass();

	void createSwapchain(const VkExtent2D& swapExtent);

	void cleanupSwapchain();

	void createSampler();

	void createPipeline2D();

	void createPipeline3D();

	void loadModels();

	void createVertexBuffers();

	void initVulkan();
};

static void PushBackInput(GLFWwindow * window, vka::InputMessage && msg);

static ClientApp * GetUserPointer(GLFWwindow * window);

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

static void CharacterCallback(GLFWwindow* window, unsigned int codepoint);

static void CursorPositionCallback(GLFWwindow* window, double xpos, double ypos);

static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
