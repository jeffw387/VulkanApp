
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
#include "vka/vulkanHelperFunctions.hpp"
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
	GLFWwindow* window;
	vka::VS vs;
	entt::DefaultRegistry ecs;
	Models::ModelData models;
	vka::InputState is;

	void Update(TimePoint_ms updateTime)
	{
		bool inputToProcess = true;
		while (inputToProcess)
		{
			auto messageOptional = is.inputBuffer.popFirstIf(
				[updateTime](vka::InputMessage message) { return message.time < updateTime; });

			if (messageOptional.has_value())
			{
				auto &message = *messageOptional;
				auto bindHash = is.inputBindMap[message.signature];
				auto stateVariant = is.inputStateMap[bindHash];
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
		vs.VulkanLibrary = vka::LoadVulkanLibrary();
		vka::LoadExportedEntryPoints(vs.VulkanLibrary);
		vka::LoadGlobalLevelEntryPoints();

		auto appInfo = vka::applicationInfo();
		appInfo.pApplicationName = appName;
		appInfo.pEngineName = engineName;
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

		auto instanceCreateInfo = vka::instanceCreateInfo();
		instanceCreateInfo.pApplicationInfo = &appInfo;
		instanceCreateInfo.enabledLayerCount = gsl::narrow_cast<uint32_t>(instanceLayers.size());
		instanceCreateInfo.ppEnabledLayerNames = instanceLayers.data();
		instanceCreateInfo.enabledExtensionCount = gsl::narrow_cast<uint32_t>(instanceExtensions.size());
		instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
		vs.unique.instance = vka::CreateInstanceUnique(instanceCreateInfo);
		vs.instance = vs.unique.instance.get();

		vka::LoadInstanceLevelEntryPoints(vs.instance);

		auto physicalDevices = vka::GetPhysicalDevices(vs.instance);
		// TODO: error handling if no physical devices found
		auto physicalDevice = physicalDevices.at(0);

		vs.unique.surface = vka::CreateSurfaceUnique(vs.instance, window);
		vs.surface = vs.unique.surface.get();

		auto queueSearch = [](
			std::vector<VkQueueFamilyProperties>& properties,
			VkPhysicalDevice physicalDevice,
			VkSurfaceKHR surface,
			VkQueueFlags flags,
			bool presentSupportNeeded)
		{
			std::optional<uint32_t> result;
			for (uint32_t i = 0; i < properties.size(); ++i)
			{
				const auto& prop = properties[i];
				auto flagMatch = flags & prop.queueFlags;
				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
				auto presentMatch = presentSupportNeeded & presentSupport;
				if (flagMatch &&presentMatch)
				{
					result = i;
					return result;
				}
			}
			return result;
		};

		vs.graphicsQueueFamilyIndex = queueSearch(
			vs.queueFamilyProperties, 
			vs.physicalDevice, 
			vs.surface, 
			VK_QUEUE_GRAPHICS_BIT, 
			false).value();

		vs.presentQueueFamilyIndex = queueSearch(
			vs.queueFamilyProperties,
			vs.physicalDevice,
			vs.surface,
			0,
			true).value();

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

		queueCreateInfos.push_back(vka::deviceQueueCreateInfo());
		auto& graphicsQueueCreateInfo = queueCreateInfos.back();
		graphicsQueueCreateInfo.queueFamilyIndex = vs.graphicsQueueFamilyIndex;
		graphicsQueueCreateInfo.queueCount = 1;
		graphicsQueueCreateInfo.pQueuePriorities = &vs.graphicsQueuePriority;

		queueCreateInfos.push_back(vka::deviceQueueCreateInfo());
		auto presentQueueCreateInfo = queueCreateInfos.back();
		presentQueueCreateInfo.queueFamilyIndex = vs.presentQueueFamilyIndex;
		presentQueueCreateInfo.queueCount = 1;
		presentQueueCreateInfo.pQueuePriorities = &vs.presentQueuePriority;

		auto deviceCreateInfo = vka::deviceCreateInfo();
		deviceCreateInfo.enabledExtensionCount = gsl::narrow_cast<uint32_t>(deviceExtensions.size());
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		deviceCreateInfo.queueCreateInfoCount = gsl::narrow_cast<uint32_t>(queueCreateInfos.size());
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
		vs.unique.device = vka::CreateDeviceUnique(physicalDevice, deviceCreateInfo);
		vs.device = vs.unique.device.get();

		vka::LoadDeviceLevelEntryPoints(vs.device);

		auto chooseSurfaceFormat = [](const std::vector<VkSurfaceFormatKHR> supportedFormats, VkFormat preferredFormat, VkColorSpaceKHR preferredColorSpace)
		{
			if (supportedFormats[0].format == VK_FORMAT_UNDEFINED)
			{
				return VkSurfaceFormatKHR{ preferredFormat, preferredColorSpace };
			}
			for (const auto& surfFormat : supportedFormats)
			{
				if (surfFormat.format == preferredFormat && surfFormat.colorSpace == preferredColorSpace)
				{
					return surfFormat;
				}
			}
			return supportedFormats[0];
		};

		vs.supportedSurfaceFormats = vka::GetSurfaceFormats(vs.physicalDevice, vs.surface);
		vs.surfaceFormat = chooseSurfaceFormat(vs.supportedSurfaceFormats, VK_FORMAT_R8G8B8A8_SNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);
		
		auto choosePresentMode = [](const std::vector<VkPresentModeKHR>& supported, VkPresentModeKHR preferredPresentMode)
		{
			for (const auto& presentMode : supported)
			{
				if (presentMode == preferredPresentMode)
					return presentMode;
			}
			return supported[0];
		};

		vs.supportedPresentModes = vka::GetPresentModes(vs.physicalDevice, vs.surface);
		vs.presentMode = choosePresentMode(vs.supportedPresentModes, VK_PRESENT_MODE_FIFO_KHR);

		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = vs.surfaceFormat.format;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

		vs.depthFormat = VK_FORMAT_D32_SFLOAT;
		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = vs.depthFormat;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		
		std::vector<VkAttachmentDescription> attachmentDescriptions;
		attachmentDescriptions.push_back(colorAttachment);
		attachmentDescriptions.push_back(depthAttachment);

		VkAttachmentReference colorRef = {};
		colorRef.attachment = 0;
		colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; 

		VkAttachmentReference depthRef = {};
		depthRef.attachment = 1;
		depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass3D = {};
		subpass3D.colorAttachmentCount = 1;
		subpass3D.pColorAttachments = &colorRef;
		subpass3D.pDepthStencilAttachment = &depthRef;
		subpass3D.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		
		VkSubpassDescription subpass2D = {};
		subpass2D.colorAttachmentCount = 1;
		subpass2D.pColorAttachments = &colorRef;
		subpass2D.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		std::vector<VkSubpassDescription> subpassDescriptions;
		subpassDescriptions.push_back(subpass3D);
		subpassDescriptions.push_back(subpass2D);

		auto renderPassCreateInfo = vka::renderPassCreateInfo();
		renderPassCreateInfo.attachmentCount = gsl::narrow_cast<uint32_t>(attachmentDescriptions.size());
		renderPassCreateInfo.pAttachments = attachmentDescriptions.data();
		renderPassCreateInfo.subpassCount = gsl::narrow_cast<uint32_t>(subpassDescriptions.size());
		renderPassCreateInfo.pSubpasses = subpassDescriptions.data();
		renderPassCreateInfo.dependencyCount = ;
		renderPassCreateInfo.pDependencies = ;
	}
};



int main()
{
	ClientApp app;
	
	app.is.inputBindMap[vka::MakeSignature(GLFW_KEY_LEFT, GLFW_PRESS)] = Bindings::Left;
	app.is.inputBindMap[vka::MakeSignature(GLFW_KEY_RIGHT, GLFW_PRESS)] = Bindings::Right;
	app.is.inputBindMap[vka::MakeSignature(GLFW_KEY_UP, GLFW_PRESS)] = Bindings::Up;
	app.is.inputBindMap[vka::MakeSignature(GLFW_KEY_DOWN, GLFW_PRESS)] = Bindings::Down;

	app.is.inputStateMap[Bindings::Left] = vka::MakeAction([]() { std::cout << "Left Pressed.\n"; });
	app.is.inputStateMap[Bindings::Right] = vka::MakeAction([]() { std::cout << "Right Pressed.\n"; });
	app.is.inputStateMap[Bindings::Up] = vka::MakeAction([]() { std::cout << "Up Pressed.\n"; });
	app.is.inputStateMap[Bindings::Down] = vka::MakeAction([]() { std::cout << "Down Pressed.\n"; });

	// 3D render views
	app.ecs.prepare<cmp::Transform, cmp::Color, Models::Cube>();
	app.ecs.prepare<cmp::Transform, cmp::Color, Models::Cylinder>();
	app.ecs.prepare<cmp::Transform, cmp::Color, Models::IcosphereSub2>();
	app.ecs.prepare<cmp::Transform, cmp::Color, Models::Pentagon>();
	app.ecs.prepare<cmp::Transform, cmp::Color, Models::Triangle>();


	// 3D entities
}