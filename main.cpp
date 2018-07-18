
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
constexpr VkExtent2D DefaultWindowSize = { 900, 900 };

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
	std::vector<const char*> imagePaths = {

	};

	struct {
		const char* vertex = "shaders/2D/vert.spv";
		const char* fragment = "shaders/2D/frag.spv";
	} shader2D;

	struct {
		const char* vertex = "shaders/3D/vert.spv";
		const char* fragment = "shaders/3D/frag.spv";
	} shader3D;
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

	void createInstance()
	{
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
	}

	void createDevice()
	{
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
		vs.unique.device = vka::CreateDeviceUnique(vs.physicalDevice, deviceCreateInfo);
		vs.device = vs.unique.device.get();

		vka::LoadDeviceLevelEntryPoints(vs.device);

		vkGetDeviceQueue(vs.device, vs.graphicsQueueFamilyIndex, 0, &vs.graphicsQueue);
		vkGetDeviceQueue(vs.device, vs.presentQueueFamilyIndex, 0, &vs.presentQueue);
	}

	void createSurface()
	{
		vs.unique.surface = vka::CreateSurfaceUnique(vs.instance, window);
		vs.surface = vs.unique.surface.get();

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
	}

	void createRenderPass()
	{
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

		VkSubpassDependency dep3Dto2D = {};
		dep3Dto2D.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dep3Dto2D.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dep3Dto2D.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dep3Dto2D.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		dep3Dto2D.srcSubpass = 0;
		dep3Dto2D.dstSubpass = 1;
		dep3Dto2D.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		auto renderPassCreateInfo = vka::renderPassCreateInfo();
		renderPassCreateInfo.attachmentCount = gsl::narrow_cast<uint32_t>(attachmentDescriptions.size());
		renderPassCreateInfo.pAttachments = attachmentDescriptions.data();
		renderPassCreateInfo.subpassCount = gsl::narrow_cast<uint32_t>(subpassDescriptions.size());
		renderPassCreateInfo.pSubpasses = subpassDescriptions.data();
		renderPassCreateInfo.dependencyCount = 1;
		renderPassCreateInfo.pDependencies = &dep3Dto2D;

		vs.unique.renderPass = vka::CreateRenderPassUnique(vs.device, renderPassCreateInfo);
		vs.renderPass = vs.unique.renderPass.get();
	}

	void createSwapchain(const VkExtent2D& swapExtent)
	{
		std::vector<uint32_t> queueFamilyIndices;
		queueFamilyIndices.push_back(vs.graphicsQueueFamilyIndex);
		VkSharingMode imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		if (vs.graphicsQueueFamilyIndex == vs.presentQueueFamilyIndex)
		{
			queueFamilyIndices.push_back(vs.presentQueueFamilyIndex);
			VkSharingMode imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		}
		auto createInfo = vka::swapchainCreateInfoKHR();
		createInfo.surface = vs.surface;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.imageColorSpace = vs.surfaceFormat.colorSpace;
		createInfo.imageFormat = vs.surfaceFormat.format;
		createInfo.imageExtent = swapExtent;
		createInfo.presentMode = vs.presentMode;
		createInfo.minImageCount = BufferCount;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		createInfo.imageSharingMode = imageSharingMode;
		createInfo.queueFamilyIndexCount = gsl::narrow_cast<uint32_t>(queueFamilyIndices.size());
		createInfo.pQueueFamilyIndices = queueFamilyIndices.data();

		vs.unique.swapchain = vka::CreateSwapchainUnique(vs.device, createInfo);
		vs.swapchain = vs.unique.swapchain.get();
	}

	void createSampler()
	{
		auto createInfo = vka::samplerCreateInfo();
		createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		vs.pipelines.pipeline2D.unique.sampler = vka::CreateSamplerUnique(vs.device, createInfo);
		vs.pipelines.pipeline2D.sampler = vs.pipelines.pipeline2D.unique.sampler.get();
	}

	void createPipeline2D()
	{
		auto& staticBinding0 = vs.pipelines.pipeline2D.staticBindings.emplace_back();
		staticBinding0.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		staticBinding0.binding = 0;
		staticBinding0.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		staticBinding0.descriptorCount = 1;
		staticBinding0.pImmutableSamplers = &vs.pipelines.pipeline2D.sampler;
		auto& staticBinding1 = vs.pipelines.pipeline2D.staticBindings.emplace_back();
		staticBinding1.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		staticBinding1.binding = 1;
		staticBinding1.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		staticBinding1.descriptorCount = imagePaths.size();
		auto staticLayoutCreateInfo = vka::descriptorSetLayoutCreateInfo(vs.pipelines.pipeline2D.staticBindings);
		vs.pipelines.pipeline2D.unique.staticLayout = vka::CreateDescriptorSetLayoutUnique(vs.device, staticLayoutCreateInfo);
		vs.pipelines.pipeline2D.staticLayout = vs.pipelines.pipeline2D.unique.staticLayout.get();

		auto& frameBinding0 = vs.pipelines.pipeline2D.frameBindings.emplace_back();
		frameBinding0.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		frameBinding0.binding = 0;
		frameBinding0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		frameBinding0.descriptorCount = 1;
		auto frameLayoutCreateInfo = vka::descriptorSetLayoutCreateInfo(vs.pipelines.pipeline2D.frameBindings);
		vs.pipelines.pipeline2D.unique.frameLayout = vka::CreateDescriptorSetLayoutUnique(vs.device, frameLayoutCreateInfo);
		vs.pipelines.pipeline2D.frameLayout = vs.pipelines.pipeline2D.unique.frameLayout.get();

		auto& modelBinding0 = vs.pipelines.pipeline2D.modelBindings.emplace_back();
		modelBinding0.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		modelBinding0.binding = 0;
		modelBinding0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		modelBinding0.descriptorCount = 1;
		auto modelLayoutCreateInfo = vka::descriptorSetLayoutCreateInfo(vs.pipelines.pipeline2D.modelBindings);
		vs.pipelines.pipeline2D.unique.modelLayout = vka::CreateDescriptorSetLayoutUnique(vs.device, modelLayoutCreateInfo);
		vs.pipelines.pipeline2D.modelLayout = vs.pipelines.pipeline2D.unique.modelLayout.get();

		auto& drawBinding0 = vs.pipelines.pipeline2D.drawBindings.emplace_back();
		drawBinding0.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		drawBinding0.binding = 0;
		drawBinding0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		drawBinding0.descriptorCount = 1;
		auto drawLayoutCreateInfo = vka::descriptorSetLayoutCreateInfo(vs.pipelines.pipeline2D.drawBindings);
		vs.pipelines.pipeline2D.unique.drawLayout = vka::CreateDescriptorSetLayoutUnique(vs.device, drawLayoutCreateInfo);
		vs.pipelines.pipeline2D.drawLayout = vs.pipelines.pipeline2D.unique.drawLayout.get();

		std::vector<VkDescriptorSetLayout> layouts;
		layouts.push_back(vs.pipelines.pipeline2D.staticLayout);
		layouts.push_back(vs.pipelines.pipeline2D.frameLayout);
		layouts.push_back(vs.pipelines.pipeline2D.modelLayout);
		layouts.push_back(vs.pipelines.pipeline2D.drawLayout);
		auto pipelineLayoutCreateInfo = vka::pipelineLayoutCreateInfo(layouts.data(), gsl::narrow_cast<uint32_t>(layouts.size()));
		vs.pipelines.pipeline2D.unique.pipelineLayout = vka::CreatePipelineLayoutUnique(vs.device, pipelineLayoutCreateInfo);
		vs.pipelines.pipeline2D.pipelineLayout = vs.pipelines.pipeline2D.unique.pipelineLayout.get();

		auto vertexCreateInfo = vka::shaderModuleCreateInfo();
		auto vertexBytes = fileIO::readFile(std::string(shader2D.vertex));
		vertexCreateInfo.codeSize = vertexBytes.size();
		vertexCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vertexBytes.data());
	}

	void initVulkan()
	{
		vs.VulkanLibrary = vka::LoadVulkanLibrary();
		vka::LoadExportedEntryPoints(vs.VulkanLibrary);
		vka::LoadGlobalLevelEntryPoints();

		createInstance();

		auto physicalDevices = vka::GetPhysicalDevices(vs.instance);
		// TODO: error handling if no physical devices found
		vs.physicalDevice = physicalDevices.at(0);

		createSurface();

		createDevice();

		createRenderPass();

		createSwapchain(DefaultWindowSize);

		createSampler();
		

		
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