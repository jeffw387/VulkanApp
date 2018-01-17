#define VMA_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "VulkanApp2.hpp"

inline Device::Device(const vk::Instance& instance, 
	const Window & window, 
	const std::vector<const char*> deviceExtensions, 
	const ShaderData & shaderData,
	std::function<void(VulkanApp*)> imageLoadCallback,
	VulkanApp* app)
{
	// select physical device
	auto devices = instance.enumeratePhysicalDevices();
	m_PhysicalDevice = devices[0];

	// find graphics queue
	auto gpuQueueProperties = m_PhysicalDevice.getQueueFamilyProperties();
	auto graphicsQueueInfo = std::find_if(gpuQueueProperties.begin(), gpuQueueProperties.end(),
		[&](vk::QueueFamilyProperties q)
	{
		return q.queueFlags & vk::QueueFlagBits::eGraphics;
	});

	// this gets the position of the queue, which is also its family ID
	m_GraphicsQueueFamilyID = static_cast<uint32_t>(graphicsQueueInfo - gpuQueueProperties.begin());
	// priority of 0 since we only have one queue
	auto graphicsPriority = 0.f;
	auto graphicsQueueCreateInfo = vk::DeviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), m_GraphicsQueueFamilyID, 1U, &graphicsPriority);

	// create Logical Device
	auto gpuFeatures = m_PhysicalDevice.getFeatures();
	m_LogicalDevice = m_PhysicalDevice.createDeviceUnique(
		vk::DeviceCreateInfo(vk::DeviceCreateFlags(),
			1,
			&graphicsQueueCreateInfo,
			0,
			nullptr,
			static_cast<uint32_t>(deviceExtensions.size()),
			deviceExtensions.data(),
			&gpuFeatures)
	);

	

	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = m_PhysicalDevice;
	allocatorInfo.device = m_LogicalDevice.get();
	allocatorInfo.frameInUseCount = 1U;
	VmaAllocator alloc;
	vmaCreateAllocator(&allocatorInfo, &alloc);
	m_Allocator = alloc;

	// get the queue from the logical device
	m_GraphicsQueue = m_LogicalDevice->getQueue(m_GraphicsQueueFamilyID, 0);

	// create surface
	m_Surface = instance.createWin32SurfaceKHRUnique(
		vk::Win32SurfaceCreateInfoKHR(
			vk::Win32SurfaceCreateFlagsKHR(),
			window.getHINSTANCE(),
			window.getHWND()
		));

	// choose surface format from supported list
	auto surfaceFormats = m_PhysicalDevice.getSurfaceFormatsKHR(m_Surface.get());
	if (surfaceFormats.size() == 1 && surfaceFormats[0].format == vk::Format::eUndefined)
		m_SurfaceFormat = vk::Format::eB8G8R8A8Unorm;
	else
		m_SurfaceFormat = surfaceFormats[0].format;
	// get color space
	m_SurfaceColorSpace = surfaceFormats[0].colorSpace;

	auto formatProperties = m_PhysicalDevice.getFormatProperties(vk::Format::eR8G8B8A8Unorm);
	auto surfaceCapabilities = m_PhysicalDevice.getSurfaceCapabilitiesKHR(m_Surface.get());
	auto surfacePresentModes = m_PhysicalDevice.getSurfacePresentModesKHR(m_Surface.get());

	if (!(surfaceCapabilities.currentExtent.width == -1 || surfaceCapabilities.currentExtent.height == -1)) {
		m_SurfaceExtent = surfaceCapabilities.currentExtent;
	}
	else
	{
		auto size = window.GetClientSize();
		m_SurfaceExtent = { static_cast<uint32_t>(size.width), static_cast<uint32_t>(size.height) };
	}

	// create render pass
	auto colorAttachmentDescription = vk::AttachmentDescription(
		vk::AttachmentDescriptionFlags(),
		m_SurfaceFormat,
		vk::SampleCountFlagBits::e1,
		vk::AttachmentLoadOp::eClear,
		vk::AttachmentStoreOp::eStore,
		vk::AttachmentLoadOp::eDontCare,
		vk::AttachmentStoreOp::eDontCare,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::ePresentSrcKHR
	);
	auto colorAttachmentRef = vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal);

	auto subpassDescription = vk::SubpassDescription(
		vk::SubpassDescriptionFlags(),
		vk::PipelineBindPoint::eGraphics,
		0,
		nullptr,
		1U,
		&colorAttachmentRef,
		nullptr,
		nullptr,
		0,
		nullptr
	);

	std::vector<vk::SubpassDependency> dependencies =
	{
		vk::SubpassDependency(
			0U,
			0U,
			vk::PipelineStageFlagBits::eBottomOfPipe,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::AccessFlagBits::eMemoryRead,
			vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
			vk::DependencyFlagBits::eByRegion
		),
		vk::SubpassDependency(
			0U,
			0U,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eBottomOfPipe,
			vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
			vk::AccessFlagBits::eMemoryRead,
			vk::DependencyFlagBits::eByRegion
		)
	};

	auto renderPassInfo = vk::RenderPassCreateInfo(
		vk::RenderPassCreateFlags(),
		1U,
		&colorAttachmentDescription,
		1U,
		&subpassDescription,
		dependencies.size(),
		dependencies.data());
	m_RenderPass = m_LogicalDevice.get().createRenderPassUnique(renderPassInfo);

	// create shader modules
	m_VertexShader = createShaderModule(m_LogicalDevice.get(), shaderData.vertexShaderPath);
	m_FragmentShader = createShaderModule(m_LogicalDevice.get(), shaderData.fragmentShaderPath);

	// default present mode: immediate
	m_PresentMode = vk::PresentModeKHR::eImmediate;

	// use mailbox present mode if supported
	auto mailboxPresentMode = std::find(surfacePresentModes.begin(), surfacePresentModes.end(), vk::PresentModeKHR::eMailbox);
	if (mailboxPresentMode != surfacePresentModes.end())
		m_PresentMode = *mailboxPresentMode;

	m_SwapchainDependencies = SwapChainDependencies(
		m_PhysicalDevice,
		m_LogicalDevice.get(),
		m_Surface.get(),
		m_RenderPass.get(),
		m_SurfaceFormat,
		m_SurfaceColorSpace,
		m_SurfaceExtent,
		m_GraphicsQueueFamilyID,
		m_PresentMode,
		shaderData,
		m_VertexShader.get(),
		m_FragmentShader.get());

	m_FramebufferSupports.resize(m_SwapchainDependencies.swapImageCount);
	for (auto& support : m_FramebufferSupports)
	{
		support.m_CommandPool = m_LogicalDevice->createCommandPoolUnique(
			vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlags(), m_GraphicsQueueFamilyID));
		support.m_CommandBuffer = std::move(m_LogicalDevice->allocateCommandBuffersUnique(
			vk::CommandBufferAllocateInfo(support.m_CommandPool.get(),
				vk::CommandBufferLevel::ePrimary,
				1U))[0]);
		support.m_VertexLayoutDescriptorPool = createVertexDescriptorPool(
			m_LogicalDevice.get(), 
			m_SwapchainDependencies.swapImageCount);
		support.m_BufferCapacity = 0U;
		support.m_ImagePresentCompleteSemaphore = m_LogicalDevice->createSemaphoreUnique(
			vk::SemaphoreCreateInfo(
				vk::SemaphoreCreateFlags()));
		support.m_ImageRenderCompleteSemaphore = m_LogicalDevice->createSemaphoreUnique(
			vk::SemaphoreCreateInfo(
				vk::SemaphoreCreateFlags()));
		support.m_MatrixBufferStagingCompleteSemaphore = m_LogicalDevice->createSemaphoreUnique(
			vk::SemaphoreCreateInfo(
				vk::SemaphoreCreateFlags()));
		support.m_ImagePresentCompleteFence = m_LogicalDevice->createFenceUnique(
			vk::FenceCreateInfo(
				vk::FenceCreateFlagBits::eSignaled));
	}
}


inline SwapChainDependencies::SwapChainDependencies(SwapChainDependencies && other)
{
	m_Swapchain = std::move(other.m_Swapchain);
	m_SwapImages = std::move(other.m_SwapImages);
	m_SwapViews = std::move(other.m_SwapViews);
	m_Framebuffers = std::move(other.m_Framebuffers);
	m_PipelineLayout = std::move(other.m_PipelineLayout);
	m_Pipeline = std::move(other.m_Pipeline);
}

inline SwapChainDependencies::SwapChainDependencies(const vk::PhysicalDevice & physicalDevice, 
	const vk::Device & logicalDevice, 
	const vk::SurfaceKHR & surface, 
	const vk::RenderPass & renderPass, 
	const vk::Format & surfaceFormat, 
	const vk::ColorSpaceKHR & surfaceColorSpace, 
	const vk::Extent2D & surfaceExtent, 
	const uint32_t graphicsQueueFamilyID, 
	const vk::PresentModeKHR presentMode, 
	const ShaderData & shaderData, 
	const vk::ShaderModule vertexShader, 
	const vk::ShaderModule fragmentShader)
{
	m_Swapchain = logicalDevice.createSwapchainKHRUnique(
		vk::SwapchainCreateInfoKHR(
			vk::SwapchainCreateFlagsKHR(),
			surface,
			swapImageCount,
			surfaceFormat,
			surfaceColorSpace,
			surfaceExtent,
			1U,
			vk::ImageUsageFlagBits::eColorAttachment,
			vk::SharingMode::eExclusive,
			1U,
			&graphicsQueueFamilyID,
			vk::SurfaceTransformFlagBitsKHR::eIdentity,
			vk::CompositeAlphaFlagBitsKHR::eOpaque,
			presentMode,
			0U));

	m_SwapImages = logicalDevice.getSwapchainImagesKHR(m_Swapchain.get());
	for (auto i = 0U; i < swapImageCount; i++)
	{
		// create swap image view
		auto viewInfo = vk::ImageViewCreateInfo(
			vk::ImageViewCreateFlags(),
			m_SwapImages[i],
			vk::ImageViewType::e2D,
			surfaceFormat,
			vk::ComponentMapping(),
			vk::ImageSubresourceRange(
				vk::ImageAspectFlagBits::eColor,
				0U,
				1U,
				0U,
				1U));
		m_SwapViews[i] = logicalDevice.createImageViewUnique(viewInfo);

		// create framebuffer
		auto fbInfo = vk::FramebufferCreateInfo(
			vk::FramebufferCreateFlags(),
			renderPass,
			1U,
			&m_SwapViews[i].get(),
			surfaceExtent.width,
			surfaceExtent.height,
			1U);
		m_Framebuffers[i] = logicalDevice.createFramebufferUnique(fbInfo);
	}

	auto descriptorSetLayouts = std::vector<vk::DescriptorSetLayout>
	{
		shaderData.vertexDescriptorSetLayout,
		shaderData.fragmentDescriptorSetLayout
	};

	auto pipelineLayoutInfo = vk::PipelineLayoutCreateInfo(
		vk::PipelineLayoutCreateFlags(),
		descriptorSetLayouts.size(),
		descriptorSetLayouts.data(),
		shaderData.pushConstantRanges.size(),
		shaderData.pushConstantRanges.data());

	m_PipelineLayout = logicalDevice.createPipelineLayoutUnique(pipelineLayoutInfo);

	auto vertexSpecializationInfo = vk::SpecializationInfo(
		shaderData.vertexSpecializationConstants.size(),
		shaderData.vertexSpecializationConstants.data());

	auto vertexStageInfo = vk::PipelineShaderStageCreateInfo(
		vk::PipelineShaderStageCreateFlags(),
		vk::ShaderStageFlagBits::eVertex,
		vertexShader,
		"main",
		&vertexSpecializationInfo);

	auto fragmentSpecializationInfo = vk::SpecializationInfo(
		shaderData.fragmentSpecializationConstants.size(),
		shaderData.fragmentSpecializationConstants.data());

	auto fragmentStageInfo = vk::PipelineShaderStageCreateInfo(
		vk::PipelineShaderStageCreateFlags(),
		vk::ShaderStageFlagBits::eFragment,
		fragmentShader,
		"main",
		&fragmentSpecializationInfo);

	auto shaderStages = std::vector<vk::PipelineShaderStageCreateInfo>{ vertexStageInfo, fragmentStageInfo };

	auto vertexInputInfo = vk::PipelineVertexInputStateCreateInfo(
		vk::PipelineVertexInputStateCreateFlags(),
		shaderData.vertexBindingDescriptions.size(),
		shaderData.vertexBindingDescriptions.data(),
		shaderData.vertexAttributeDescriptions.size(),
		shaderData.vertexAttributeDescriptions.data());

	auto inputAssemblyStateInfo = vk::PipelineInputAssemblyStateCreateInfo(
		vk::PipelineInputAssemblyStateCreateFlags(),
		vk::PrimitiveTopology::eTriangleList);

	auto tesselationState = vk::PipelineTessellationStateCreateInfo(
		vk::PipelineTessellationStateCreateFlags());

	auto viewportStateInfo = vk::PipelineViewportStateCreateInfo(
		vk::PipelineViewportStateCreateFlags(),
		1U,
		nullptr,
		1U,
		nullptr);

	auto rasterizationStateInfo = vk::PipelineRasterizationStateCreateInfo(
		vk::PipelineRasterizationStateCreateFlags(),
		0U,
		0U,
		vk::PolygonMode::eFill,
		vk::CullModeFlagBits::eNone,
		vk::FrontFace::eCounterClockwise,
		0U,
		0.f,
		0.f,
		0.f);

	auto multisampleStateInfo = vk::PipelineMultisampleStateCreateInfo();

	auto depthStencilInfo = vk::PipelineDepthStencilStateCreateInfo();

	auto colorAttachment = vk::PipelineColorBlendAttachmentState(
		1U,
		vk::BlendFactor::eSrcAlpha,
		vk::BlendFactor::eOneMinusSrcAlpha,
		vk::BlendOp::eAdd,
		vk::BlendFactor::eZero,
		vk::BlendFactor::eZero,
		vk::BlendOp::eAdd,
		vk::ColorComponentFlagBits::eR |
		vk::ColorComponentFlagBits::eG |
		vk::ColorComponentFlagBits::eB |
		vk::ColorComponentFlagBits::eA);

	auto blendStateInfo = vk::PipelineColorBlendStateCreateInfo(
		vk::PipelineColorBlendStateCreateFlags(),
		0U,
		vk::LogicOp::eClear,
		1U,
		&colorAttachment);

	auto dynamicStates = std::vector<vk::DynamicState>{ vk::DynamicState::eViewport, vk::DynamicState::eScissor };

	auto dynamicStateInfo = vk::PipelineDynamicStateCreateInfo(
		vk::PipelineDynamicStateCreateFlags(),
		dynamicStates.size(),
		dynamicStates.data());

	auto pipelineInfo = vk::GraphicsPipelineCreateInfo(
		vk::PipelineCreateFlags(),
		shaderStages.size(),
		shaderStages.data(),
		&vertexInputInfo,
		&inputAssemblyStateInfo,
		&tesselationState,
		&viewportStateInfo,
		&rasterizationStateInfo,
		&multisampleStateInfo,
		&depthStencilInfo,
		&blendStateInfo,
		&dynamicStateInfo,
		m_PipelineLayout.get(),
		renderPass
	);
}

inline VulkanApp::VulkanApp(LPTSTR WindowClassName, 
	LPTSTR WindowTitle, 
	Window::WindowStyle windowStyle, 
	int width, int height, 
	const vk::InstanceCreateInfo & instanceCreateInfo, 
	const std::vector<const char*>& deviceExtensions, 
	const ShaderData & shaderData, 
	std::function<void(VulkanApp*)> imageLoadCallback) :
	m_Instance(vk::createInstanceUnique(instanceCreateInfo)),
	m_Window(WindowClassName, WindowTitle, windowStyle, this, width, height),
	m_Device(m_Instance.get(), m_Window, deviceExtensions, shaderData, imageLoadCallback, this)
{}

inline uint32_t VulkanApp::createTexture(const Bitmap & bitmap)
{
	uint32_t textureIndex = static_cast<uint32_t>(m_Textures.size());
	// create vke::Image2D
	auto& image2D = m_Images.emplace_back();
	auto& imageAlloc = m_ImageAllocations.emplace_back();
	auto& imageAllocInfo = m_ImageAllocationInfos.emplace_back();

	auto imageInfo = vk::ImageCreateInfo(
		vk::ImageCreateFlags(),
		vk::ImageType::e2D,
		vk::Format::eR8G8B8A8Srgb,
		vk::Extent3D(bitmap.m_Width, bitmap.m_Height, 1U),
		1U,
		1U,
		vk::SampleCountFlagBits::e1,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
		vk::SharingMode::eExclusive,
		1U,
		&m_Device.m_GraphicsQueueFamilyID,
		vk::ImageLayout::eUndefined);

	auto imageAllocCreateInfo = VmaAllocationCreateInfo {};
	imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	VkImage image;
	VmaAllocation imageAllocation;
	VmaAllocationInfo imageAllocInfo;
	vmaCreateImage(m_Device.m_Allocator, 
		&imageInfo.operator const VkImageCreateInfo &(), 
		&imageAllocCreateInfo, 
		&image, 
		&imageAllocation, 
		&imageAllocInfo);

	float left = static_cast<float>(bitmap.m_Left);
	float top = static_cast<float>(bitmap.m_Top);
	float width = static_cast<float>(bitmap.m_Width);
	float height = static_cast<float>(bitmap.m_Height);

	float right = left + width;
	float bottom = top - height;

	glm::vec2 LeftTopPos, LeftBottomPos, RightBottomPos, RightTopPos;
	LeftTopPos =		{ left,		top		};
	LeftBottomPos =		{ left,		bottom  };
	RightBottomPos =	{ right,	bottom  };
	RightTopPos =		{ right,	top		};

	glm::vec2 LeftTopUV, LeftBottomUV, RightBottomUV, RightTopUV;
	LeftTopUV = { 0.0f, 0.0f };
	LeftBottomUV = { 0.0f, 1.0f };
	RightBottomUV = { 1.0f, 1.0f };
	RightTopUV = { 1.0f, 0.0f };

	VulkanData::Vertex LeftTop, LeftBottom, RightBottom, RightTop;
	LeftTop = { LeftTopPos,		LeftTopUV };
	LeftBottom = { LeftBottomPos,	LeftBottomUV };
	RightBottom = { RightBottomPos,	RightBottomUV };
	RightTop = { RightTopPos,		RightTopUV };

	// push back vertices
	m_VertexData.push_back(LeftTop);
	m_VertexData.push_back(LeftBottom);
	m_VertexData.push_back(RightBottom);
	m_VertexData.push_back(RightTop);

	auto verticesPerTexture = 4U;
	uint32_t indexOffset = verticesPerTexture * textureIndex;

	// push back indices
	m_IndexData.push_back(indexOffset + 0);
	m_IndexData.push_back(indexOffset + 1);
	m_IndexData.push_back(indexOffset + 2);
	m_IndexData.push_back(indexOffset + 2);
	m_IndexData.push_back(indexOffset + 3);
	m_IndexData.push_back(indexOffset + 0);

	// Create a Texture2D object for this texture
	Texture2D texture = {};
	texture.index = static_cast<uint32_t>(m_Textures.size());
	texture.width = bitmap.m_Width;
	texture.height = bitmap.m_Height;
	texture.size = m_Images.back().getMemoryRange().totalSize;
	texture.imageHandle = m_Images.back().getImage();

	// add the texture to a vector and a map by name
	m_Textures.push_back(texture);

	vke::Buffer stagingBuffer;
	stagingBuffer.init(
		m_Device,
		&m_Allocator,
		texture.size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	if (stagingBuffer.mapMemoryRange() != true)
	{
		throw std::runtime_error("unable to map buffer");
	}

	memcpy(stagingBuffer.mappedData, bitmap.m_Data.data(), static_cast<size_t>(bitmap.m_Size));
	stagingBuffer.unmap();

	m_Images.back().copyFromBuffer(stagingBuffer, m_GraphicsCommandPool, m_GraphicsQueue);

	stagingBuffer.cleanup();

	return texture.index;
}

inline VMAWrapper::~VMAWrapper()
{
	if (allocator)
		vmaDestroyAllocator(allocator);
}

inline VMAWrapper::operator VmaAllocator()
{
	return allocator;
}

inline std::vector<vk::VertexInputBindingDescription> Vertex::vertexBindingDescriptions()
{
	auto binding0 = vk::VertexInputBindingDescription(0U, sizeof(Vertex), vk::VertexInputRate::eVertex);
}

inline std::vector<vk::VertexInputAttributeDescription> Vertex::vertexAttributeDescriptions()
{
	// Binding both attributes to one buffer, interleaving Position and UV
	auto attrib0 = vk::VertexInputAttributeDescription(0U, 0U, vk::Format::eR32G32Sfloat, offsetof(Vertex, Position));
	auto attrib1 = vk::VertexInputAttributeDescription(1U, 0U, vk::Format::eR32G32Sfloat, offsetof(Vertex, UV));
}

inline vk::UniqueShaderModule Device::createShaderModule(const vk::Device& device, const char* path)
{
	// read from file
	auto shaderBinary = fileIO::readFile(path);
	// convert to aligned data
	std::vector<uint32_t> codeAligned(shaderBinary.size() / sizeof(uint32_t) + 1);
	memcpy(codeAligned.data(), shaderBinary.data(), shaderBinary.size());
	// create shader module
	auto shaderInfo = vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(),
		codeAligned.size(),
		codeAligned.data());
	return device.createShaderModuleUnique(shaderInfo);
}

inline vk::UniqueDescriptorPool Device::createVertexDescriptorPool(const vk::Device& logicalDevice, uint32_t bufferCount)
{
	auto poolSizes = std::vector<vk::DescriptorPoolSize>
	{
		vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1U)
	};

	return logicalDevice.createDescriptorPoolUnique(
		vk::DescriptorPoolCreateInfo(
			vk::DescriptorPoolCreateFlags(),
			bufferCount,
			poolSizes.size(),
			poolSizes.data()));
}

inline vk::UniqueDescriptorPool Device::createFragmentDescriptorPool(const vk::Device& logicalDevice, uint32_t textureCount)
{
	auto poolSizes = std::vector<vk::DescriptorPoolSize>
	{
		vk::DescriptorPoolSize(vk::DescriptorType::eSampler, 1U),
		vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, textureCount)
	};

	return logicalDevice.createDescriptorPoolUnique(
		vk::DescriptorPoolCreateInfo(
			vk::DescriptorPoolCreateFlags(),
			1U,
			poolSizes.size(),
			poolSizes.data()));
}

inline vk::UniqueDescriptorSetLayout Device::createVertexSetLayout(const vk::Device& logicalDevice)
{
	auto vertexLayoutBindings = std::vector<vk::DescriptorSetLayoutBinding>
	{
		// Vertex: matrix uniform buffer (dynamic)
		vk::DescriptorSetLayoutBinding(
			0U,
			vk::DescriptorType::eUniformBufferDynamic,
			1U,
			vk::ShaderStageFlagBits::eVertex)
	};

	return logicalDevice.createDescriptorSetLayoutUnique(
		vk::DescriptorSetLayoutCreateInfo(
			vk::DescriptorSetLayoutCreateFlags(),
			vertexLayoutBindings.size(),
			vertexLayoutBindings.data()));
}

inline vk::UniqueDescriptorSetLayout Device::createFragmentSetLayout(
	const vk::Device& logicalDevice, 
	const uint32_t textureCount, 
	const vk::Sampler sampler)
{
	auto fragmentLayoutBindings = std::vector<vk::DescriptorSetLayoutBinding>
	{
		// Fragment: sampler uniform buffer
		vk::DescriptorSetLayoutBinding(
			0U,
			vk::DescriptorType::eSampler,
			1U,
			vk::ShaderStageFlagBits::eFragment,
			&sampler),
		// Fragment: texture uniform buffer (array)
		vk::DescriptorSetLayoutBinding(
			1U,
			vk::DescriptorType::eSampledImage,
			textureCount,
			vk::ShaderStageFlagBits::eFragment)
	};

	return logicalDevice.createDescriptorSetLayoutUnique(
		vk::DescriptorSetLayoutCreateInfo(
			vk::DescriptorSetLayoutCreateFlags(),
			fragmentLayoutBindings.size(),
			fragmentLayoutBindings.data()));
}