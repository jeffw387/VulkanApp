
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
constexpr uint32_t MaxLights = 3;

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
}

void ClientApp::createInstance()
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

void ClientApp::selectPhysicalDevice()
{
	auto physicalDevices = vka::GetPhysicalDevices(vs.instance);
	// TODO: error handling if no physical devices found
	vs.physicalDevice = physicalDevices.at(0);

	auto memProps = vka::GetMemoryProperties(vs.physicalDevice);
	auto unifiedProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	for (auto i = 0U; i < memProps.memoryTypeCount; ++i)
	{
		const auto& memType = memProps.memoryTypes[i];
		if (memType.propertyFlags & unifiedProps)
		{
			vs.unifiedMemory = true;
		}
	}
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

void ClientApp::createWindow(const char* title, uint32_t width, uint32_t height)
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(width, height, title, nullptr, nullptr);
}

void ClientApp::createSurface()
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

void ClientApp::chooseSwapExtent()
{
	auto capabilities = vka::GetSurfaceCapabilities(vs.physicalDevice, vs.surface);
	vs.swapExtent = capabilities.currentExtent;
}

void ClientApp::createRenderPass()
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

void ClientApp::createSwapchain(const VkExtent2D& swapExtent)
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

	vs.depthImage = vka::CreateImage2DExtended(
		vs.device,
		vs.allocator,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_FORMAT_D32_SFLOAT,
		swapExtent.width,
		swapExtent.height,
		vs.graphicsQueueFamilyIndex,
		VK_IMAGE_ASPECT_DEPTH_BIT);

	auto swapImages = vka::GetSwapImages(vs.device, vs.swapchain);
	for (auto i = 0U; i < BufferCount; ++i)
	{
		auto& fd = vs.frameData[i];
		fd.swapImage = swapImages[i];
		fd.unique.swapView = vka::CreateImageView2DUnique(vs.device, swapImages[i], vs.surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
		fd.swapView = fd.unique.swapView.get();

		std::array<VkImageView, 2> attachments = {
			fd.swapView,
			vs.depthImage.view
		};

		auto framebufferCreateInfo = vka::framebufferCreateInfo();
		framebufferCreateInfo.renderPass = vs.renderPass;
		framebufferCreateInfo.attachmentCount = 2;
		framebufferCreateInfo.pAttachments = attachments.data();
		framebufferCreateInfo.layers = 1;
		framebufferCreateInfo.width = swapExtent.width;
		framebufferCreateInfo.height = swapExtent.height;
		fd.unique.framebuffer = vka::CreateFramebufferUnique(vs.device, framebufferCreateInfo);
		fd.framebuffer = fd.unique.framebuffer.get();
	}
}

void ClientApp::cleanupSwapchain()
{
	vkDeviceWaitIdle(vs.device);
	for (auto i = 0U; i < BufferCount; ++i)
	{
		auto& fdu = vs.frameData[i].unique;
		fdu.framebuffer.reset();
		fdu.swapView.reset();
	}
	vs.unique.swapchain.reset();
}

void ClientApp::createSampler()
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

void ClientApp::createPipeline2D()
{
	auto& p2D = vs.pipelines.pipeline2D;
	auto& staticBinding0 = p2D.staticBindings.emplace_back();
	staticBinding0.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	staticBinding0.binding = 0;
	staticBinding0.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	staticBinding0.descriptorCount = 1;
	staticBinding0.pImmutableSamplers = &p2D.sampler;
	auto& staticBinding1 = p2D.staticBindings.emplace_back();
	staticBinding1.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	staticBinding1.binding = 1;
	staticBinding1.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	staticBinding1.descriptorCount = paths.images.size();
	auto staticLayoutCreateInfo = vka::descriptorSetLayoutCreateInfo(p2D.staticBindings);
	p2D.unique.staticLayout = vka::CreateDescriptorSetLayoutUnique(vs.device, staticLayoutCreateInfo);
	p2D.staticLayout = p2D.unique.staticLayout.get();

	auto& frameBinding0 = p2D.frameBindings.emplace_back();
	frameBinding0.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	frameBinding0.binding = 0;
	frameBinding0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	frameBinding0.descriptorCount = 1;
	auto frameLayoutCreateInfo = vka::descriptorSetLayoutCreateInfo(p2D.frameBindings);
	p2D.unique.frameLayout = vka::CreateDescriptorSetLayoutUnique(vs.device, frameLayoutCreateInfo);
	p2D.frameLayout = p2D.unique.frameLayout.get();

	auto& modelBinding0 = p2D.materialBindings.emplace_back();
	modelBinding0.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	modelBinding0.binding = 0;
	modelBinding0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	modelBinding0.descriptorCount = 1;
	auto modelLayoutCreateInfo = vka::descriptorSetLayoutCreateInfo(p2D.materialBindings);
	p2D.unique.materialLayout = vka::CreateDescriptorSetLayoutUnique(vs.device, modelLayoutCreateInfo);
	p2D.materialLayout = p2D.unique.materialLayout.get();

	auto& drawBinding0 = p2D.drawBindings.emplace_back();
	drawBinding0.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	drawBinding0.binding = 0;
	drawBinding0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	drawBinding0.descriptorCount = 1;
	auto drawLayoutCreateInfo = vka::descriptorSetLayoutCreateInfo(p2D.drawBindings);
	p2D.unique.drawLayout = vka::CreateDescriptorSetLayoutUnique(vs.device, drawLayoutCreateInfo);
	p2D.drawLayout = p2D.unique.drawLayout.get();

	std::vector<VkDescriptorSetLayout> layouts;
	layouts.push_back(p2D.staticLayout);
	layouts.push_back(p2D.frameLayout);
	layouts.push_back(p2D.materialLayout);
	layouts.push_back(p2D.drawLayout);
	auto pipelineLayoutCreateInfo = vka::pipelineLayoutCreateInfo(layouts.data(), gsl::narrow_cast<uint32_t>(layouts.size()));
	p2D.unique.pipelineLayout = vka::CreatePipelineLayoutUnique(vs.device, pipelineLayoutCreateInfo);
	p2D.pipelineLayout = p2D.unique.pipelineLayout.get();

	auto vertexBytes = fileIO::readFile(std::string(paths.shader2D.vertex));
	auto vertexCreateInfo = vka::shaderModuleCreateInfo();
	vertexCreateInfo.codeSize = vertexBytes.size();
	vertexCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vertexBytes.data());
	p2D.unique.vertex = vka::CreateShaderModuleUnique(vs.device, vertexCreateInfo);
	p2D.vertex = p2D.unique.vertex.get();

	auto fragmentBytes = fileIO::readFile(std::string(paths.shader2D.fragment));
	auto fragmentCreateInfo = vka::shaderModuleCreateInfo();
	fragmentCreateInfo.codeSize = fragmentBytes.size();
	fragmentCreateInfo.pCode = reinterpret_cast<const uint32_t*>(fragmentBytes.data());
	p2D.unique.fragment = vka::CreateShaderModuleUnique(vs.device, fragmentCreateInfo);
	p2D.fragment = p2D.unique.fragment.get();

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

	auto vertexShaderStage = vka::pipelineShaderStageCreateInfo();
	vertexShaderStage.module = p2D.vertex;
	vertexShaderStage.pName = "main";
	vertexShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages.push_back(vertexShaderStage);

	glm::uint32 imageCount = paths.images.size();
	auto fragSpecSize = sizeof(glm::uint32);
	auto imageCountSpecMap = VkSpecializationMapEntry{};
	imageCountSpecMap.constantID = 0;
	imageCountSpecMap.offset = 0;
	imageCountSpecMap.size = fragSpecSize;
	auto fragmentSpecInfo = vka::specializationInfo(1, &imageCountSpecMap, fragSpecSize, &imageCount);

	auto fragmentShaderStage = vka::pipelineShaderStageCreateInfo();
	fragmentShaderStage.module = p2D.fragment;
	fragmentShaderStage.pName = "main";
	fragmentShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderStage.pSpecializationInfo = &fragmentSpecInfo;
	shaderStages.push_back(fragmentShaderStage);

	auto colorBlendAttachmentState = vka::pipelineColorBlendAttachmentState(
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT,
		true);
	colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	auto colorBlendState = vka::pipelineColorBlendStateCreateInfo(1, &colorBlendAttachmentState);

	std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	auto dynamicState = vka::pipelineDynamicStateCreateInfo(dynamicStates);

	auto multisampleState = vka::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

	auto rasterizationState = vka::pipelineRasterizationStateCreateInfo(
		VK_POLYGON_MODE_FILL,
		VK_CULL_MODE_NONE,
		VK_FRONT_FACE_COUNTER_CLOCKWISE);

	std::vector<VkVertexInputAttributeDescription> vertexAttributes;
	auto& vertexPositionAttribute = vertexAttributes.emplace_back();
	vertexPositionAttribute.binding = 0;
	vertexPositionAttribute.location = 0;
	vertexPositionAttribute.offset = 0;
	vertexPositionAttribute.format = VK_FORMAT_R32G32_SFLOAT;

	auto& vertexUVAttribute = vertexAttributes.emplace_back();
	vertexUVAttribute.binding = 0;
	vertexUVAttribute.location = 1;
	vertexUVAttribute.offset = 8;
	vertexUVAttribute.format = VK_FORMAT_R32G32_SFLOAT;

	auto vertexBindingDescription = vka::vertexInputBindingDescription(
		0,
		16,
		VK_VERTEX_INPUT_RATE_VERTEX);

	auto vertexInputState = vka::pipelineVertexInputStateCreateInfo();
	vertexInputState.vertexAttributeDescriptionCount = vertexAttributes.size();
	vertexInputState.pVertexAttributeDescriptions = vertexAttributes.data();
	vertexInputState.vertexBindingDescriptionCount = 1;
	vertexInputState.pVertexBindingDescriptions = &vertexBindingDescription;

	auto inputAssemblyState = vka::pipelineInputAssemblyStateCreateInfo(
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		0,
		false);

	auto viewportState = vka::pipelineViewportStateCreateInfo(1, 1);

	auto pipelineCreateInfo = vka::pipelineCreateInfo(p2D.pipelineLayout, vs.renderPass);
	pipelineCreateInfo.stageCount = shaderStages.size();
	pipelineCreateInfo.pStages = shaderStages.data();
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pDepthStencilState = nullptr;
	pipelineCreateInfo.pDynamicState = &dynamicState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pVertexInputState = &vertexInputState;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.subpass = 0;

	p2D.unique.pipeline = std::move(vka::CreateGraphicsPipelinesUnique(vs.device, pipelineCreateInfo)[0]);
	p2D.pipeline = p2D.unique.pipeline.get();
}

void ClientApp::createPipeline3D()
{
	auto& p3D = vs.pipelines.pipeline3D;

	auto& materialsBinding = p3D.materialBindings.emplace_back();
	materialsBinding.binding = 0;
	materialsBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	materialsBinding.descriptorCount = materials.size();
	materialsBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	auto materialLayoutCreateInfo = vka::descriptorSetLayoutCreateInfo(
		p3D.materialBindings);
	p3D.unique.materialLayout = vka::CreateDescriptorSetLayoutUnique(
		vs.device, 
		materialLayoutCreateInfo);
	p3D.materialLayout = p3D.unique.materialLayout.get();

	auto& cameraBinding = p3D.frameBindings.emplace_back();
	cameraBinding.binding = 0;
	cameraBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	cameraBinding.descriptorCount = 1;
	cameraBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	auto& lightsBinding = p3D.frameBindings.emplace_back();
	lightsBinding.binding = 1;
	lightsBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	lightsBinding.descriptorCount = MaxLights;
	lightsBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	auto frameLayoutCreateInfo = vka::descriptorSetLayoutCreateInfo(
		p3D.frameBindings);
	p3D.unique.frameLayout = vka::CreateDescriptorSetLayoutUnique(
		vs.device, 
		frameLayoutCreateInfo);
	p3D.frameLayout = p3D.unique.frameLayout.get();

	std::vector<VkDescriptorSetLayout> layouts;
	layouts.push_back(p3D.materialLayout);
	layouts.push_back(p3D.frameLayout);
	auto pipelineLayoutCreateInfo = vka::pipelineLayoutCreateInfo(layouts.data(), layouts.size());
	p3D.unique.pipelineLayout = vka::CreatePipelineLayoutUnique(vs.device, pipelineLayoutCreateInfo);
	p3D.pipelineLayout = p3D.unique.pipelineLayout.get();

	auto vertexBytes = fileIO::readFile(std::string(paths.shader3D.vertex));
	auto vertexCreateInfo = vka::shaderModuleCreateInfo();
	vertexCreateInfo.codeSize = vertexBytes.size();
	vertexCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vertexBytes.data());
	p3D.unique.vertex = vka::CreateShaderModuleUnique(vs.device, vertexCreateInfo);
	p3D.vertex = p3D.unique.vertex.get();

	auto fragmentBytes = fileIO::readFile(std::string(paths.shader3D.fragment));
	auto fragmentCreateInfo = vka::shaderModuleCreateInfo();
	fragmentCreateInfo.codeSize = fragmentBytes.size();
	fragmentCreateInfo.pCode = reinterpret_cast<const uint32_t*>(fragmentBytes.data());
	p3D.unique.fragment = vka::CreateShaderModuleUnique(vs.device, fragmentCreateInfo);
	p3D.fragment = p3D.unique.fragment.get();

	size_t vertSpecDataSize = sizeof(uint32_t);
	std::vector<VkSpecializationMapEntry> vertSpecEntries;
	auto& maxLightsMapEntry = vertSpecEntries.emplace_back();
	maxLightsMapEntry.constantID = 0;
	maxLightsMapEntry.offset = 0;
	maxLightsMapEntry.size = sizeof(uint32_t);
	auto vertSpecInfo = vka::specializationInfo(vertSpecEntries.size(), vertSpecEntries.data(), vertSpecDataSize, &MaxLights);
	auto vertexShaderStage = vka::pipelineShaderStageCreateInfo();
	vertexShaderStage.module = p3D.vertex;
	vertexShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderStage.pName = "main";
	vertexShaderStage.pSpecializationInfo = &vertSpecInfo;

	size_t fragSpecDataSize = sizeof(uint32_t) * 2;
	std::vector<VkSpecializationMapEntry> fragSpecEntries;
	fragSpecEntries.push_back(maxLightsMapEntry);
	auto& materialCountMapEntry = fragSpecEntries.emplace_back();
	materialCountMapEntry.constantID = 1;
	materialCountMapEntry.offset = sizeof(uint32_t);
	materialCountMapEntry.size = sizeof(uint32_t);
	struct {
		uint32_t maxLights;
		uint32_t materialCount;
	} fragSpecData;
	fragSpecData.maxLights = MaxLights;
	fragSpecData.materialCount = materials.size();
	auto fragSpecInfo = vka::specializationInfo(fragSpecEntries.size(), fragSpecEntries.data(), fragSpecDataSize, &fragSpecData);
	auto fragmentShaderStage = vka::pipelineShaderStageCreateInfo();
	fragmentShaderStage.module = p3D.fragment;
	fragmentShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderStage.pName = "main";
	fragmentShaderStage.pSpecializationInfo = &fragSpecInfo;

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	shaderStages.push_back(vertexShaderStage);
	shaderStages.push_back(fragmentShaderStage);

	auto colorBlendAttachmentState = vka::pipelineColorBlendAttachmentState(0, false);
	auto colorBlendState = vka::pipelineColorBlendStateCreateInfo(1, &colorBlendAttachmentState);

	auto depthStencilState = vka::pipelineDepthStencilStateCreateInfo(true, true, VK_COMPARE_OP_LESS);

	std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	auto dynamicState = vka::pipelineDynamicStateCreateInfo(dynamicStates);

	auto multisampleState = vka::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

	auto rasterizationState = vka::pipelineRasterizationStateCreateInfo(
		VK_POLYGON_MODE_FILL,
		VK_CULL_MODE_BACK_BIT,
		VK_FRONT_FACE_COUNTER_CLOCKWISE);

	auto positionAttribute = vka::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);
	auto normalAttribute = vka::vertexInputAttributeDescription(1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0);
	std::vector<VkVertexInputAttributeDescription> vertexAttributes = { positionAttribute, normalAttribute };
	auto positionBinding = vka::vertexInputBindingDescription(0, 12, VK_VERTEX_INPUT_RATE_VERTEX);
	auto normalBinding = vka::vertexInputBindingDescription(1, 12, VK_VERTEX_INPUT_RATE_VERTEX);
	std::vector<VkVertexInputBindingDescription> vertexBindings = { positionBinding, normalBinding };
	auto vertexInputState = vka::pipelineVertexInputStateCreateInfo();
	vertexInputState.vertexAttributeDescriptionCount = vertexAttributes.size();
	vertexInputState.pVertexAttributeDescriptions = vertexAttributes.data();
	vertexInputState.vertexBindingDescriptionCount = vertexBindings.size();
	vertexInputState.pVertexBindingDescriptions = vertexBindings.data();

	auto inputAssemblyState = vka::pipelineInputAssemblyStateCreateInfo(
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		0,
		false);

	auto viewportState = vka::pipelineViewportStateCreateInfo(1, 1);

	auto pipelineCreateInfo = vka::pipelineCreateInfo(p3D.pipelineLayout, vs.renderPass);
	pipelineCreateInfo.stageCount = shaderStages.size();
	pipelineCreateInfo.pStages = shaderStages.data();
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.pDynamicState = &dynamicState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pVertexInputState = &vertexInputState;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.subpass = 1;

	p3D.unique.pipeline = std::move(vka::CreateGraphicsPipelinesUnique(vs.device, pipelineCreateInfo)[0]);
	p3D.pipeline = p3D.unique.pipeline.get();
}

void ClientApp::loadModels()
{
	auto loadFunc = [&](const entt::HashedString& path) { models[path] = vka::LoadModel(path); };
	loadFunc(Models::Cube::path);
	loadFunc(Models::Cylinder::path);
	loadFunc(Models::IcosphereSub2::path);
	loadFunc(Models::Pentagon::path);
	loadFunc(Models::Triangle::path);
}

void ClientApp::createVertexBuffers()
{

}

void ClientApp::initVulkan()
{
	createWindow(appName, DefaultWindowSize.width, DefaultWindowSize.height);

	vs.VulkanLibrary = vka::LoadVulkanLibrary();
	vka::LoadExportedEntryPoints(vs.VulkanLibrary);
	vka::LoadGlobalLevelEntryPoints();

	createInstance();

	selectPhysicalDevice();

	createSurface();

	createDevice();

	createRenderPass();

	createSwapchain(DefaultWindowSize);

	createSampler();

	createPipeline2D();

	createPipeline3D();

	loadModels();

	createVertexBuffers();
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
	ClientApp app;

	app.is.inputBindMap[vka::MakeSignature(GLFW_KEY_LEFT, GLFW_PRESS)] = Bindings::Left;
	app.is.inputBindMap[vka::MakeSignature(GLFW_KEY_RIGHT, GLFW_PRESS)] = Bindings::Right;
	app.is.inputBindMap[vka::MakeSignature(GLFW_KEY_UP, GLFW_PRESS)] = Bindings::Up;
	app.is.inputBindMap[vka::MakeSignature(GLFW_KEY_DOWN, GLFW_PRESS)] = Bindings::Down;

	app.is.inputStateMap[Bindings::Left] = vka::MakeAction([]() { std::cout << "Left Pressed.\n"; });
	app.is.inputStateMap[Bindings::Right] = vka::MakeAction([]() { std::cout << "Right Pressed.\n"; });
	app.is.inputStateMap[Bindings::Up] = vka::MakeAction([]() { std::cout << "Up Pressed.\n"; });
	app.is.inputStateMap[Bindings::Down] = vka::MakeAction([]() { std::cout << "Down Pressed.\n"; });

	using proto = entt::DefaultPrototype;
	auto player = proto(app.ecs);
	auto triangle = proto(app.ecs);
	auto cube = proto(app.ecs);
	auto cylinder = proto(app.ecs);
	auto icosphereSub2 = proto(app.ecs);
	auto pentagon = proto(app.ecs);
	auto light = proto(app.ecs);

	triangle.set<Models::Triangle>();
	triangle.set<cmp::Transform>(glm::mat4(1.f));
	triangle.set<cmp::Color>(glm::vec4(1.f));

	cube.set<Models::Cube>();
	cube.set<cmp::Transform>(glm::mat4(1.f));
	cube.set<cmp::Color>(glm::vec4(1.f));

	cylinder.set<Models::Cylinder>();
	cylinder.set<cmp::Transform>(glm::mat4(1.f));
	cylinder.set<cmp::Color>(glm::vec4(1.f));

	icosphereSub2.set<Models::IcosphereSub2>();
	icosphereSub2.set<cmp::Transform>(glm::mat4(1.f));
	icosphereSub2.set<cmp::Color>(glm::vec4(1.f));

	pentagon.set<Models::Pentagon>();
	pentagon.set<cmp::Transform>(glm::mat4(1.f));
	pentagon.set<cmp::Color>(glm::vec4(1.f));

	light.set<cmp::Light>();
	light.set<cmp::Color>(glm::vec4(1.f));
	light.set<cmp::Transform>(glm::mat4(1.f));

	// 3D render views
	app.ecs.prepare<cmp::Transform, cmp::Color, Models::Cube>();
	app.ecs.prepare<cmp::Transform, cmp::Color, Models::Cylinder>();
	app.ecs.prepare<cmp::Transform, cmp::Color, Models::IcosphereSub2>();
	app.ecs.prepare<cmp::Transform, cmp::Color, Models::Pentagon>();
	app.ecs.prepare<cmp::Transform, cmp::Color, Models::Triangle>();


	// 3D entities

	app.initVulkan();
}