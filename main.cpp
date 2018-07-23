
#define STB_IMAGE_IMPLEMENTATION
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_GLYPH_H


#include "main.hpp"

namespace Fonts
{
	constexpr auto AeroviasBrasil = entt::HashedString("Content/Fonts/AeroviasBrasilNF.ttf");
}
constexpr VkExtent2D DefaultWindowSize = { 900, 900 };
constexpr uint32_t LightCount = 3;

void ClientApp::InputThread()
{
	bool shouldExit = false;
	while (!shouldExit)
	{
		glfwWaitEvents();
		if (glfwWindowShouldClose(window))
		{
			exit(0);
		}
	}
}

void ClientApp::UpdateThread()
{
	startupTimePoint = NowMilliseconds();
	currentSimulationTime = startupTimePoint;

	while (!exitUpdateThread)
	{
		auto currentTime = NowMilliseconds();
		while (currentTime - currentSimulationTime > UpdateDuration)
			Update(currentSimulationTime);
		Draw();
	}
}

void ClientApp::Update(TimePoint_ms updateTime)
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

void ClientApp::Draw()
{
	// acquire image
	// update matrix buffers
	// update matrix descriptor set if needed
}

void ClientApp::cleanup()
{
	vkDeviceWaitIdle(vs.device);
	cleanupFrameResources();
	cleanupStaticUniformBuffer();
	cleanupVertexBuffers();
	cleanupPipelines();
	cleanupShaderModules();
	cleanupPipelineLayout();
	cleanupStaticDescriptorPool();
	cleanupDescriptorSetLayouts();
	cleanupSampler();
	cleanupSwapchain();
	cleanupRenderPass();
	cleanupUtilityResources();
	cleanupSurface();
	cleanupWindow();
	cleanupAllocator();
	cleanupDevice();
	cleanupInstance();
}

void ClientApp::createInstance()
{
	auto appInfo = vka::applicationInfo();
	appInfo.pApplicationName = appName;
	appInfo.pEngineName = engineName;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

	auto createInfo = vka::instanceCreateInfo();
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledLayerCount = gsl::narrow_cast<uint32_t>(instanceLayers.size());
	createInfo.ppEnabledLayerNames = instanceLayers.data();
	createInfo.enabledExtensionCount = gsl::narrow_cast<uint32_t>(instanceExtensions.size());
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();
	vkCreateInstance(&createInfo, nullptr, &vs.instance);

	vka::LoadInstanceLevelEntryPoints(vs.instance);
}

void ClientApp::cleanupInstance()
{
	vkDestroyInstance(vs.instance, nullptr);
}

void ClientApp::selectPhysicalDevice()
{
	auto physicalDevices = vka::GetPhysicalDevices(vs.instance);
	// TODO: error handling if no physical devices found
	vs.physicalDevice = physicalDevices.at(0);

	auto props = vka::GetProperties(vs.physicalDevice);
	if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
	{
		vs.unifiedMemory = true;
	}

	vs.uniformBufferOffsetAlignment = props.limits.minUniformBufferOffsetAlignment;
}

void ClientApp::createDevice()
{
	auto queueSearch = [](
		std::vector<VkQueueFamilyProperties>& properties,
		VkPhysicalDevice physicalDevice,
		VkSurfaceKHR surface,
		VkQueueFlags flags,
		VkBool32 presentSupportNeeded)
	{
		std::optional<uint32_t> result;
		for (uint32_t i = 0; i < properties.size(); ++i)
		{
			const auto& prop = properties[i];
			auto flagMatch = ((flags & prop.queueFlags) == flags);
			if (presentSupportNeeded)
			{
				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
				if (flagMatch && presentSupport)
				{
					result = i;
					return result;
				}
			}
			else
			{
				if (flagMatch)
				{
					result = i;
					return result;
				}
			}
		}
		return result;
	};

	vs.queue.familyProperties = vka::GetQueueFamilyProperties(vs.physicalDevice);

	vs.queue.graphics.familyIndex = queueSearch(
		vs.queue.familyProperties,
		vs.physicalDevice,
		vs.surface,
		VK_QUEUE_GRAPHICS_BIT,
		false).value();

	vs.queue.present.familyIndex = queueSearch(
		vs.queue.familyProperties,
		vs.physicalDevice,
		vs.surface,
		0,
		true).value();

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

	queueCreateInfos.push_back(vka::deviceQueueCreateInfo());
	auto& graphicsQueueCreateInfo = queueCreateInfos.back();
	graphicsQueueCreateInfo.queueFamilyIndex = vs.queue.graphics.familyIndex;
	graphicsQueueCreateInfo.queueCount = 1;
	graphicsQueueCreateInfo.pQueuePriorities = &vs.queue.graphics.priority;

	if (vs.queue.graphics.familyIndex != vs.queue.present.familyIndex)
	{
		queueCreateInfos.push_back(vka::deviceQueueCreateInfo());
		auto& presentQueueCreateInfo = queueCreateInfos.back();
		presentQueueCreateInfo.queueFamilyIndex = vs.queue.present.familyIndex;
		presentQueueCreateInfo.queueCount = 1;
		presentQueueCreateInfo.pQueuePriorities = &vs.queue.present.priority;
	}

	auto deviceCreateInfo = vka::deviceCreateInfo();
	deviceCreateInfo.enabledExtensionCount = gsl::narrow_cast<uint32_t>(deviceExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
	deviceCreateInfo.queueCreateInfoCount = gsl::narrow_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	vkCreateDevice(vs.physicalDevice, &deviceCreateInfo, nullptr, &vs.device);

	vka::LoadDeviceLevelEntryPoints(vs.device);

	vkGetDeviceQueue(vs.device, vs.queue.graphics.familyIndex, 0, &vs.queue.graphics.queue);
	vkGetDeviceQueue(vs.device, vs.queue.present.familyIndex, 0, &vs.queue.present.queue);
}

void ClientApp::createAllocator()
{
	vs.allocator = vka::Allocator(vs.physicalDevice, vs.device);
}

void ClientApp::cleanupAllocator()
{
	vs.allocator = vka::Allocator();
}

void ClientApp::cleanupDevice()
{
	vkDestroyDevice(vs.device, nullptr);
}

void ClientApp::createWindow(const char* title, uint32_t width, uint32_t height)
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(width, height, title, nullptr, nullptr);
}

void ClientApp::cleanupWindow()
{
	glfwDestroyWindow(window);
}

void ClientApp::createUtilityResources()
{
	auto poolCreateInfo = vka::commandPoolCreateInfo();
	poolCreateInfo.queueFamilyIndex = vs.queue.graphics.familyIndex;
	poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	vkCreateCommandPool(vs.device, &poolCreateInfo, nullptr, &vs.utility.pool);

	auto allocateInfo = vka::commandBufferAllocateInfo(vs.utility.pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
	vkAllocateCommandBuffers(vs.device, &allocateInfo, &vs.utility.buffer);

	auto fenceCreateInfo = vka::fenceCreateInfo();
	vkCreateFence(vs.device, &fenceCreateInfo, nullptr, &vs.utility.fence);
}

void ClientApp::cleanupUtilityResources()
{
	vkDestroyFence(vs.device, vs.utility.fence, nullptr);
	vkDestroyCommandPool(vs.device, vs.utility.pool, nullptr);
}

void ClientApp::createSurface()
{
	glfwCreateWindowSurface(vs.instance, window, nullptr, &vs.surface);

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

	vs.surfaceData.supportedFormats = vka::GetSurfaceFormats(vs.physicalDevice, vs.surface);
	vs.surfaceData.swapFormat = chooseSurfaceFormat(vs.surfaceData.supportedFormats, VK_FORMAT_R8G8B8A8_SNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);

	auto choosePresentMode = [](const std::vector<VkPresentModeKHR>& supported, VkPresentModeKHR preferredPresentMode)
	{
		for (const auto& presentMode : supported)
		{
			if (presentMode == preferredPresentMode)
				return presentMode;
		}
		return supported[0];
	};

	vs.surfaceData.supportedPresentModes = vka::GetPresentModes(vs.physicalDevice, vs.surface);
	vs.surfaceData.presentMode = choosePresentMode(vs.surfaceData.supportedPresentModes, VK_PRESENT_MODE_FIFO_KHR);
}

void ClientApp::cleanupSurface()
{
	vkDestroySurfaceKHR(vs.instance, vs.surface, nullptr);
}

void ClientApp::chooseSwapExtent()
{
	auto capabilities = vka::GetSurfaceCapabilities(vs.physicalDevice, vs.surface);
	vs.surfaceData.extent = capabilities.currentExtent;
}

void ClientApp::createRenderPass()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = vs.surfaceData.swapFormat.format;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

	vs.surfaceData.depthFormat = VK_FORMAT_D32_SFLOAT;
	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = vs.surfaceData.depthFormat;
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

	auto createInfo = vka::renderPassCreateInfo();
	createInfo.attachmentCount = gsl::narrow_cast<uint32_t>(attachmentDescriptions.size());
	createInfo.pAttachments = attachmentDescriptions.data();
	createInfo.subpassCount = gsl::narrow_cast<uint32_t>(subpassDescriptions.size());
	createInfo.pSubpasses = subpassDescriptions.data();
	createInfo.dependencyCount = 1;
	createInfo.pDependencies = &dep3Dto2D;
	vkCreateRenderPass(vs.device, &createInfo, nullptr, &vs.renderPass);
}

void ClientApp::cleanupRenderPass()
{
	vkDestroyRenderPass(vs.device, vs.renderPass, nullptr);
}

void ClientApp::createSwapchain()
{
	std::vector<uint32_t> queueFamilyIndices;
	queueFamilyIndices.push_back(vs.queue.graphics.familyIndex);
	VkSharingMode imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vs.queue.graphics.familyIndex == vs.queue.present.familyIndex)
	{
		queueFamilyIndices.push_back(vs.queue.present.familyIndex);
		VkSharingMode imageSharingMode = VK_SHARING_MODE_CONCURRENT;
	}
	auto createInfo = vka::swapchainCreateInfoKHR();
	createInfo.surface = vs.surface;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.imageFormat = vs.surfaceData.swapFormat.format;
	createInfo.imageColorSpace = vs.surfaceData.swapFormat.colorSpace;
	createInfo.imageExtent = vs.surfaceData.extent;
	createInfo.presentMode = vs.surfaceData.presentMode;
	createInfo.minImageCount = BufferCount;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	createInfo.imageSharingMode = imageSharingMode;
	createInfo.queueFamilyIndexCount = gsl::narrow_cast<uint32_t>(queueFamilyIndices.size());
	createInfo.pQueueFamilyIndices = queueFamilyIndices.data();

	vkCreateSwapchainKHR(vs.device, &createInfo, nullptr, &vs.swapchain);

	vs.depthImage = vka::CreateAllocatedImage2D(
		vs.device,
		vs.allocator,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		vs.surfaceData.depthFormat,
		vs.surfaceData.extent.width,
		vs.surfaceData.extent.height,
		vs.queue.graphics.familyIndex,
		VK_IMAGE_ASPECT_DEPTH_BIT);

	auto swapImages = vka::GetSwapImages(vs.device, vs.swapchain);
	for (auto i = 0U; i < BufferCount; ++i)
	{
		auto& fd = vs.frameData[i];
		fd.swapImage = swapImages[i];
		fd.swapView = vka::CreateImageView2D(vs.device, swapImages[i], vs.surfaceData.swapFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);

		std::array<VkImageView, 2> attachments = {
			fd.swapView,
			vs.depthImage.view
		};

		auto framebufferCreateInfo = vka::framebufferCreateInfo();
		framebufferCreateInfo.renderPass = vs.renderPass;
		framebufferCreateInfo.attachmentCount = 2;
		framebufferCreateInfo.pAttachments = attachments.data();
		framebufferCreateInfo.layers = 1;
		framebufferCreateInfo.width = vs.surfaceData.extent.width;
		framebufferCreateInfo.height = vs.surfaceData.extent.height;
		vkCreateFramebuffer(vs.device, &framebufferCreateInfo, nullptr, &fd.framebuffer);
	}
}

void ClientApp::cleanupSwapchain()
{
	vkDeviceWaitIdle(vs.device);
	vkDestroyImageView(vs.device, vs.depthImage.view, nullptr);
	vkDestroyImage(vs.device, vs.depthImage.image, nullptr);
	for (auto i = 0U; i < BufferCount; ++i)
	{
		vkDestroyFramebuffer(vs.device, vs.frameData[i].framebuffer, nullptr);
		vkDestroyImageView(vs.device, vs.frameData[i].swapView, nullptr);
	}
	vkDestroySwapchainKHR(vs.device, vs.swapchain, nullptr);
}

void ClientApp::recreateSwapchain()
{
	cleanupSwapchain();
	chooseSwapExtent();
	createSwapchain();
}

void ClientApp::createSampler()
{
	auto createInfo = vka::samplerCreateInfo();
	createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	vkCreateSampler(vs.device, &createInfo, nullptr, &vs.sampler);
}

void ClientApp::cleanupSampler()
{
	vkDestroySampler(vs.device, vs.sampler, nullptr);
}

void ClientApp::createDescriptorSetLayouts()
{
	auto& samplerBinding = vs.staticLayoutBindings.emplace_back();
	samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerBinding.binding = 0;
	samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	samplerBinding.descriptorCount = 1;
	samplerBinding.pImmutableSamplers = &vs.sampler;

	auto& texturesBinding = vs.staticLayoutBindings.emplace_back();
	texturesBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	texturesBinding.binding = 1;
	texturesBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	texturesBinding.descriptorCount = std::max(paths.images.size(), 1Ui64);

	auto& materialsBinding = vs.staticLayoutBindings.emplace_back();
	materialsBinding.binding = 2;
	materialsBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	materialsBinding.descriptorCount = std::max(materials.size(), 1Ui64);
	materialsBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	auto& cameraBinding = vs.frameLayoutBindings.emplace_back();
	cameraBinding.binding = 0;
	cameraBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	cameraBinding.descriptorCount = 1;
	cameraBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	auto& lightsBinding = vs.frameLayoutBindings.emplace_back();
	lightsBinding.binding = 1;
	lightsBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	lightsBinding.descriptorCount = LightCount;
	lightsBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	auto& instanceBinding = vs.drawLayoutBindings.emplace_back();
	instanceBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	instanceBinding.binding = 0;
	instanceBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	instanceBinding.descriptorCount = 1;

	auto staticLayoutCreateInfo = vka::descriptorSetLayoutCreateInfo(vs.staticLayoutBindings);
	auto frameLayoutCreateInfo = vka::descriptorSetLayoutCreateInfo(vs.frameLayoutBindings);
	auto drawLayoutCreateInfo = vka::descriptorSetLayoutCreateInfo(vs.drawLayoutBindings);

	vkCreateDescriptorSetLayout(vs.device, &staticLayoutCreateInfo, nullptr, &vs.layouts[0]);
	vkCreateDescriptorSetLayout(vs.device, &frameLayoutCreateInfo, nullptr, &vs.layouts[1]);
	vkCreateDescriptorSetLayout(vs.device, &drawLayoutCreateInfo, nullptr, &vs.layouts[2]);
}

void ClientApp::cleanupDescriptorSetLayouts()
{
	for (auto& layout : vs.layouts)
	{
		vkDestroyDescriptorSetLayout(vs.device, layout, nullptr);
	}
}

void ClientApp::createStaticDescriptorPool()
{
	std::vector<VkDescriptorPoolSize> poolSizes;
	poolSizes.emplace_back();
	poolSizes.back().type = VK_DESCRIPTOR_TYPE_SAMPLER;
	poolSizes.back().descriptorCount = 1;
	poolSizes.emplace_back();
	poolSizes.back().type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	poolSizes.back().descriptorCount = std::max(paths.images.size(), 1Ui64);
	poolSizes.emplace_back();
	poolSizes.back().type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes.back().descriptorCount = std::max(materials.size(), 1Ui64);
	auto createInfo = vka::descriptorPoolCreateInfo(poolSizes, 1);
	vkCreateDescriptorPool(vs.device, &createInfo, nullptr, &vs.staticLayoutPool);
}

void ClientApp::cleanupStaticDescriptorPool()
{
	vkDestroyDescriptorPool(vs.device, vs.staticLayoutPool, nullptr);
}

void ClientApp::createPushRanges()
{
	vs.pushRange = vka::pushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(vka::VS::PushConstantData), 0);
}

void ClientApp::createPipelineLayout()
{
	auto createInfo = vka::pipelineLayoutCreateInfo(vs.layouts.data(), gsl::narrow_cast<uint32_t>(vs.layouts.size()));
	createInfo.pushConstantRangeCount = 1;
	createInfo.pPushConstantRanges = &vs.pushRange;
	vkCreatePipelineLayout(vs.device, &createInfo, nullptr, &vs.pipeline.layout);
}

void ClientApp::cleanupPipelineLayout()
{
	vkDestroyPipelineLayout(vs.device, vs.pipeline.layout, nullptr);
}

void ClientApp::createPipelineCommonState()
{
	auto& cmn = vs.pipeline.common;
	vs.specializationData.lightCount = LightCount;
	vs.specializationData.textureCount = paths.images.size();
	vs.specializationData.materialCount = materials.size();
	cmn.specializationMapEntries.materialCount = vka::specializationMapEntry(
		0, 
		offsetof(vka::VS::SpecializationData, materialCount), 
		sizeof(vka::VS::SpecializationData::MaterialCount));
	cmn.specializationMapEntries.textureCount = vka::specializationMapEntry(
		1, 
		offsetof(vka::VS::SpecializationData, textureCount), 
		sizeof(vka::VS::SpecializationData::TextureCount));
	cmn.specializationMapEntries.lightCount = vka::specializationMapEntry(
		2, 
		offsetof(vka::VS::SpecializationData, lightCount), 
		sizeof(vka::VS::SpecializationData::LightCount));
	cmn.specializationInfo = vka::specializationInfo(
		3, 
		&cmn.specializationMapEntries.materialCount, 
		sizeof(vka::VS::SpecializationData), &vs.specializationData);
	cmn.dynamicState = vka::pipelineDynamicStateCreateInfo(
		&vs.pipeline.common.dynamicStates.viewport,
		2);
	cmn.multisampleState = vka::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
	cmn.viewportState = vka::pipelineViewportStateCreateInfo(1, 1);
	cmn.inputAssemblyState = vka::pipelineInputAssemblyStateCreateInfo(
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		0,
		false);
}

void ClientApp::createShader2DModules()
{
	auto vertexBytes = fileIO::readFile(std::string(paths.shader2D.vertex));
	auto vertexCreateInfo = vka::shaderModuleCreateInfo();
	vertexCreateInfo.codeSize = vertexBytes.size();
	vertexCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vertexBytes.data());
	vkCreateShaderModule(vs.device, &vertexCreateInfo, nullptr, &vs.shaders.p2D.vertex);

	auto fragmentBytes = fileIO::readFile(std::string(paths.shader2D.fragment));
	auto fragmentCreateInfo = vka::shaderModuleCreateInfo();
	fragmentCreateInfo.codeSize = fragmentBytes.size();
	fragmentCreateInfo.pCode = reinterpret_cast<const uint32_t*>(fragmentBytes.data());
	vkCreateShaderModule(vs.device, &fragmentCreateInfo, nullptr, &vs.shaders.p2D.fragment);
}

void ClientApp::createShader3DModules()
{
	auto vertexBytes = fileIO::readFile(std::string(paths.shader3D.vertex));
	auto vertexCreateInfo = vka::shaderModuleCreateInfo();
	vertexCreateInfo.codeSize = vertexBytes.size();
	vertexCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vertexBytes.data());
	vkCreateShaderModule(vs.device, &vertexCreateInfo, nullptr, &vs.shaders.p3D.vertex);

	auto fragmentBytes = fileIO::readFile(std::string(paths.shader3D.fragment));
	auto fragmentCreateInfo = vka::shaderModuleCreateInfo();
	fragmentCreateInfo.codeSize = fragmentBytes.size();
	fragmentCreateInfo.pCode = reinterpret_cast<const uint32_t*>(fragmentBytes.data());
	vkCreateShaderModule(vs.device, &fragmentCreateInfo, nullptr, &vs.shaders.p3D.fragment);
}

void ClientApp::cleanupShaderModules()
{
	vkDestroyShaderModule(vs.device, vs.shaders.p3D.fragment, nullptr);
	vkDestroyShaderModule(vs.device, vs.shaders.p3D.vertex, nullptr);
	vkDestroyShaderModule(vs.device, vs.shaders.p2D.fragment, nullptr);
	vkDestroyShaderModule(vs.device, vs.shaders.p2D.vertex, nullptr);
}

void ClientApp::setupPipeline2D()
{
	auto& p2D = vs.pipeline.p2DState;

	auto vertexShaderStage = vka::pipelineShaderStageCreateInfo();
	vertexShaderStage.module = vs.shaders.p2D.vertex;
	vertexShaderStage.pName = "main";
	vertexShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderStage.pSpecializationInfo = &vs.pipeline.common.specializationInfo;
	p2D.shaderStages.push_back(vertexShaderStage);

	auto fragmentShaderStage = vka::pipelineShaderStageCreateInfo();
	fragmentShaderStage.module = vs.shaders.p2D.fragment;
	fragmentShaderStage.pName = "main";
	fragmentShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderStage.pSpecializationInfo = &vs.pipeline.common.specializationInfo;
	p2D.shaderStages.push_back(fragmentShaderStage);

	p2D.colorBlendAttachmentState = vka::pipelineColorBlendAttachmentState(
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT,
		true);
	p2D.colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	p2D.colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	p2D.colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	p2D.colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	p2D.colorBlendState = vka::pipelineColorBlendStateCreateInfo(1, &p2D.colorBlendAttachmentState);

	p2D.rasterizationState = vka::pipelineRasterizationStateCreateInfo(
		VK_POLYGON_MODE_FILL,
		VK_CULL_MODE_NONE,
		VK_FRONT_FACE_COUNTER_CLOCKWISE);

	auto& vertexPositionAttribute = p2D.vertexAttributes.emplace_back();
	vertexPositionAttribute.binding = 0;
	vertexPositionAttribute.location = 0;
	vertexPositionAttribute.offset = 0;
	vertexPositionAttribute.format = VK_FORMAT_R32G32_SFLOAT;

	auto& vertexUVAttribute = p2D.vertexAttributes.emplace_back();
	vertexUVAttribute.binding = 0;
	vertexUVAttribute.location = 1;
	vertexUVAttribute.offset = 8;
	vertexUVAttribute.format = VK_FORMAT_R32G32_SFLOAT;

	auto vertexBindingDescription = vka::vertexInputBindingDescription(
		0,
		16,
		VK_VERTEX_INPUT_RATE_VERTEX);
	p2D.vertexBindings.push_back(vertexBindingDescription);

	p2D.vertexInputState = vka::pipelineVertexInputStateCreateInfo();
	p2D.vertexInputState.vertexAttributeDescriptionCount = p2D.vertexAttributes.size();
	p2D.vertexInputState.pVertexAttributeDescriptions = p2D.vertexAttributes.data();
	p2D.vertexInputState.vertexBindingDescriptionCount = p2D.vertexBindings.size();
	p2D.vertexInputState.pVertexBindingDescriptions = p2D.vertexBindings.data();

	vs.pipeline.p2DCreateInfo = vka::pipelineCreateInfo(vs.pipeline.layout, vs.renderPass);
	vs.pipeline.p2DCreateInfo.stageCount = p2D.shaderStages.size();
	vs.pipeline.p2DCreateInfo.pStages = p2D.shaderStages.data();
	vs.pipeline.p2DCreateInfo.pColorBlendState = &p2D.colorBlendState;
	vs.pipeline.p2DCreateInfo.pDepthStencilState = nullptr;
	vs.pipeline.p2DCreateInfo.pDynamicState = &vs.pipeline.common.dynamicState;
	vs.pipeline.p2DCreateInfo.pMultisampleState = &vs.pipeline.common.multisampleState;
	vs.pipeline.p2DCreateInfo.pRasterizationState = &p2D.rasterizationState;
	vs.pipeline.p2DCreateInfo.pVertexInputState = &p2D.vertexInputState;
	vs.pipeline.p2DCreateInfo.pInputAssemblyState = &vs.pipeline.common.inputAssemblyState;
	vs.pipeline.p2DCreateInfo.pViewportState = &vs.pipeline.common.viewportState;
	vs.pipeline.p2DCreateInfo.subpass = 1;
}

void ClientApp::setupPipeline3D()
{
	auto& p3D = vs.pipeline.p3DState;

	auto vertexShaderStage = vka::pipelineShaderStageCreateInfo();
	vertexShaderStage.module = vs.shaders.p3D.vertex;
	vertexShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderStage.pName = "main";
	vertexShaderStage.pSpecializationInfo = &vs.pipeline.common.specializationInfo;

	auto fragmentShaderStage = vka::pipelineShaderStageCreateInfo();
	fragmentShaderStage.module = vs.shaders.p3D.fragment;
	fragmentShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderStage.pName = "main";
	fragmentShaderStage.pSpecializationInfo = &vs.pipeline.common.specializationInfo;

	p3D.shaderStages.push_back(vertexShaderStage);
	p3D.shaderStages.push_back(fragmentShaderStage);

	p3D.colorBlendAttachmentState = vka::pipelineColorBlendAttachmentState(0, false);
	p3D.colorBlendState = vka::pipelineColorBlendStateCreateInfo(1, &p3D.colorBlendAttachmentState);

	p3D.depthStencilState = vka::pipelineDepthStencilStateCreateInfo(true, true, VK_COMPARE_OP_LESS);

	p3D.rasterizationState = vka::pipelineRasterizationStateCreateInfo(
		VK_POLYGON_MODE_FILL,
		VK_CULL_MODE_BACK_BIT,
		VK_FRONT_FACE_COUNTER_CLOCKWISE);

	auto positionAttribute = vka::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);
	auto normalAttribute = vka::vertexInputAttributeDescription(1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0);
	auto positionBinding = vka::vertexInputBindingDescription(0, 12, VK_VERTEX_INPUT_RATE_VERTEX);
	auto normalBinding = vka::vertexInputBindingDescription(1, 12, VK_VERTEX_INPUT_RATE_VERTEX);

	p3D.vertexAttributes.push_back(positionAttribute);
	p3D.vertexAttributes.push_back(normalAttribute);
	p3D.vertexBindings.push_back(positionBinding);
	p3D.vertexBindings.push_back(normalBinding);
	p3D.vertexInputState = vka::pipelineVertexInputStateCreateInfo();

	p3D.vertexInputState.vertexAttributeDescriptionCount = p3D.vertexAttributes.size();
	p3D.vertexInputState.pVertexAttributeDescriptions = p3D.vertexAttributes.data();
	p3D.vertexInputState.vertexBindingDescriptionCount = p3D.vertexBindings.size();
	p3D.vertexInputState.pVertexBindingDescriptions = p3D.vertexBindings.data();

	vs.pipeline.p3DCreateInfo = vka::pipelineCreateInfo(vs.pipeline.layout, vs.renderPass);
	vs.pipeline.p3DCreateInfo.stageCount = p3D.shaderStages.size();
	vs.pipeline.p3DCreateInfo.pStages = p3D.shaderStages.data();
	vs.pipeline.p3DCreateInfo.pColorBlendState = &p3D.colorBlendState;
	vs.pipeline.p3DCreateInfo.pDepthStencilState = &p3D.depthStencilState;
	vs.pipeline.p3DCreateInfo.pDynamicState = &vs.pipeline.common.dynamicState;
	vs.pipeline.p3DCreateInfo.pMultisampleState = &vs.pipeline.common.multisampleState;
	vs.pipeline.p3DCreateInfo.pRasterizationState = &p3D.rasterizationState;
	vs.pipeline.p3DCreateInfo.pVertexInputState = &p3D.vertexInputState;
	vs.pipeline.p3DCreateInfo.pInputAssemblyState = &vs.pipeline.common.inputAssemblyState;
	vs.pipeline.p3DCreateInfo.pViewportState = &vs.pipeline.common.viewportState;
	vs.pipeline.p3DCreateInfo.subpass = 0;
}

void ClientApp::createPipelines()
{
	vkCreateGraphicsPipelines(vs.device, VK_NULL_HANDLE, 2, &vs.pipeline.p2DCreateInfo, nullptr, &vs.pipeline.p2D);
}

void ClientApp::cleanupPipelines()
{
	vkDestroyPipeline(vs.device, vs.pipeline.p3D, nullptr);
	vkDestroyPipeline(vs.device, vs.pipeline.p2D, nullptr);
}

void ClientApp::loadImages()
{}

void ClientApp::loadQuads()
{}

void ClientApp::loadModels()
{
	auto loadFunc = [&](const entt::HashedString& path) { models[path] = vka::LoadModel(path); };
	loadFunc(Models::Cube::path);
	loadFunc(Models::Cylinder::path);
	loadFunc(Models::IcosphereSub2::path);
	loadFunc(Models::Pentagon::path);
	loadFunc(Models::Triangle::path);
}

auto ClientApp::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties, bool dedicatedAllocation = true)
{
	return vka::CreateAllocatedBuffer(
		vs.device,
		vs.allocator,
		size,
		usage,
		vs.queue.graphics.familyIndex,
		memoryProperties,
		dedicatedAllocation);
};

void ClientApp::createVertexBuffer2D()
{
	auto& p2D = vs.pipeline.p2D;
	VkDeviceSize vertexBufferSize = 0;

	for (const auto& quad : quads)
	{
		auto vertexData = gsl::span(quad.second.vertices);
		vertexBufferSize += vertexData.length_bytes();
	}
	if (!vertexBufferSize)
		return;
	VkMemoryPropertyFlags vertexBufferMemProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	VkMemoryPropertyFlags stagingBufferMemProps =
		(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VkBufferUsageFlags vertexBufferUsageFlags{};
	if (vs.unifiedMemory)
	{
		vertexBufferMemProps |= stagingBufferMemProps;
	}
	else
	{
		vertexBufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}

	vs.vertexBuffers.p2D.vertexBuffer = createBuffer(
		vertexBufferSize,
		vertexBufferUsageFlags |
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		vertexBufferMemProps);

	vka::Buffer vertexHostBuffer{};

	vka::Buffer vertexStagingBuffer{};

	if (vs.unifiedMemory)
	{
		vertexHostBuffer = vs.vertexBuffers.p2D.vertexBuffer;
	}
	else
	{
		vertexStagingBuffer = createBuffer(
			vertexBufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			stagingBufferMemProps);

		vertexHostBuffer = vertexStagingBuffer;
	}

	vka::MapBuffer(vs.device, vertexHostBuffer);

	size_t vertexOffset = 0;
	size_t vertexBufferOffset = 0;
	for (auto& quadPair : quads)
	{
		auto& quad = quadPair.second;
		auto vertexData = gsl::span(quad.vertices);
		memcpy((gsl::byte*)(vertexHostBuffer.mapPtr) + vertexBufferOffset, vertexData.data(), vertexData.length_bytes());

		quad.firstVertex = vertexOffset;

		vertexOffset += vertexData.size();
		vertexBufferOffset += vertexData.length_bytes();
	}

	if (!vs.unifiedMemory)
	{
		auto copyFromStaging = [&](vka::Buffer& source, vka::Buffer& dest)
		{
			VkBufferCopy bufferCopy{};
			bufferCopy.srcOffset = 0;
			bufferCopy.dstOffset = 0;
			bufferCopy.size = source.size;
			vka::CopyBufferToBuffer(
				vs.utility.buffer,
				vs.queue.graphics.queue,
				source.buffer,
				dest.buffer,
				bufferCopy,
				vs.utility.fence,
				gsl::span<VkSemaphore>());
			constexpr size_t maxU64 = ~(0Ui64);
			vkWaitForFences(vs.device, 1, &vs.utility.fence, false, maxU64);
			vkResetFences(vs.device, 1, &vs.utility.fence);
		};
		copyFromStaging(vertexHostBuffer, vs.vertexBuffers.p2D.vertexBuffer);

		vka::DestroyAllocatedBuffer(vs.device, vertexStagingBuffer);
	}
}

void ClientApp::createVertexBuffers3D()
{
	auto& p3D = vs.pipeline.p3D;
	VkDeviceSize positionBufferSize = 0;
	VkDeviceSize normalBufferSize = 0;
	VkDeviceSize indexBufferSize = 0;
	for (const auto& model : models)
	{
		const auto& mesh = model.second.full;

		auto positionData = gsl::span(mesh.positions);
		positionBufferSize += positionData.length_bytes();

		auto normalData = gsl::span(mesh.normals);
		normalBufferSize += normalData.length_bytes();

		auto indexData = gsl::span(mesh.indices);
		indexBufferSize += indexData.length_bytes();
	}
	if (!positionBufferSize || !normalBufferSize || !indexBufferSize)
		return;

	VkMemoryPropertyFlags vertexBufferMemProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	VkMemoryPropertyFlags stagingBufferMemProps =
		(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VkBufferUsageFlags vertexBufferUsageFlags{};
	if (vs.unifiedMemory)
	{
		vertexBufferMemProps |= stagingBufferMemProps;
	}
	else
	{
		vertexBufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}

	vs.vertexBuffers.p3D.positionBuffer = createBuffer(
		positionBufferSize,
		vertexBufferUsageFlags |
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		vertexBufferMemProps);
	vs.vertexBuffers.p3D.normalBuffer = createBuffer(
		normalBufferSize,
		vertexBufferUsageFlags |
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		vertexBufferMemProps);
	vs.vertexBuffers.p3D.indexBuffer = createBuffer(
		indexBufferSize,
		vertexBufferUsageFlags |
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		vertexBufferMemProps);

	vka::Buffer positionHostBuffer{};
	vka::Buffer normalHostBuffer{};
	vka::Buffer indexHostBuffer{};

	vka::Buffer positionStagingBuffer{};
	vka::Buffer normalStagingBuffer{};
	vka::Buffer indexStagingBuffer{};

	if (vs.unifiedMemory)
	{
		positionHostBuffer = vs.vertexBuffers.p3D.positionBuffer;
		normalHostBuffer = vs.vertexBuffers.p3D.normalBuffer;
		indexHostBuffer = vs.vertexBuffers.p3D.indexBuffer;
	}
	else
	{
		positionStagingBuffer = createBuffer(
			positionBufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			stagingBufferMemProps);
		normalStagingBuffer = createBuffer(
			normalBufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			stagingBufferMemProps);
		indexStagingBuffer = createBuffer(
			indexBufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			stagingBufferMemProps);

		positionHostBuffer = positionStagingBuffer;
		normalHostBuffer = normalStagingBuffer;
		indexHostBuffer = indexStagingBuffer;
	}

	vka::MapBuffer(vs.device, positionHostBuffer);
	vka::MapBuffer(vs.device, normalHostBuffer);
	vka::MapBuffer(vs.device, indexHostBuffer);

	size_t vertexOffset = 0;
	size_t indexOffset = 0;
	size_t positionBufferOffset = 0;
	size_t normalBufferOffset = 0;
	size_t indexBufferOffset = 0;
	for (auto& model : models)
	{
		auto& mesh = model.second.full;

		auto positionData = gsl::span(mesh.positions);
		auto normalData = gsl::span(mesh.normals);
		auto indexData = gsl::span(mesh.indices);

		mesh.firstVertex = vertexOffset;
		mesh.firstIndex = indexOffset;

		memcpy((gsl::byte*)(positionHostBuffer.mapPtr) + positionBufferOffset, positionData.data(), positionData.length_bytes());
		memcpy((gsl::byte*)(normalHostBuffer.mapPtr) + normalBufferOffset, normalData.data(), normalData.length_bytes());
		memcpy((gsl::byte*)(indexHostBuffer.mapPtr) + indexBufferOffset, indexData.data(), indexData.length_bytes());

		vertexOffset += positionData.size();
		indexOffset += indexData.size();
		positionBufferOffset += positionData.length_bytes();
		normalBufferOffset += normalData.length_bytes();
		indexBufferOffset += indexData.length_bytes();
	}

	if (!vs.unifiedMemory)
	{
		auto copyFromStaging = [&](vka::Buffer& source, vka::Buffer& dest)
		{
			VkBufferCopy bufferCopy{};
			bufferCopy.srcOffset = 0;
			bufferCopy.dstOffset = 0;
			bufferCopy.size = source.size;
			vka::CopyBufferToBuffer(
				vs.utility.buffer,
				vs.queue.graphics.queue,
				source.buffer,
				dest.buffer,
				bufferCopy,
				vs.utility.fence,
				gsl::span<VkSemaphore>());
			constexpr size_t maxU64 = ~(0Ui64);
			vkWaitForFences(vs.device, 1, &vs.utility.fence, false, maxU64);
			vkResetFences(vs.device, 1, &vs.utility.fence);
		};
		copyFromStaging(positionHostBuffer, vs.vertexBuffers.p3D.positionBuffer);
		copyFromStaging(normalHostBuffer, vs.vertexBuffers.p3D.normalBuffer);
		copyFromStaging(indexHostBuffer, vs.vertexBuffers.p3D.indexBuffer);

		vka::DestroyAllocatedBuffer(vs.device, positionStagingBuffer);
		vka::DestroyAllocatedBuffer(vs.device, normalStagingBuffer);
		vka::DestroyAllocatedBuffer(vs.device, indexStagingBuffer);
	}
}

void ClientApp::cleanupVertexBuffers()
{
	vka::DestroyAllocatedBuffer(vs.device, vs.vertexBuffers.p3D.indexBuffer);
	vka::DestroyAllocatedBuffer(vs.device, vs.vertexBuffers.p3D.normalBuffer);
	vka::DestroyAllocatedBuffer(vs.device, vs.vertexBuffers.p3D.positionBuffer);
	vka::DestroyAllocatedBuffer(vs.device, vs.vertexBuffers.p2D.vertexBuffer);
}

static VkDeviceSize SelectUniformBufferOffset(VkDeviceSize elementSize, VkDeviceSize minimumOffsetAlignment)
{
	if (elementSize <= minimumOffsetAlignment)
		return minimumOffsetAlignment;
	auto mult = (VkDeviceSize)std::ceil((double)elementSize / (double)minimumOffsetAlignment);
	return mult * minimumOffsetAlignment;
}

void ClientApp::createStaticUniformBuffer()
{
	VkBufferUsageFlags uniformBufferUsageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	VkMemoryPropertyFlags uniformBufferMemProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	VkMemoryPropertyFlags stagingBufferMemProps =
		(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (vs.unifiedMemory)
	{
		uniformBufferMemProps |= stagingBufferMemProps;
	}
	else
	{
		uniformBufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}

	auto bufferOffset = SelectUniformBufferOffset(sizeof(Material), vs.uniformBufferOffsetAlignment);
	auto materialBufferSize = bufferOffset * materials.size();

	vs.materials = createBuffer(
		materialBufferSize,
		uniformBufferUsageFlags,
		uniformBufferMemProps,
		true);
}

void ClientApp::cleanupStaticUniformBuffer()
{
	vka::DestroyAllocatedBuffer(vs.device, vs.materials);
}

void ClientApp::createFrameResources()
{
	VkBufferUsageFlags uniformBufferUsageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	VkMemoryPropertyFlags uniformBufferMemProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	VkMemoryPropertyFlags stagingBufferMemProps =
		(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (vs.unifiedMemory)
	{
		uniformBufferMemProps |= stagingBufferMemProps;
	}
	else
	{
		uniformBufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}
	for (auto i = 0U; i < BufferCount; ++i)
	{
		auto& fd = vs.frameData[i];

		fd.uniform.camera = createBuffer(
			sizeof(glm::mat4) * 2,
			uniformBufferUsageFlags,
			uniformBufferMemProps,
			false);

		fd.uniform.lights = createBuffer(
			SelectUniformBufferOffset(sizeof(glm::vec4) * 2, vs.uniformBufferOffsetAlignment) * LightCount,
			uniformBufferUsageFlags,
			uniformBufferMemProps,
			false);

		auto cameraPoolSize = vka::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1);
		auto lightsPoolSize = vka::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, LightCount);
		auto instancePoolSize = vka::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1);
		std::vector<VkDescriptorPoolSize> poolSizes = { cameraPoolSize, lightsPoolSize, instancePoolSize };
		auto descriptorPoolCreateInfo = vka::descriptorPoolCreateInfo(poolSizes, 2);
		vkCreateDescriptorPool(
			vs.device, 
			&descriptorPoolCreateInfo, 
			nullptr, 
			&fd.dynamicDescriptorPool);

		auto commandPoolCreateInfo = vka::commandPoolCreateInfo();
		commandPoolCreateInfo.queueFamilyIndex = vs.queue.graphics.familyIndex;
		vkCreateCommandPool(
			vs.device, 
			&commandPoolCreateInfo, 
			nullptr, 
			&fd.renderCommandPool);

		auto commandBufferAllocateInfo = vka::commandBufferAllocateInfo(
			fd.renderCommandPool, 
			VK_COMMAND_BUFFER_LEVEL_PRIMARY, 
			1);
		vkAllocateCommandBuffers(
			vs.device, 
			&commandBufferAllocateInfo, 
			&fd.renderCommandBuffer);

		auto fenceCreateInfo = vka::fenceCreateInfo();
		vkCreateFence(vs.device, &fenceCreateInfo, nullptr, &vs.imagePresentedFences[i]);
		vs.imagePresentedFencePool.pool(vs.imagePresentedFences[i]);

		auto semaphoreCreateInfo = vka::semaphoreCreateInfo();
		vkCreateSemaphore(vs.device, &semaphoreCreateInfo, nullptr, &fd.imageRenderedSemaphore);
	}
}

void ClientApp::cleanupFrameResources()
{
	for (auto i = 0U; i < BufferCount; ++i)
	{
		auto& fd = vs.frameData[i];
		vkDestroyFence(vs.device, vs.imagePresentedFences[i], nullptr);

		vkDestroySemaphore(vs.device, fd.imageRenderedSemaphore, nullptr);

		vkDestroyCommandPool(vs.device, fd.renderCommandPool, nullptr);

		vkDestroyDescriptorPool(vs.device, fd.dynamicDescriptorPool, nullptr);

		vka::DestroyAllocatedBuffer(vs.device, fd.uniform.camera);
		vka::DestroyAllocatedBuffer(vs.device, fd.uniform.lights);
		vka::DestroyAllocatedBuffer(vs.device, fd.uniform.instance);
	}
}

void ClientApp::initVulkan()
{
	glfwInit();
	createWindow(
		appName,
		DefaultWindowSize.width,
		DefaultWindowSize.height);

	vs.VulkanLibrary = vka::LoadVulkanLibrary();
	vka::LoadExportedEntryPoints(vs.VulkanLibrary);
	vka::LoadGlobalLevelEntryPoints();

	createInstance();

	selectPhysicalDevice();

	createSurface();

	createDevice();

	createUtilityResources();

	createAllocator();

	createRenderPass();

	chooseSwapExtent();

	createSwapchain();

	createSampler();

	createDescriptorSetLayouts();

	createStaticDescriptorPool();

	createPushRanges();

	createPipelineLayout();

	createShader2DModules();

	createShader3DModules();

	createPipelineCommonState();

	setupPipeline2D();

	setupPipeline3D();

	createPipelines();

	createFrameResources();

	loadImages();

	loadQuads();

	loadModels();

	createVertexBuffer2D();

	createVertexBuffers3D();
}

static void PushBackInput(GLFWwindow * window, vka::InputMessage && msg)
{
	auto &app = *GetUserPointer(window);
	msg.time = NowMilliseconds();
	app.is.inputBuffer.pushLast(std::move(msg));
}

static ClientApp * GetUserPointer(GLFWwindow * window)
{
	return reinterpret_cast<ClientApp *>(glfwGetWindowUserPointer(window));
}

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_REPEAT)
		return;
	vka::InputMessage msg;
	msg.signature.code = key;
	msg.signature.action = action;
	PushBackInput(window, std::move(msg));
}

static void CharacterCallback(GLFWwindow* window, unsigned int codepoint)
{
}

static void CursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
	GetUserPointer(window)->is.cursorX = xpos;
	GetUserPointer(window)->is.cursorY = ypos;
}

static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	vka::InputMessage msg;
	msg.signature.code = button;
	msg.signature.action = action;
	PushBackInput(window, std::move(msg));
}

int main()
{
	ClientApp app{};

	//app.is.inputBindMap[vka::MakeSignature(GLFW_KEY_LEFT, GLFW_PRESS)] = Bindings::Left;
	//app.is.inputBindMap[vka::MakeSignature(GLFW_KEY_RIGHT, GLFW_PRESS)] = Bindings::Right;
	//app.is.inputBindMap[vka::MakeSignature(GLFW_KEY_UP, GLFW_PRESS)] = Bindings::Up;
	//app.is.inputBindMap[vka::MakeSignature(GLFW_KEY_DOWN, GLFW_PRESS)] = Bindings::Down;

	//app.is.inputStateMap[Bindings::Left] = vka::MakeAction([]() { std::cout << "Left Pressed.\n"; });
	//app.is.inputStateMap[Bindings::Right] = vka::MakeAction([]() { std::cout << "Right Pressed.\n"; });
	//app.is.inputStateMap[Bindings::Up] = vka::MakeAction([]() { std::cout << "Up Pressed.\n"; });
	//app.is.inputStateMap[Bindings::Down] = vka::MakeAction([]() { std::cout << "Down Pressed.\n"; });

	//using proto = entt::DefaultPrototype;
	//auto player = proto(app.ecs);
	//auto triangle = proto(app.ecs);
	//auto cube = proto(app.ecs);
	//auto cylinder = proto(app.ecs);
	//auto icosphereSub2 = proto(app.ecs);
	//auto pentagon = proto(app.ecs);
	//auto light = proto(app.ecs);

	//triangle.set<Models::Triangle>();
	//triangle.set<cmp::Transform>(glm::mat4(1.f));
	//triangle.set<cmp::Color>(glm::vec4(1.f));

	//cube.set<Models::Cube>();
	//cube.set<cmp::Transform>(glm::mat4(1.f));
	//cube.set<cmp::Color>(glm::vec4(1.f));

	//cylinder.set<Models::Cylinder>();
	//cylinder.set<cmp::Transform>(glm::mat4(1.f));
	//cylinder.set<cmp::Color>(glm::vec4(1.f));

	//icosphereSub2.set<Models::IcosphereSub2>();
	//icosphereSub2.set<cmp::Transform>(glm::mat4(1.f));
	//icosphereSub2.set<cmp::Color>(glm::vec4(1.f));

	//pentagon.set<Models::Pentagon>();
	//pentagon.set<cmp::Transform>(glm::mat4(1.f));
	//pentagon.set<cmp::Color>(glm::vec4(1.f));

	//light.set<cmp::Light>();
	//light.set<cmp::Color>(glm::vec4(1.f));
	//light.set<cmp::Transform>(glm::mat4(1.f));

	//// 3D render views
	//app.ecs.prepare<cmp::Transform, cmp::Color, Models::Cube>();
	//app.ecs.prepare<cmp::Transform, cmp::Color, Models::Cylinder>();
	//app.ecs.prepare<cmp::Transform, cmp::Color, Models::IcosphereSub2>();
	//app.ecs.prepare<cmp::Transform, cmp::Color, Models::Pentagon>();
	//app.ecs.prepare<cmp::Transform, cmp::Color, Models::Triangle>();


	// 3D entities

	app.initVulkan();
	app.cleanup();
}