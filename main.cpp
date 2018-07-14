
#define STB_IMAGE_IMPLEMENTATION
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <algorithm>
#include <iostream>
#include <map>
#include <optional>
#include <variant>
#include <memory>

#include "vka/VulkanApp.hpp"
#include "vka/UniqueVulkan.hpp"
#include "vka/vulkanCreators.hpp"
#include "vka/VulkanInitializers.hpp"
#include "vka/VulkanFunctionLoader.hpp"
#include "VulkanState.hpp"
#include "vka/Input.hpp"
#include "TimeHelper.hpp"
#include "ECSComponents.hpp"
#include "entt/entt.hpp"
#include "collision.hpp"
#include "Models.hpp"
#include "prototypes.hpp"

namespace Fonts
{
	constexpr auto AeroviasBrasil = entt::HashedString("Content/Fonts/AeroviasBrasilNF.ttf");
}

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
	vka::VulkanState vulkanState;
	entt::DefaultRegistry enttRegistry;
	Models::ModelData models;
	vka::InputState inputState;

	void Update(TimePoint_ms updateTime)
	{
		bool inputToProcess = true;
		while (inputToProcess)
		{
			auto messageOptional = inputState.inputBuffer.popFirstIf(
				[updateTime](vka::InputMessage message) { return message.time < updateTime; });

			if (messageOptional.has_value())
			{
				auto &message = *messageOptional;
				auto bindHash = inputState.inputBindMap[message.signature];
				auto stateVariant = inputState.inputStateMap[bindHash];
				vka::StateVisitor visitor;
				visitor.signature = message.signature;
				std::visit(visitor, stateVariant);
			}
			else
			{
				inputToProcess = false;
			}
		}

	}

	void Draw()
	{
	}

	void initVulkan()
	{
		vulkanState.VulkanLibrary = vka::LoadVulkanLibrary();
		vka::LoadGlobalLevelEntryPoints();
		auto appInfo = vka::applicationInfo();
		appInfo.pApplicationName = appName;
		appInfo.pEngineName = engineName;
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);
		vka::LoadInstanceLevelEntryPoints(vulkanState.instanceUnique.get());

	}
};



int main()
{
	ClientApp app;
	
	app.inputBindMap[vka::MakeSignature(GLFW_KEY_LEFT, GLFW_PRESS)] = Bindings::Left;
	app.inputBindMap[vka::MakeSignature(GLFW_KEY_RIGHT, GLFW_PRESS)] = Bindings::Right;
	app.inputBindMap[vka::MakeSignature(GLFW_KEY_UP, GLFW_PRESS)] = Bindings::Up;
	app.inputBindMap[vka::MakeSignature(GLFW_KEY_DOWN, GLFW_PRESS)] = Bindings::Down;

	app.inputStateMap[Bindings::Left] = vka::MakeAction([]() { std::cout << "Left Pressed.\n"; });
	app.inputStateMap[Bindings::Right] = vka::MakeAction([]() { std::cout << "Right Pressed.\n"; });
	app.inputStateMap[Bindings::Up] = vka::MakeAction([]() { std::cout << "Up Pressed.\n"; });
	app.inputStateMap[Bindings::Down] = vka::MakeAction([]() { std::cout << "Down Pressed.\n"; });

	// 3D render views
	app.enttRegistry.prepare<cmp::Transform, cmp::Color, Models::Cube>();
	app.enttRegistry.prepare<cmp::Transform, cmp::Color, Models::Cylinder>();
	app.enttRegistry.prepare<cmp::Transform, cmp::Color, Models::IcosphereSub2>();
	app.enttRegistry.prepare<cmp::Transform, cmp::Color, Models::Pentagon>();
	app.enttRegistry.prepare<cmp::Transform, cmp::Color, Models::Triangle>();


	// 3D entities
}