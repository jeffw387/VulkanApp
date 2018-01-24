#define VMA_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "VulkanApp2.hpp"
#include "gsl/gsl"

namespace vka
{
	bool VulkanApp::LoadVulkanDLL()
	{
		m_VulkanLibrary = LoadLibrary(L"vulkan-1.dll");

		if (m_VulkanLibrary == nullptr)
			return false;
		return true;
	}

	bool VulkanApp::LoadVulkanEntryPoint()
	{
#define VK_EXPORTED_FUNCTION( fun )                                                   \
    if( !(fun = (PFN_##fun)GetProcAddress( m_VulkanLibrary, #fun )) ) {               \
      std::cout << "Could not load exported function: " << #fun << "!" << std::endl;  \
    }

#include "VulkanFunctions.inl"

		return true;
	}

	bool VulkanApp::LoadVulkanGlobalFunctions()
	{
#define VK_GLOBAL_LEVEL_FUNCTION( fun )                                                   \
    if( !(fun = (PFN_##fun)vkGetInstanceProcAddr( nullptr, #fun )) ) {                    \
      std::cout << "Could not load global level function: " << #fun << "!" << std::endl;  \
    }

#include "VulkanFunctions.inl"

		return true;
	}

	bool VulkanApp::LoadVulkanInstanceFunctions()
	{
#define VK_INSTANCE_LEVEL_FUNCTION( fun )                                                   \
    if( !(fun = (PFN_##fun)vkGetInstanceProcAddr( m_Instance.get(), #fun )) ) {             \
      std::cout << "Could not load instance level function: " << #fun << "!" << std::endl;  \
    }

#include "VulkanFunctions.inl"

		return true;
	}

	bool VulkanApp::LoadVulkanDeviceFunctions()
	{
#define VK_DEVICE_LEVEL_FUNCTION( fun )                                                   \
    if( !(fun = (PFN_##fun)vkGetDeviceProcAddr( m_LogicalDevice.get(), #fun )) ) {        \
      std::cout << "Could not load device level function: " << #fun << "!" << std::endl;  \
    }

#include "VulkanFunctions.inl"

		return true;
	}

	static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugBreakCallback(
		VkDebugReportFlagsEXT flags, 
		VkDebugReportObjectTypeEXT objType, 
		uint64_t obj,
		size_t location,
		int32_t messageCode,
		const char* pLayerPrefix,
		const char* pMessage, 
		void* userData)
	{
		if (flags & VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_DEBUG_BIT_EXT)
			std::cerr << "(Debug Callback)";
		if (flags & VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_ERROR_BIT_EXT)
			std::cerr << "(Error Callback)";
		if (flags & VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
			return false;
			//std::cerr << "(Information Callback)";
		if (flags & VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
			std::cerr << "(Performance Warning Callback)";
		if (flags & VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_WARNING_BIT_EXT)
			std::cerr << "(Warning Callback)";
		std::cerr << pLayerPrefix << pMessage <<"\n";

		return false;
	}

	struct BufferCreateResult
	{
		vk::UniqueBuffer buffer;
		UniqueVmaAllocation allocation;
		VmaAllocationInfo allocationInfo;
	};
	inline auto VulkanApp::CreateBuffer(
		vk::DeviceSize bufferSize, 
		vk::BufferUsageFlags bufferUsage, 
		VmaMemoryUsage memoryUsage,
		VmaAllocationCreateFlags memoryCreateFlags)
	{
		BufferCreateResult result;
		auto bufferCreateInfo = vk::BufferCreateInfo(
			vk::BufferCreateFlags(),
			bufferSize,
			bufferUsage,
			vk::SharingMode::eExclusive,
			1U,
			&m_GraphicsQueueFamilyID);
		auto allocationCreateInfo = VmaAllocationCreateInfo {};
		allocationCreateInfo.usage = memoryUsage;
		allocationCreateInfo.flags = memoryCreateFlags;
		VkBuffer buffer;
		VmaAllocation allocation;
		vmaCreateBuffer(m_Allocator.get(), 
			&bufferCreateInfo.operator const VkBufferCreateInfo &(),
			&allocationCreateInfo,
			&buffer,
			&allocation,
			&result.allocationInfo);
		
		result.buffer = vk::UniqueBuffer(buffer, vk::BufferDeleter(m_LogicalDevice.get()));
		VmaAllocationDeleter allocationDeleter = VmaAllocationDeleter(m_Allocator.get());
		result.allocation = UniqueVmaAllocation(allocation, allocationDeleter);

		return result;
	}

	inline TemporaryCommandStructure VulkanApp::CreateTemporaryCommandBuffer()
	{
		TemporaryCommandStructure result;
		// get command pool and buffer
		result.pool = m_LogicalDevice->createCommandPoolUnique(
			vk::CommandPoolCreateInfo(
				vk::CommandPoolCreateFlagBits::eTransient,
				m_GraphicsQueueFamilyID));
		auto buffer = m_LogicalDevice->allocateCommandBuffers(
			vk::CommandBufferAllocateInfo(
				result.pool.get(),
				vk::CommandBufferLevel::ePrimary,
				1U))[0];
		auto bufferDeleter = vk::CommandBufferDeleter(m_LogicalDevice.get(), result.pool.get());
		result.buffer = vk::UniqueCommandBuffer(buffer, bufferDeleter);
		return result;
	}

	inline void VulkanApp::CopyToBuffer(
		vk::Buffer source, 
		vk::Buffer destination, 
		vk::DeviceSize size, 
		vk::DeviceSize sourceOffset, 
		vk::DeviceSize destinationOffset,
		std::vector<vk::Semaphore> waitSemaphores = {},
		std::vector<vk::Semaphore> signalSemaphores = {}
	)
	{
		auto fence = m_LogicalDevice->createFenceUnique(vk::FenceCreateInfo());
		auto command = CreateTemporaryCommandBuffer();
		command.buffer->begin(vk::CommandBufferBeginInfo(
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
		command.buffer->copyBuffer(source, destination, {vk::BufferCopy(sourceOffset, destinationOffset, size)});
		command.buffer->end();
		m_GraphicsQueue.submit( {vk::SubmitInfo(
			gsl::narrow<uint32_t>(waitSemaphores.size()),
			waitSemaphores.data(),
			nullptr,
			1U,
			&command.buffer.get(),
			gsl::narrow<uint32_t>(signalSemaphores.size()),
			signalSemaphores.data()) }, fence.get());

		// wait for buffer to finish execution
		m_LogicalDevice->waitForFences(
			{fence.get()}, 
			static_cast<vk::Bool32>(true), 
			std::numeric_limits<uint64_t>::max());
	}

auto createRenderPass(const vk::Device& logicalDevice, const vk::Format surfaceFormat)
{
	// create render pass
	auto colorAttachmentDescription = vk::AttachmentDescription(
		vk::AttachmentDescriptionFlags(),
		surfaceFormat,
		vk::SampleCountFlagBits::e1,
		vk::AttachmentLoadOp::eClear,
		vk::AttachmentStoreOp::eStore,
		vk::AttachmentLoadOp::eDontCare,
		vk::AttachmentStoreOp::eDontCare,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::ePresentSrcKHR
	);
	auto colorAttachmentRef = vk::AttachmentReference(
		0,
		vk::ImageLayout::eColorAttachmentOptimal);

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
		gsl::narrow<uint32_t>(dependencies.size()),
		dependencies.data());
	return logicalDevice.createRenderPassUnique(renderPassInfo);
}

inline void createPipelineCreateInfo(
	std::vector<vk::SpecializationMapEntry>& vertexSpecs,
	std::vector<vk::SpecializationMapEntry>& fragmentSpecs,
	vk::SpecializationInfo& vertexSpecInfo,
	vk::SpecializationInfo& fragmentSpecInfo,
	const uint32_t textureCount,
	vk::PipelineShaderStageCreateInfo& vertexShaderStageInfo,
	vk::PipelineShaderStageCreateInfo& fragmentShaderStageInfo,
	const vk::ShaderModule& vertexShader,
	const vk::ShaderModule& fragmentShader,
	std::vector<vk::PipelineShaderStageCreateInfo>& pipelineShaderInfo,
	std::vector<vk::VertexInputBindingDescription>& vertexBindings,
	std::vector<vk::VertexInputAttributeDescription>& vertexAttribs,
	vk::PipelineVertexInputStateCreateInfo& pipelineVertexInputInfo,
	vk::PipelineInputAssemblyStateCreateInfo& pipelineInputAssemblyInfo,
	vk::PipelineTessellationStateCreateInfo& pipelineTesselationInfo,
	vk::PipelineViewportStateCreateInfo& pipelineViewportInfo,
	vk::PipelineRasterizationStateCreateInfo& pipelineRasterizationInfo,
	vk::PipelineMultisampleStateCreateInfo& pipelineMultisampleInfo,
	vk::PipelineDepthStencilStateCreateInfo& pipelineDepthInfo,
	vk::PipelineColorBlendAttachmentState& pipelineBlendAttachmentInfo,
	vk::PipelineColorBlendStateCreateInfo& pipelineBlendInfo,
	std::vector<vk::DynamicState>& dynamicStates,
	vk::PipelineDynamicStateCreateInfo& pipelineDynamicInfo,
	const vk::PipelineLayout& pipelineLayout,
	const vk::RenderPass& renderPass,
	vk::GraphicsPipelineCreateInfo& pipelineCreateInfo
	)
{
	fragmentSpecs.emplace_back(vk::SpecializationMapEntry(
		0U,
		0U,
		sizeof(glm::uint32)));

	vertexSpecInfo = vk::SpecializationInfo();
	fragmentSpecInfo = vk::SpecializationInfo(
		gsl::narrow<uint32_t>(fragmentSpecs.size()),
		fragmentSpecs.data(),
		gsl::narrow<uint32_t>(sizeof(glm::uint32)),
		&textureCount
	);

	vertexShaderStageInfo = vk::PipelineShaderStageCreateInfo(
		vk::PipelineShaderStageCreateFlags(),
		vk::ShaderStageFlagBits::eVertex,
		vertexShader,
		"main",
		&vertexSpecInfo);

	fragmentShaderStageInfo = vk::PipelineShaderStageCreateInfo(
		vk::PipelineShaderStageCreateFlags(),
		vk::ShaderStageFlagBits::eFragment,
		fragmentShader,
		"main",
		&fragmentSpecInfo);

	pipelineShaderInfo = std::vector<vk::PipelineShaderStageCreateInfo>{ vertexShaderStageInfo, fragmentShaderStageInfo };

	vertexBindings = Vertex::vertexBindingDescriptions();
	vertexAttribs = Vertex::vertexAttributeDescriptions();
	pipelineVertexInputInfo = vk::PipelineVertexInputStateCreateInfo(
		vk::PipelineVertexInputStateCreateFlags(),
		gsl::narrow<uint32_t>(vertexBindings.size()),
		vertexBindings.data(),
		gsl::narrow<uint32_t>(vertexAttribs.size()),
		vertexAttribs.data());

	pipelineInputAssemblyInfo = vk::PipelineInputAssemblyStateCreateInfo(
		vk::PipelineInputAssemblyStateCreateFlags(),
		vk::PrimitiveTopology::eTriangleList);

	pipelineTesselationInfo = vk::PipelineTessellationStateCreateInfo(
		vk::PipelineTessellationStateCreateFlags());

	pipelineViewportInfo = vk::PipelineViewportStateCreateInfo(
		vk::PipelineViewportStateCreateFlags(),
		1U,
		nullptr,
		1U,
		nullptr);

	pipelineRasterizationInfo = vk::PipelineRasterizationStateCreateInfo(
		vk::PipelineRasterizationStateCreateFlags(),
		false,
		false,
		vk::PolygonMode::eFill,
		vk::CullModeFlagBits::eNone,
		vk::FrontFace::eCounterClockwise,
		false,
		0.f,
		0.f,
		0.f,
		1.f);

	pipelineMultisampleInfo = vk::PipelineMultisampleStateCreateInfo();

	pipelineDepthInfo = vk::PipelineDepthStencilStateCreateInfo();

	pipelineBlendAttachmentInfo = vk::PipelineColorBlendAttachmentState(
		true,
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

	pipelineBlendInfo = vk::PipelineColorBlendStateCreateInfo(
		vk::PipelineColorBlendStateCreateFlags(),
		0U,
		vk::LogicOp::eClear,
		1U,
		&pipelineBlendAttachmentInfo);

	dynamicStates = std::vector<vk::DynamicState>{ vk::DynamicState::eViewport, vk::DynamicState::eScissor };

	pipelineDynamicInfo = vk::PipelineDynamicStateCreateInfo(
		vk::PipelineDynamicStateCreateFlags(),
		gsl::narrow<uint32_t>(dynamicStates.size()),
		dynamicStates.data());

	pipelineCreateInfo = vk::GraphicsPipelineCreateInfo(
		vk::PipelineCreateFlags(),
		gsl::narrow<uint32_t>(pipelineShaderInfo.size()),
		pipelineShaderInfo.data(),
		&pipelineVertexInputInfo,
		&pipelineInputAssemblyInfo,
		&pipelineTesselationInfo,
		&pipelineViewportInfo,
		&pipelineRasterizationInfo,
		&pipelineMultisampleInfo,
		&pipelineDepthInfo,
		&pipelineBlendInfo,
		&pipelineDynamicInfo,
		pipelineLayout,
		renderPass
	);
}

inline vk::UniqueSwapchainKHR VulkanApp::createSwapChain()
{
	vk::Bool32 presentSupport;
	vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, m_GraphicsQueueFamilyID, m_Surface.get(), &presentSupport);
	if (!presentSupport)
	{
		std::runtime_error("Presentation to this surface isn't supported.");
		exit(1);
	}
	return m_LogicalDevice->createSwapchainKHRUnique(
		vk::SwapchainCreateInfoKHR(
			vk::SwapchainCreateFlagsKHR(),
			m_Surface.get(),
			BufferCount,
			m_SurfaceFormat,
			m_SurfaceColorSpace,
			m_SurfaceExtent,
			1U,
			vk::ImageUsageFlagBits::eColorAttachment,
			vk::SharingMode::eExclusive,
			1U,
			&m_GraphicsQueueFamilyID,
			vk::SurfaceTransformFlagBitsKHR::eIdentity,
			vk::CompositeAlphaFlagBitsKHR::eOpaque,
			m_PresentMode,
			0U));
}

inline SwapChainDependencies::SwapChainDependencies(
	const vk::Device & logicalDevice,
	vk::UniqueSwapchainKHR swapChain,
	const uint32_t bufferCount,
	const vk::RenderPass & renderPass,
	const vk::Format & surfaceFormat,
	const vk::Extent2D & surfaceExtent,
	const vk::GraphicsPipelineCreateInfo& pipelineCreateInfo
) noexcept :
	m_Swapchain(std::move(swapChain))
{
	m_SwapImages = logicalDevice.getSwapchainImagesKHR(m_Swapchain.get());
	m_SwapViews.resize(bufferCount);
	m_Framebuffers.resize(bufferCount);
	for (auto i = 0U; i < bufferCount; i++)
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
	VkPipeline pipeline;
	auto pipelineResult = vkCreateGraphicsPipelines(logicalDevice, NULL, 1U, &pipelineCreateInfo.operator const VkGraphicsPipelineCreateInfo &(), nullptr, &pipeline);
	m_Pipeline = logicalDevice.createGraphicsPipelineUnique(vk::PipelineCache(), pipelineCreateInfo);
}

inline void VulkanApp::resizeWindow(ClientSize size)
{
	m_SurfaceExtent = vk::Extent2D{ gsl::narrow_cast<uint32_t>(size.width), gsl::narrow_cast<uint32_t>(size.height) };
	m_SwapchainDependencies = SwapChainDependencies(
		m_LogicalDevice.get(),
		createSwapChain(),
		BufferCount,
		m_RenderPass.get(),
		m_SurfaceFormat,
		m_SurfaceExtent,
		m_PipelineCreateInfo);
}

void VulkanApp::CreateVertexBuffer()
{
	// create vertex buffers
	auto vertexBufferSize = sizeof(Vertex) * m_Vertices.size();
	auto vertexStagingResult = CreateBuffer(vertexBufferSize,
		vk::BufferUsageFlagBits::eVertexBuffer |
		vk::BufferUsageFlagBits::eTransferSrc,
		VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_ONLY,
		VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
	auto vertexResult = CreateBuffer(vertexBufferSize,
		vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
		VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY,
		0);
	// copy data to vertex buffer
	void* vertexStagingData;
	vmaMapMemory(m_Allocator.get(), vertexStagingResult.allocation.get(), &vertexStagingData);
	memcpy(vertexStagingData, m_Vertices.data(), vertexBufferSize);
	vmaUnmapMemory(m_Allocator.get(), vertexStagingResult.allocation.get());
	CopyToBuffer(vertexStagingResult.buffer.get(),
		vertexResult.buffer.get(),
		vertexBufferSize,
		vertexStagingResult.allocationInfo.offset,
		vertexResult.allocationInfo.offset);
	// transfer ownership to VulkanApp
	auto vertexBufferDeleter = vk::BufferDeleter(m_LogicalDevice.get());
	m_VertexBuffer = vk::UniqueBuffer(vertexResult.buffer.release(), vertexBufferDeleter);
}

void VulkanApp::CreateIndexBuffer()
{
	//create index buffers
	auto indexBufferSize = sizeof(Index) * m_Indices.size();
	auto indexStagingResult = CreateBuffer(indexBufferSize,
		vk::BufferUsageFlagBits::eTransferSrc |
		vk::BufferUsageFlagBits::eIndexBuffer,
		VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_ONLY,
		VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
	auto indexResult = CreateBuffer(indexBufferSize,
		vk::BufferUsageFlagBits::eTransferDst |
		vk::BufferUsageFlagBits::eIndexBuffer,
		VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY,
		0);

	// copy data to index buffer
	void* indexStagingData;
	vmaMapMemory(m_Allocator.get(), indexStagingResult.allocation.get(), &indexStagingData);
	memcpy(indexStagingData, m_Indices.data(), indexBufferSize);
	vmaUnmapMemory(m_Allocator.get(), indexStagingResult.allocation.get());
	CopyToBuffer(indexStagingResult.buffer.get(),
		indexResult.buffer.get(),
		indexBufferSize,
		indexStagingResult.allocationInfo.offset,
		indexResult.allocationInfo.offset);
	// transfer ownership to VulkanApp
	auto indexBufferDeleter = vk::BufferDeleter(m_LogicalDevice.get());
	m_IndexBuffer = vk::UniqueBuffer(indexResult.buffer.release(), indexBufferDeleter);
}

VulkanApp::VulkanApp(InitData initData) :
	m_Window(
		initData.WindowClassName, 
		initData.WindowTitle, 
		initData.windowStyle, 
		this, 
		initData.width, 
		initData.height),
	m_Camera(),
	m_Text(this)
{
	// Window resize callback
	m_Window.SetResizeCallback([](void* ptr, ClientSize size)
	{
		((VulkanApp*)ptr)->resizeWindow(size);
	}
	);
	m_Camera.setSize({static_cast<float>(initData.width), static_cast<float>(initData.height)});

	// load vulkan dll, entry point, and instance/global function pointers
	if (!LoadVulkanDLL())
	{
		exit(1);
	}
	LoadVulkanEntryPoint();
	LoadVulkanGlobalFunctions();
	m_Instance = vk::createInstanceUnique(initData.instanceCreateInfo);
	LoadVulkanInstanceFunctions();

	// select physical device
	auto devices = m_Instance->enumeratePhysicalDevices();
	m_PhysicalDevice = devices[0];

	m_UniformBufferOffsetAlignment = m_PhysicalDevice.getProperties().limits.minUniformBufferOffsetAlignment;

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
			static_cast<uint32_t>(initData.deviceExtensions.size()),
			initData.deviceExtensions.data(),
			&gpuFeatures)
	);
	// Load device-dependent vulkan function pointers
	LoadVulkanDeviceFunctions();

	// get the queue from the logical device
	m_GraphicsQueue = m_LogicalDevice->getQueue(m_GraphicsQueueFamilyID, 0);

	// initialize debug reporting
	if (vkCreateDebugReportCallbackEXT)
	{
		m_DebugBreakpointCallbackData = m_Instance->createDebugReportCallbackEXTUnique(
			vk::DebugReportCallbackCreateInfoEXT(
				vk::DebugReportFlagBitsEXT::ePerformanceWarning |
				vk::DebugReportFlagBitsEXT::eError |
				vk::DebugReportFlagBitsEXT::eDebug |
				vk::DebugReportFlagBitsEXT::eInformation |
				vk::DebugReportFlagBitsEXT::eWarning,
				reinterpret_cast<PFN_vkDebugReportCallbackEXT>(&debugBreakCallback)
				));
	}

	// create the allocator
	auto makeAllocator = [&]()
	{
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = m_PhysicalDevice.operator VkPhysicalDevice();
		allocatorInfo.device = m_LogicalDevice->operator VkDevice();
		allocatorInfo.frameInUseCount = 1U;
		VmaVulkanFunctions vulkanFunctions = {};
		vulkanFunctions.vkAllocateMemory = vkAllocateMemory;
		vulkanFunctions.vkBindBufferMemory = vkBindBufferMemory;
		vulkanFunctions.vkBindImageMemory = vkBindImageMemory;
		vulkanFunctions.vkCreateBuffer = vkCreateBuffer;
		vulkanFunctions.vkCreateImage = vkCreateImage;
		vulkanFunctions.vkDestroyBuffer = vkDestroyBuffer;
		vulkanFunctions.vkDestroyImage = vkDestroyImage;
		vulkanFunctions.vkFreeMemory = vkFreeMemory;
		vulkanFunctions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
		vulkanFunctions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR;
		vulkanFunctions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
		vulkanFunctions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
		vulkanFunctions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
		vulkanFunctions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
		vulkanFunctions.vkMapMemory = vkMapMemory;
		vulkanFunctions.vkUnmapMemory = vkUnmapMemory;
		allocatorInfo.pVulkanFunctions = &vulkanFunctions;

		VmaAllocator alloc;
		vmaCreateAllocator(&allocatorInfo, &alloc);
		m_Allocator = UniqueVmaAllocator(alloc);
	};
	makeAllocator();

	// allow client app to load images
	initData.imageLoadCallback(this);
	m_TextureCount = gsl::narrow<uint32_t>(m_Images.size());

	CreateVertexBuffer();

	CreateIndexBuffer();

	// create surface
	auto inst = m_Window.getHINSTANCE();
	auto hwnd = m_Window.getHWND();
	auto surfaceCreateInfo = vk::Win32SurfaceCreateInfoKHR(vk::Win32SurfaceCreateFlagsKHR(), inst, hwnd);
	VkSurfaceKHR surface;
	vkCreateWin32SurfaceKHR(m_Instance.get(), &surfaceCreateInfo.operator const VkWin32SurfaceCreateInfoKHR &(), nullptr, &surface);
	m_Surface = vk::UniqueSurfaceKHR(surface, vk::SurfaceKHRDeleter(m_Instance.get()));
	// get surface format from supported list
	auto surfaceFormats = m_PhysicalDevice.getSurfaceFormatsKHR(m_Surface.get());
	if (surfaceFormats.size() == 1 && surfaceFormats[0].format == vk::Format::eUndefined)
		m_SurfaceFormat = vk::Format::eB8G8R8A8Unorm;
	else
		m_SurfaceFormat = surfaceFormats[0].format;
	// get surface color space
	m_SurfaceColorSpace = surfaceFormats[0].colorSpace;

	auto formatProperties = m_PhysicalDevice.getFormatProperties(vk::Format::eR8G8B8A8Unorm);
	auto surfaceCapabilities = m_PhysicalDevice.getSurfaceCapabilitiesKHR(m_Surface.get());
	auto surfacePresentModes = m_PhysicalDevice.getSurfacePresentModesKHR(m_Surface.get());

	if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachmentBlend))
	{
		std::runtime_error("Blend not supported by surface format.");
	}
	// get surface extent
	if (surfaceCapabilities.currentExtent.width == -1 ||
		surfaceCapabilities.currentExtent.height == -1)
	{
		auto size = m_Window.GetClientSize();
		m_SurfaceExtent = vk::Extent2D
		{
			static_cast<uint32_t>(size.width),
			static_cast<uint32_t>(size.height)
		};
	}
	else
	{
		m_SurfaceExtent = surfaceCapabilities.currentExtent;
	}

	m_RenderPass = createRenderPass(m_LogicalDevice.get(), m_SurfaceFormat);

	// create sampler
	m_Sampler = m_LogicalDevice.get().createSamplerUnique(
		vk::SamplerCreateInfo(
			vk::SamplerCreateFlags(),
			vk::Filter::eLinear,
			vk::Filter::eLinear,
			vk::SamplerMipmapMode::eLinear,
			vk::SamplerAddressMode::eRepeat,
			vk::SamplerAddressMode::eRepeat,
			vk::SamplerAddressMode::eRepeat,
			0.f,
			1U,
			16.f,
			0U,
			vk::CompareOp::eNever));

	// create shader modules
	m_VertexShader = createShaderModule(m_LogicalDevice.get(), initData.shaderData.vertexShaderPath);
	m_FragmentShader = createShaderModule(m_LogicalDevice.get(), initData.shaderData.fragmentShaderPath);

	// create descriptor set layouts
	m_VertexDescriptorSetLayout = createVertexSetLayout(m_LogicalDevice.get());
	m_FragmentDescriptorSetLayout = createFragmentSetLayout(m_LogicalDevice.get(), gsl::narrow<uint32_t>(m_Textures.size()), m_Sampler.get());
	m_DescriptorSetLayouts.push_back(m_VertexDescriptorSetLayout.get());
	m_DescriptorSetLayouts.push_back(m_FragmentDescriptorSetLayout.get());

	// setup push constant ranges
	m_PushConstantRanges.emplace_back(vk::PushConstantRange(
		vk::ShaderStageFlagBits::eFragment,
		0U,
		sizeof(FragmentPushConstants)));

	// create fragment layout descriptor pool
	m_FragmentLayoutDescriptorPool = createFragmentDescriptorPool(
		m_LogicalDevice.get(),
		m_TextureCount);

	m_FragmentDescriptorSet = std::move(
		m_LogicalDevice->allocateDescriptorSetsUnique(
			vk::DescriptorSetAllocateInfo(
				m_FragmentLayoutDescriptorPool.get(),
				1U,
				&m_FragmentDescriptorSetLayout.get()))[0]);

	// update fragment descriptor set
	m_LogicalDevice->updateDescriptorSets(
		{ 
			vk::WriteDescriptorSet(
				m_FragmentDescriptorSet.get(),
				0U,
				0U,
				1U,
				vk::DescriptorType::eSampler,
				&vk::DescriptorImageInfo(m_Sampler.get()))
		},
		{ }
	);
	auto imageInfos = std::vector<vk::DescriptorImageInfo>();
	imageInfos.reserve(m_TextureCount);
	for (auto i = 0U; i < m_TextureCount; i++)
	{
		imageInfos.emplace_back(vk::DescriptorImageInfo(
			nullptr, 
			m_ImageViews[i].get(), 
			vk::ImageLayout::eShaderReadOnlyOptimal));
	}
	m_LogicalDevice->updateDescriptorSets(
		{ 
			vk::WriteDescriptorSet(
				m_FragmentDescriptorSet.get(),
				1U,
				0U,
				gsl::narrow<uint32_t>(imageInfos.size()),
				vk::DescriptorType::eSampledImage,
				imageInfos.data())
		},
		{ }
	);

	// default present mode: immediate
	m_PresentMode = vk::PresentModeKHR::eImmediate;

	// use mailbox present mode if supported
	auto mailboxPresentMode = std::find(surfacePresentModes.begin(), surfacePresentModes.end(), vk::PresentModeKHR::eMailbox);
	if (mailboxPresentMode != surfacePresentModes.end())
		m_PresentMode = *mailboxPresentMode;

	// create pipeline layout
	auto pipelineLayoutInfo = vk::PipelineLayoutCreateInfo(
		vk::PipelineLayoutCreateFlags(),
		gsl::narrow<uint32_t>(m_DescriptorSetLayouts.size()),
		m_DescriptorSetLayouts.data(),
		gsl::narrow<uint32_t>(m_PushConstantRanges.size()),
		m_PushConstantRanges.data());
	m_PipelineLayout = m_LogicalDevice->createPipelineLayoutUnique(pipelineLayoutInfo);

	// create pipeline creation info
	createPipelineCreateInfo(
		m_VertexSpecializations,
		m_FragmentSpecializations,
		m_VertexSpecializationInfo,
		m_FragmentSpecializationInfo,
		m_TextureCount,
		m_VertexShaderStageInfo,
		m_FragmentShaderStageInfo,
		m_VertexShader.get(),
		m_FragmentShader.get(),
		m_PipelineShaderStageInfo,
		m_VertexBindings,
		m_VertexAttributes,
		m_PipelineVertexInputInfo,
		m_PipelineInputAssemblyInfo,
		m_PipelineTesselationStateInfo,
		m_PipelineViewportInfo,
		m_PipelineRasterizationInfo,
		m_PipelineMultisampleInfo,
		m_PipelineDepthStencilInfo,
		m_PipelineColorBlendAttachmentState,
		m_PipelineBlendStateInfo,
		m_DynamicStates,
		m_PipelineDynamicStateInfo,
		m_PipelineLayout.get(),
		m_RenderPass.get(),
		m_PipelineCreateInfo
	);

	// init all the swapchain dependencies
	m_SwapchainDependencies = SwapChainDependencies(
		m_LogicalDevice.get(),
		createSwapChain(),
		BufferCount,
		m_RenderPass.get(),
		m_SurfaceFormat,
		m_SurfaceExtent,
		m_PipelineCreateInfo);

	// init the support structures for each frame buffer
	// -these should not depend on the swapchain and shouldn't need to
	// -be recreated when the window is resized
	m_FramebufferSupports.resize(BufferCount);
	for (auto& support : m_FramebufferSupports)
	{
		support.m_CommandPool = m_LogicalDevice->createCommandPoolUnique(
			vk::CommandPoolCreateInfo(
				vk::CommandPoolCreateFlagBits::eResetCommandBuffer | 
					vk::CommandPoolCreateFlagBits::eTransient,
				m_GraphicsQueueFamilyID));
		support.m_CommandBuffer = std::move(m_LogicalDevice->allocateCommandBuffersUnique(
			vk::CommandBufferAllocateInfo(
				support.m_CommandPool.get(),
				vk::CommandBufferLevel::ePrimary,
				1U))[0]);
		support.m_VertexLayoutDescriptorPool = createVertexDescriptorPool(
			m_LogicalDevice.get(),
			BufferCount);
		support.m_BufferCapacity = 0U;
		support.m_ImageRenderCompleteSemaphore = m_LogicalDevice->createSemaphoreUnique(
			vk::SemaphoreCreateInfo(
				vk::SemaphoreCreateFlags()));
		support.m_MatrixBufferStagingCompleteSemaphore = m_LogicalDevice->createSemaphoreUnique(
			vk::SemaphoreCreateInfo(
				vk::SemaphoreCreateFlags()));
	}
}

inline auto VulkanApp::acquireImage()
{
	auto renderFinishedFence = m_LogicalDevice->createFence(
		vk::FenceCreateInfo(vk::FenceCreateFlags()));
	auto nextImage = m_LogicalDevice->acquireNextImageKHR(
		m_SwapchainDependencies.m_Swapchain.get(), 
		std::numeric_limits<uint64_t>::max(), 
		vk::Semaphore(),
		renderFinishedFence);
	m_FramebufferSupports[nextImage.value].m_ImagePresentCompleteFence = vk::UniqueFence(renderFinishedFence);
	return nextImage;
}

inline bool VulkanApp::BeginRender(SpriteCount spriteCount)
{
	auto nextImage = acquireImage();
	if (nextImage.result != vk::Result::eSuccess)
	{
		return false;
	}
	m_UpdateData.nextImage = nextImage.value;

	auto& supports = m_FramebufferSupports[nextImage.value];

	// Sync host to device for the next image
	m_LogicalDevice->waitForFences(
		{supports.m_ImagePresentCompleteFence.get()}, 
		static_cast<vk::Bool32>(true), 
		std::numeric_limits<uint64_t>::max());

	const size_t minimumCount = 1;
	size_t instanceCount = std::max(minimumCount, spriteCount);
	// (re)create matrix buffer if it is smaller than required
	if (instanceCount > supports.m_BufferCapacity)
	{
		supports.m_BufferCapacity = instanceCount * 2;

		auto newBufferSize = supports.m_BufferCapacity * m_UniformBufferOffsetAlignment;

		// create staging buffer
		auto stagingBufferResult = CreateBuffer(newBufferSize, 
			vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eUniformBuffer,
			VMA_MEMORY_USAGE_CPU_ONLY,
			VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
		supports.m_MatrixStagingBuffer = std::move(stagingBufferResult.buffer);
		supports.m_MatrixStagingMemory = std::move(stagingBufferResult.allocation);
		supports.m_MatrixStagingMemoryInfo = stagingBufferResult.allocationInfo;

		// create matrix buffer
		auto matrixBufferResult = CreateBuffer(newBufferSize,
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer,
			VMA_MEMORY_USAGE_GPU_ONLY,
			VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
		supports.m_MatrixBuffer = std::move(matrixBufferResult.buffer);
		supports.m_MatrixMemory = std::move(matrixBufferResult.allocation);
		supports.m_MatrixMemoryInfo = matrixBufferResult.allocationInfo;
	}
	
	vmaMapMemory(m_Allocator.get(), supports.m_MatrixStagingMemory.get(), &m_UpdateData.mapped);

	m_UpdateData.copyOffset = 0;
	m_UpdateData.vp = m_Camera.getMatrix();

	// TODO: benchmark whether staging buffer use is faster than alternatives
	m_LogicalDevice->updateDescriptorSets(
		{ 
			vk::WriteDescriptorSet(
				supports.m_VertexDescriptorSet.get(),
				0U,
				0U,
				1U,
				vk::DescriptorType::eUniformBufferDynamic,
				nullptr,
				&vk::DescriptorBufferInfo(
					supports.m_MatrixBuffer.get(),
					0Ui64,
					m_UniformBufferOffsetAlignment),
				nullptr) 
		},
		{ }
	);

	/////////////////////////////////////////////////////////////
	// set up data that is constant between all command buffers
	//

	// Dynamic viewport info
	auto viewport = vk::Viewport(
		0.f,
		0.f,
		static_cast<float>(m_SurfaceExtent.width),
		static_cast<float>(m_SurfaceExtent.height),
		0.f,
		1.f);

	// Dynamic scissor info
	auto scissorRect = vk::Rect2D(
	vk::Offset2D(0, 0),
	m_SurfaceExtent);

	// clear values for the color and depth attachments
	auto clearValue = vk::ClearValue(vk::ClearColorValue(std::array<float, 4>{0.f, 0.f, 0.f, 0.f}));

	// reset command buffer
	supports.m_CommandBuffer->reset(vk::CommandBufferResetFlags());

	// Render pass info
	auto renderPassInfo = vk::RenderPassBeginInfo(
		m_RenderPass.get(),
		m_SwapchainDependencies.m_Framebuffers[nextImage.value].get(),
		scissorRect,
		1U,
		&clearValue);

	// record the command buffer
	supports.m_CommandBuffer->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
	supports.m_CommandBuffer->setViewport(0U, { viewport });
	supports.m_CommandBuffer->setScissor(0U, { scissorRect });
	supports.m_CommandBuffer->beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);
	supports.m_CommandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, m_SwapchainDependencies.m_Pipeline.get());
	supports.m_CommandBuffer->bindVertexBuffers(
		0U, 
		{ m_VertexBuffer.get() }, 
		{ m_VertexMemoryInfo.offset });
	supports.m_CommandBuffer->bindIndexBuffer(
		m_IndexBuffer.get(), 
		m_IndexMemoryInfo.offset, 
		vk::IndexType::eUint32);

	// bind sampler and images uniforms
	supports.m_CommandBuffer->bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics,
		m_PipelineLayout.get(),
		0U,
		{ m_FragmentDescriptorSet.get() },
		{});

	m_UpdateData.spriteIndex = 0;
	return true;
}

void VulkanApp::RenderSprite(const Sprite& sprite)
{
	auto& supports = m_FramebufferSupports[m_UpdateData.nextImage];
	glm::mat4 mvp =  m_UpdateData.vp * sprite.transform;

	memcpy((char*)m_UpdateData.mapped + m_UpdateData.copyOffset, &mvp, sizeof(glm::mat4));
	m_UpdateData.copyOffset += m_UniformBufferOffsetAlignment;

	FragmentPushConstants pushRange;
	pushRange.textureID = sprite.textureIndex;
	pushRange.r = sprite.color.r;
	pushRange.g = sprite.color.g;
	pushRange.b = sprite.color.b;
	pushRange.a = sprite.color.a;

	supports.m_CommandBuffer->pushConstants<FragmentPushConstants>(
		m_PipelineLayout.get(),
		vk::ShaderStageFlagBits::eFragment,
		0U,
		{ pushRange });

	uint32_t dynamicOffset = gsl::narrow<uint32_t>(m_UpdateData.spriteIndex * m_UniformBufferOffsetAlignment);

	// bind dynamic matrix uniform
	supports.m_CommandBuffer->bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics,
		m_PipelineLayout.get(),
		0U,
		{ supports.m_VertexDescriptorSet.get() },
		{ dynamicOffset });

	auto firstIndex = sprite.textureIndex * IndicesPerQuad;
	// draw the sprite
	supports.m_CommandBuffer->drawIndexed(IndicesPerQuad, 1U, firstIndex, 0U, 0U);
	m_UpdateData.spriteIndex++;
}

inline void VulkanApp::EndRender()
{
	auto& supports = m_FramebufferSupports[m_UpdateData.nextImage];
	// copy matrices from staging buffer to gpu buffer
	vmaUnmapMemory(m_Allocator.get(), supports.m_MatrixStagingMemory.get());

	CopyToBuffer(
		supports.m_MatrixBuffer.get(),
		supports.m_MatrixStagingBuffer.get(),
		supports.m_MatrixMemoryInfo.size,
		supports.m_MatrixStagingMemoryInfo.offset,
		supports.m_MatrixMemoryInfo.offset,
		{},
		{ 
			supports.m_MatrixBufferStagingCompleteSemaphore.get() 
		});

	// Finish recording draw command buffer
	supports.m_CommandBuffer->endRenderPass();
	supports.m_CommandBuffer->end();

	// Submit draw command buffer
	m_GraphicsQueue.submit(
		{
			vk::SubmitInfo(
				1U,
				&supports.m_MatrixBufferStagingCompleteSemaphore.get(),
				&vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput),
				1U,
				&supports.m_CommandBuffer.get(),
				1U,
				&supports.m_ImageRenderCompleteSemaphore.get())
		}, vk::Fence());

	// Present image
	auto result = m_GraphicsQueue.presentKHR(
		vk::PresentInfoKHR(
			1U,
			&supports.m_ImageRenderCompleteSemaphore.get(),
			1U,
			&m_SwapchainDependencies.m_Swapchain.get(),
			&m_UpdateData.nextImage));

	if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
	{
		resizeWindow(m_Window.GetClientSize());
	}
	else if (result != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to present swap chain image!");
	}
}

void VulkanApp::Run(LoopCallbacks callbacks)
{
	while (true)
	{
		//loop
		if (m_Window.PollEvents())
		{
			return;
		}
		auto spriteCount = callbacks.BeforeRenderCallback();
		if (!BeginRender(spriteCount))
		{
			// TODO: probably need to handle some specific errors here
			continue;
		}
		callbacks.RenderCallback(this);
		EndRender();
		callbacks.AfterRenderCallback();
	}
}

inline void VulkanApp::createVulkanImageForBitmap(const Bitmap& bitmap,
	VmaAllocationInfo& imageAllocInfo,
	vk::UniqueImage& image2D,
	UniqueVmaAllocation& imageAlloc)
{
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
		&m_GraphicsQueueFamilyID,
		vk::ImageLayout::eUndefined);

	auto imageAllocCreateInfo = VmaAllocationCreateInfo{};
	imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	VkImage image;
	VmaAllocation imageAllocation;

	vmaCreateImage(m_Allocator.get(),
		&imageInfo.operator const VkImageCreateInfo &(),
		&imageAllocCreateInfo,
		&image,
		&imageAllocation,
		&imageAllocInfo);

	auto imageDeleter = vk::ImageDeleter(m_LogicalDevice.get());
	image2D = vk::UniqueImage(vk::Image(image), imageDeleter);
	imageAlloc = UniqueVmaAllocation(imageAllocation, VmaAllocationDeleter(m_Allocator.get()));
}

uint32_t VulkanApp::createTexture(const Bitmap & bitmap)
{
	uint32_t textureIndex = static_cast<uint32_t>(m_Textures.size());
	// create vke::Image2D
	auto& image2D = m_Images.emplace_back();
	auto& imageView = m_ImageViews.emplace_back();
	auto& imageAlloc = m_ImageAllocations.emplace_back();
	auto& imageAllocInfo = m_ImageAllocationInfos.emplace_back();
	auto& texture = m_Textures.emplace_back();

	createVulkanImageForBitmap(bitmap, imageAllocInfo, image2D, imageAlloc);

	float left = static_cast<float>(bitmap.m_Left);
	float top = static_cast<float>(bitmap.m_Top);
	float width = static_cast<float>(bitmap.m_Width);
	float height = static_cast<float>(bitmap.m_Height);
	float right = left + width;
	float bottom = top - height;

	glm::vec2 LeftTopPos, LeftBottomPos, RightBottomPos, RightTopPos;
	LeftTopPos = { left,		top };
	LeftBottomPos = { left,		bottom };
	RightBottomPos = { right,	bottom };
	RightTopPos = { right,	top };

	glm::vec2 LeftTopUV, LeftBottomUV, RightBottomUV, RightTopUV;
	LeftTopUV = { 0.0f, 0.0f };
	LeftBottomUV = { 0.0f, 1.0f };
	RightBottomUV = { 1.0f, 1.0f };
	RightTopUV = { 1.0f, 0.0f };

	Vertex LeftTop, LeftBottom, RightBottom, RightTop;
	LeftTop = { LeftTopPos,		LeftTopUV };
	LeftBottom = { LeftBottomPos,	LeftBottomUV };
	RightBottom = { RightBottomPos,	RightBottomUV };
	RightTop = { RightTopPos,		RightTopUV };

	// push back vertices
	m_Vertices.push_back(LeftTop);
	m_Vertices.push_back(LeftBottom);
	m_Vertices.push_back(RightBottom);
	m_Vertices.push_back(RightTop);

	auto verticesPerTexture = 4U;
	uint32_t indexOffset = verticesPerTexture * textureIndex;

	// push back indices
	m_Indices.push_back(indexOffset + 0);
	m_Indices.push_back(indexOffset + 1);
	m_Indices.push_back(indexOffset + 2);
	m_Indices.push_back(indexOffset + 2);
	m_Indices.push_back(indexOffset + 3);
	m_Indices.push_back(indexOffset + 0);

	// initialize the texture object
	texture.index = textureIndex;
	texture.width = bitmap.m_Width;
	texture.height = bitmap.m_Height;
	texture.size = imageAllocInfo.size;

	auto stagingBufferResult = CreateBuffer(texture.size,
		vk::BufferUsageFlagBits::eTransferSrc,
		VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_ONLY,
		VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
	// copy from host to staging buffer
	void* stagingBufferData;
	vmaMapMemory(m_Allocator.get(), stagingBufferResult.allocation.get(), &stagingBufferData);
	memcpy(stagingBufferData, bitmap.m_Data.data(), static_cast<size_t>(bitmap.m_Size));
	vmaUnmapMemory(m_Allocator.get(), stagingBufferResult.allocation.get());

	// get command pool and buffer
	auto command = CreateTemporaryCommandBuffer();

	// begin recording command buffer
	command.buffer->begin(vk::CommandBufferBeginInfo(
		vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

	// transition image layout
	auto memoryBarrier = vk::ImageMemoryBarrier(
		vk::AccessFlags(),
		vk::AccessFlagBits::eTransferWrite,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eTransferDstOptimal,
		m_GraphicsQueueFamilyID,
		m_GraphicsQueueFamilyID,
		image2D.get(),
		vk::ImageSubresourceRange(
			vk::ImageAspectFlagBits::eColor,
			0U,
			1U,
			0U,
			1U));
	command.buffer->pipelineBarrier(
		vk::PipelineStageFlagBits::eTopOfPipe,
		vk::PipelineStageFlagBits::eTransfer,
		vk::DependencyFlags(),
		{ },
		{ },
		{ memoryBarrier });

	// copy from host to device (to image)
	command.buffer->copyBufferToImage(stagingBufferResult.buffer.get(), image2D.get(), vk::ImageLayout::eTransferDstOptimal,
		{
			vk::BufferImageCopy(0U, 0U, 0U,
			vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0U, 0U, 1U),
			vk::Offset3D(),
			vk::Extent3D(bitmap.m_Width, bitmap.m_Height, 1U))
		});

	// transition image for shader access
	auto memoryBarrier2 = vk::ImageMemoryBarrier(
		vk::AccessFlagBits::eTransferWrite,
		vk::AccessFlagBits::eShaderRead,
		vk::ImageLayout::eTransferDstOptimal,
		vk::ImageLayout::eShaderReadOnlyOptimal,
		m_GraphicsQueueFamilyID,
		m_GraphicsQueueFamilyID,
		image2D.get(),
		vk::ImageSubresourceRange(
			vk::ImageAspectFlagBits::eColor,
			0U,
			1U,
			0U,
			1U));
	command.buffer->pipelineBarrier(
		vk::PipelineStageFlagBits::eTransfer,
		vk::PipelineStageFlagBits::eFragmentShader,
		vk::DependencyFlags(),
		{ },
		{ },
		{ memoryBarrier2 });

	command.buffer->end();

	// create fence, submit command buffer
	auto imageLoadFence = m_LogicalDevice->createFenceUnique(vk::FenceCreateInfo());
	m_GraphicsQueue.submit(
		vk::SubmitInfo(0U, nullptr, nullptr,
			1U, &command.buffer.get(),
			0U, nullptr),
		imageLoadFence.get());

	// create image view
	imageView = m_LogicalDevice->createImageViewUnique(
		vk::ImageViewCreateInfo(
			vk::ImageViewCreateFlags(),
			image2D.get(),
			vk::ImageViewType::e2D,
			vk::Format::eR8G8B8A8Srgb,
			vk::ComponentMapping(),
			vk::ImageSubresourceRange(
				vk::ImageAspectFlagBits::eColor,
				0U,
				1U,
				0U,
				1U)));

	// wait for command buffer to be executed
	m_LogicalDevice->waitForFences({ imageLoadFence.get() }, true, std::numeric_limits<uint64_t>::max());

	auto fenceStatus = m_LogicalDevice->getFenceStatus(imageLoadFence.get());

	return textureIndex;
}

inline std::vector<vk::VertexInputBindingDescription> Vertex::vertexBindingDescriptions()
{
	auto bindingDescriptions = std::vector<vk::VertexInputBindingDescription>{ vk::VertexInputBindingDescription(0U, sizeof(Vertex), vk::VertexInputRate::eVertex) };
	return bindingDescriptions;
}

inline std::vector<vk::VertexInputAttributeDescription> Vertex::vertexAttributeDescriptions()
{
	// Binding both attributes to one buffer, interleaving Position and UV
	auto attrib0 = vk::VertexInputAttributeDescription(0U, 0U, vk::Format::eR32G32Sfloat, offsetof(Vertex, Position));
	auto attrib1 = vk::VertexInputAttributeDescription(1U, 0U, vk::Format::eR32G32Sfloat, offsetof(Vertex, UV));
	return std::vector<vk::VertexInputAttributeDescription>{ attrib0, attrib1 };
}

inline vk::UniqueShaderModule VulkanApp::createShaderModule(const vk::Device& device, const char* path)
{
	// read from file
	auto shaderBinary = fileIO::readFile(path);
	// create shader module
	auto shaderInfo = vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(),
		shaderBinary.size(),
		reinterpret_cast<const uint32_t*>(shaderBinary.data()));
	return device.createShaderModuleUnique(shaderInfo);
}

inline vk::UniqueDescriptorPool VulkanApp::createVertexDescriptorPool(const vk::Device& logicalDevice, uint32_t bufferCount)
{
	auto poolSizes = std::vector<vk::DescriptorPoolSize>
	{
		vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1U)
	};

	return logicalDevice.createDescriptorPoolUnique(
		vk::DescriptorPoolCreateInfo(
			vk::DescriptorPoolCreateFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet),
			bufferCount,
			gsl::narrow<uint32_t>(poolSizes.size()),
			poolSizes.data()));
}

inline vk::UniqueDescriptorPool VulkanApp::createFragmentDescriptorPool(const vk::Device& logicalDevice, uint32_t textureCount)
{
	auto poolSizes = std::vector<vk::DescriptorPoolSize>
	{
		vk::DescriptorPoolSize(vk::DescriptorType::eSampler, 1U),
		vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, textureCount)
	};

	return logicalDevice.createDescriptorPoolUnique(
		vk::DescriptorPoolCreateInfo(
			vk::DescriptorPoolCreateFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet),
			1U,
			gsl::narrow<uint32_t>(poolSizes.size()),
			poolSizes.data()));
}

inline vk::UniqueDescriptorSetLayout VulkanApp::createVertexSetLayout(const vk::Device& logicalDevice)
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
			gsl::narrow<uint32_t>(vertexLayoutBindings.size()),
			vertexLayoutBindings.data()));
}

inline vk::UniqueDescriptorSetLayout VulkanApp::createFragmentSetLayout(
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
			gsl::narrow<uint32_t>(fragmentLayoutBindings.size()),
			fragmentLayoutBindings.data()));
}

VulkanApp::Text::Text(VulkanApp* app) :
	m_VulkanApp(app)
{
	FT_Library temp;
	auto error = FT_Init_FreeType(&temp);
	if (error != 0)
	{
		std::cerr << "Error initializing FreeType.\n";
	}
	else
	{
		std::cout << "Initialized FreeType.\n";
	}
	m_FreeTypeLibrary = UniqueFTLibrary(temp);
}

VulkanApp::Text::SizeData& VulkanApp::Text::getSizeData(
	VulkanApp::Text::FontID fontID, 
	VulkanApp::Text::FontSize size)
{
	auto& fontData = m_FontMaps.at(fontID);
	return fontData.glyphsBySize.at(size);
}

Bitmap VulkanApp::Text::getFullBitmap(FT_Glyph glyph)
{
	auto bitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(glyph);
	auto& sourceBitmap = bitmapGlyph->bitmap;
	constexpr auto channelCount = 4U;
	auto outputSize = sourceBitmap.width * sourceBitmap.rows * channelCount;

	// Have to flip the bitmap so bottom row in source is top in output
	std::vector<unsigned char> pixels;
	pixels.reserve(outputSize);
	for (int sourceRow = sourceBitmap.rows - 1; sourceRow >= 0; sourceRow--)
	{
		for (size_t sourceColumn = 0; sourceColumn < sourceBitmap.width; sourceColumn++)
		{
			size_t sourcePosition = (sourceRow * sourceBitmap.pitch) + sourceColumn;

			auto source = sourceBitmap.buffer;
			pixels.push_back(255);
			pixels.push_back(255);
			pixels.push_back(255);
			pixels.push_back(source[sourcePosition]);
		}
	}
	FT_BBox box;
	FT_Glyph_Get_CBox(glyph, FT_GLYPH_BBOX_PIXELS, &box);

	Bitmap resultBitmap = Bitmap(
		reinterpret_cast<unsigned char*>(pixels.data()), 
		sourceBitmap.width, 
		sourceBitmap.rows, 
		box.xMin,
		box.yMax,
		outputSize
	);
	return resultBitmap;
}

std::optional<FT_Glyph> VulkanApp::Text::getGlyph(
	VulkanApp::Text::FontID fontID, 
	VulkanApp::Text::FontSize size, 
	VulkanApp::Text::CharCode charcode)
{
	SizeData& sizeData = getSizeData(fontID, size);
	auto it = sizeData.glyphMap.find(charcode);
	std::optional<FT_Glyph> glyphOptional;
	if (it != sizeData.glyphMap.end())
	{
		glyphOptional = it.operator*().second.glyph.get();
	}
	return glyphOptional;
}

auto VulkanApp::Text::getAdvance(
	VulkanApp::Text::FontID fontID, 
	VulkanApp::Text::FontSize size, 
	VulkanApp::Text::CharCode left)
{
	auto glyphOptional = getGlyph(fontID, size, left);
	if (glyphOptional)
	{
		// 16.16 fixed point to 32 int
		return static_cast<FT_Int32>((*glyphOptional)->advance.x >> 16);
	}
	else
	{
		// 16.16 fixed point to 32 int
		auto advance = getSizeData(fontID, size).spaceAdvance;
		auto advanceShifted = advance >> 6;
		return static_cast<FT_Int32>(advanceShifted);
	}
}

auto VulkanApp::Text::getAdvance(
	VulkanApp::Text::FontID fontID, 
	VulkanApp::Text::FontSize size, 
	VulkanApp::Text::CharCode left, 
	VulkanApp::Text::CharCode right)
{
	auto advance = getAdvance(fontID, size, left);
	auto face = m_FontMaps[fontID].face;
	GlyphID leftGlyph = FT_Get_Char_Index(face, left);
	GlyphID rightGlyph = FT_Get_Char_Index(face, right);
	FT_Vector kerning = { 0, 0 };
	if (FT_HAS_KERNING(face))
	{
		FT_Get_Kerning(face, leftGlyph, rightGlyph, FT_KERNING_DEFAULT, &kerning);
	}
	advance += kerning.x >> 6;
	return advance;
}

void VulkanApp::LoadFont(
	VulkanApp::Text::FontID fontID, 
	VulkanApp::Text::FontSize fontSize, 
	uint32_t DPI, 
	const char* fontPath)
{
	m_Text.LoadFont(fontID, fontSize, DPI, fontPath);
}

void VulkanApp::Text::LoadFont(
	VulkanApp::Text::FontID fontID, 
	VulkanApp::Text::FontSize fontSize, 
	uint32_t DPI, 
	const char* fontPath)
{
	VulkanApp::Text::FontData& fontData = m_FontMaps[fontID];
	fontData.path = fontPath;
	auto newFaceError = FT_New_Face(m_FreeTypeLibrary.get(), fontPath, 0, &fontData.face);
	auto setSizeError = FT_Set_Char_Size(
		fontData.face, 
		gsl::narrow_cast<FT_F26Dot6>(0), 
		gsl::narrow_cast<FT_F26Dot6>(fontSize * 64), 
		gsl::narrow<FT_UInt>(DPI), 
		gsl::narrow<FT_UInt>(0U));

	VulkanApp::Text::SizeData& sizeData = fontData.glyphsBySize[fontSize];
	auto loadError = FT_Load_Char(fontData.face, 'r', FT_LOAD_ADVANCE_ONLY);
	if (loadError)
	{
		std::runtime_error("Error loading r character (used to calculate space character size)");
	}
	sizeData.spaceAdvance = fontData.face->glyph->advance.x;

	FT_UInt glyphID;
	FT_ULong charcode = FT_Get_First_Char(fontData.face, &glyphID);

	while (glyphID)
	{
		// load and render glyph
		loadError = FT_Load_Glyph(fontData.face, glyphID, FT_LOAD_RENDER);
		if (loadError)
		{
			std::runtime_error("Error loading or rendering glyph.");
		}
		FT_Glyph glyph;
		auto getError = FT_Get_Glyph(fontData.face->glyph, &glyph);
		if (getError)
		{
			std::runtime_error("Error copying glyph from face.");
		}
		Bitmap fullBitmap = getFullBitmap(glyph);
		if (fullBitmap.m_Size == 0)
		{
			charcode = FT_Get_Next_Char(fontData.face, charcode, &glyphID);
			continue;
		}
		auto texture = m_VulkanApp->createTexture(fullBitmap);
		auto& glyphData = sizeData.glyphMap[charcode];
		glyphData.glyph = UniqueFTGlyph(glyph);
		glyphData.textureIndex = texture;

		charcode = FT_Get_Next_Char(fontData.face, charcode, &glyphID);
	}
}
std::vector<Sprite> VulkanApp::Text::createTextGroup(const VulkanApp::Text::InitInfo& initInfo)
{
	std::vector<Sprite> textGroup;
	float pen_x = gsl::narrow_cast<float>(initInfo.baseline_x);
	float pen_y = gsl::narrow_cast<float>(initInfo.baseline_y);
	static auto identityMatrix = glm::mat4(1.0f);

	// Get glyph map for font/size
	auto& fontData = m_FontMaps.at(initInfo.fontID);
	auto& sizeData = fontData.glyphsBySize.at(initInfo.fontSize);
	auto& glyphMap = sizeData.glyphMap;
	for (auto it = initInfo.text.begin(); it != initInfo.text.end(); ++it)
	{
		auto character = *it;

		// check if texture exists for character
		auto textureIterator = glyphMap.find(character);
		if (textureIterator == glyphMap.end())
		{
			// texture not found for character, use space instead
			pen_x += getAdvance(initInfo.fontID, initInfo.fontSize, character);
			continue;
		}

		// create entity and sprite
		auto& sprite = textGroup.emplace_back();
		sprite.textureIndex = textureIterator->second.textureIndex;
		sprite.transform = glm::translate(identityMatrix, glm::vec3(pen_x, pen_y, initInfo.depth));
		sprite.color = initInfo.textColor;

		// track entity in textgroup

		auto it_next = it + 1;
		FT_Int32 advance;
		if (it_next != initInfo.text.end())
		{
			advance = getAdvance(initInfo.fontID, initInfo.fontSize, character, *it_next);
		}
		else
		{
			advance = getAdvance(initInfo.fontID, initInfo.fontSize, character);
		}
		pen_x += advance;
	}
	return textGroup;
}


}