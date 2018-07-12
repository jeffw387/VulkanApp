#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "VulkanApp.hpp"

namespace vka
{
	void VulkanApp::CleanUpSwapchain()
	{
		vkDeviceWaitIdle(device);
		for (const auto& imageResources : perImageResources)
		{
			deviceOptional->DestroyFramebuffer(imageResources.swap.framebuffer);
			deviceOptional->DestroyImageView(imageResources.swap.view);
		}
		deviceOptional->DestroySwapchain(swapchain);
	}

	void VulkanApp::CreateSwapchain()
	{
		swapchain = StoreHandle(
			deviceOptional->CreateSwapchain(
				surface,
				surfaceFormat,
				configs.swapchain),
			configs.swapchain,
			swapchains);

		uint32_t swapImageCount;
		vkGetSwapchainImagesKHR(device, swapchain, &swapImageCount, nullptr);
		std::vector<VkImage> swapImages;
		swapImages.resize(BufferCount);
		vkGetSwapchainImagesKHR(device, swapchain, &swapImageCount, swapImages.data());

		for (auto i = 0U; i < BufferCount; ++i)
		{
			auto& fbImage = perImageResources[i].swap.image;
			auto& fbView = perImageResources[i].swap.view;
			auto& fb = perImageResources[i].swap.framebuffer;

			fbImage = swapImages[i];
			fbView = deviceOptional->CreateColorImageView2D(fbImage, surfaceFormat.format);
			fb = deviceOptional->CreateFramebuffer(renderPass, surfaceExtent, fbView);
		}
	}

	void VulkanApp::CreateSurface()
	{
		VkSurfaceKHR surface;
		glfwCreateWindowSurface(instance, window, nullptr, &surface);
		surfaceUnique = VkSurfaceKHRUnique(surface, VkSurfaceKHRDeleter(instance));

		
	}

	void VulkanApp::UpdateSurfaceSize()
	{
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);

		surfaceExtent = surfaceCapabilities.currentExtent;

		viewport.x = 0.f;
		viewport.y = 0.f;
		viewport.width = static_cast<float>(surfaceExtent.width);
		viewport.height = static_cast<float>(surfaceExtent.height);
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;

		scissorRect.offset.x = 0;
		scissorRect.offset.y = 0;
		scissorRect.extent = surfaceExtent;
	}

	void VulkanApp::Run(std::string vulkanInitJsonPath)
	{
		json vulkanInitData = json::parse(fileIO::readFile(vulkanInitJsonPath));
		VulkanLibrary = LoadVulkanLibrary();
		LoadExportedEntryPoints(VulkanLibrary);
		LoadGlobalLevelEntryPoints();

		auto glfwInitSuccess = glfwInit();
		if (!glfwInitSuccess)
		{
			std::runtime_error("Error initializing GLFW.");
			exit(1);
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		int width = vulkanInitData["DefaultWindowSize"]["Width"];
		int height = vulkanInitData["DefaultWindowSize"]["Height"];
		std::string appName = vulkanInitData["ApplicationName"];
		window = glfwCreateWindow(width, height, appName.c_str(), NULL, NULL);
		if (window == NULL)
		{
			std::runtime_error("Error creating GLFW window.");
			exit(1);
		}
		glfwSetWindowUserPointer(window, this);
		glfwSetKeyCallback(window, KeyCallback);
		glfwSetCharCallback(window, CharacterCallback);
		glfwSetCursorPosCallback(window, CursorPositionCallback);
		glfwSetMouseButtonCallback(window, MouseButtonCallback);
		camera.setSize({ static_cast<float>(width),
						static_cast<float>(height) });

		std::vector<std::string> globalLayers;
		std::vector<std::string> instanceExtensions;
		std::vector<std::string> deviceExtensions;

		if (!ReleaseMode)
		{
			for (const std::string &layer : vulkanInitData["GlobalLayers"]["Debug"])
			{
				globalLayers.push_back(layer);
			}
			for (const std::string &extension : vulkanInitData["InstanceExtensions"]["Debug"])
			{
				instanceExtensions.push_back(extension);
			}
			for (const std::string &extension : vulkanInitData["DeviceExtensions"]["Debug"])
			{
				deviceExtensions.push_back(extension);
			}
		}
		for (const std::string &layer : vulkanInitData["GlobalLayers"]["Constant"])
		{
			globalLayers.push_back(layer);
		}
		for (const std::string &extension : vulkanInitData["InstanceExtensions"]["Constant"])
		{
			instanceExtensions.push_back(extension);
		}
		for (const std::string &extension : vulkanInitData["DeviceExtensions"]["Constant"])
		{
			deviceExtensions.push_back(extension);
		}

		std::vector<const char *> globalLayersCstrings;
		std::vector<const char *> instanceExtensionsCstrings;
		std::vector<const char *> deviceExtensionsCstrings;

		auto stringConvert = [](const std::string &s) { return s.c_str(); };

		std::transform(globalLayers.begin(),
			globalLayers.end(),
			std::back_inserter(globalLayersCstrings),
			stringConvert);

		std::transform(instanceExtensions.begin(),
			instanceExtensions.end(),
			std::back_inserter(instanceExtensionsCstrings),
			stringConvert);

		std::transform(deviceExtensions.begin(),
			deviceExtensions.end(),
			std::back_inserter(deviceExtensionsCstrings),
			stringConvert);

		int appVersionMajor = vulkanInitData["ApplicationVersion"]["Major"];
		int appVersionMinor = vulkanInitData["ApplicationVersion"]["Minor"];
		int appVersionPatch = vulkanInitData["ApplicationVersion"]["Patch"];
		std::string engineName = vulkanInitData["EngineName"];
		int engineVersionMajor = vulkanInitData["EngineVersion"]["Major"];
		int engineVersionMinor = vulkanInitData["EngineVersion"]["Minor"];
		int engineVersionPatch = vulkanInitData["EngineVersion"]["Patch"];
		int apiVersionMajor = vulkanInitData["APIVersion"]["Major"];
		int apiVersionMinor = vulkanInitData["APIVersion"]["Minor"];
		int apiVersionPatch = vulkanInitData["APIVersion"]["Patch"];
		VkApplicationInfo applicationInfo = {};
		applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		applicationInfo.pApplicationName = appName.c_str();
		applicationInfo.applicationVersion = VK_MAKE_VERSION(appVersionMajor, appVersionMinor, appVersionPatch);
		applicationInfo.pEngineName = engineName.c_str();
		applicationInfo.engineVersion = VK_MAKE_VERSION(engineVersionMajor, engineVersionMinor, engineVersionPatch);
		applicationInfo.apiVersion = VK_MAKE_VERSION(apiVersionMajor, apiVersionMinor, apiVersionPatch);

		VkInstanceCreateInfo instanceCreateInfo = {};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pApplicationInfo = &applicationInfo;
		instanceCreateInfo.enabledLayerCount = gsl::narrow<uint32_t>(globalLayersCstrings.size());
		instanceCreateInfo.ppEnabledLayerNames = globalLayersCstrings.data();
		instanceCreateInfo.enabledExtensionCount = gsl::narrow<uint32_t>(instanceExtensionsCstrings.size());
		instanceCreateInfo.ppEnabledExtensionNames = instanceExtensionsCstrings.data();

		configs.swapchain = LoadJson("config/swapchainConfig.json");
		configs.renderPass = LoadJson("config/renderPass.json");

		configs.c2D.sampler = LoadJson("config/2D/sampler.json");
		configs.c2D.pipelineLayout = LoadJson("config/2D/pipelineLayout.json");
		configs.c2D.pipeline = LoadJson("config/2D/pipeline.json");

		configs.c3D.pipelineLayout = LoadJson("config/3D/pipelineLayout.json");
		configs.c3D.pipeline = LoadJson("config/3D/pipeline.json");

		instanceOptional = Instance(instanceCreateInfo);
		instance = instanceOptional->GetInstance();

		debugCallbackUnique = InitDebugCallback(instanceOptional->GetInstance());

		physicalDevice = SelectPhysicalDevice(instance);
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
		uniformBufferAlignment = physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;

		CreateSurface();

		deviceOptional = DeviceManager(physicalDevice, deviceExtensionsCstrings, surface);
		device = deviceOptional->GetDevice();

		auto graphicsQueueID = deviceOptional->GetGraphicsQueueID();
		utilityCommandPool = deviceOptional->CreateCommandPool(graphicsQueueID, true, true);
		auto utilityCommandBuffers = deviceOptional->AllocateCommandBuffers(utilityCommandPool, 1);
		utilityCommandBuffer = utilityCommandBuffers.at(0);

		utilityCommandFence = deviceOptional->CreateFence(false);

		LoadModels();
		LoadImages();

		CreateVertexBuffers2D();

		data2D.sampler = StoreHandle(
			deviceOptional->CreateSampler(configs.c2D.sampler),
			configs.c2D.sampler,
			samplers);

		data2D.fragmentDescriptorSetLayout = StoreHandle(
			deviceOptional->CreateDescriptorSetLayout(
				configs.fragmentDescriptorSetLayout2D,
				data2D.sampler),
			configs.fragmentDescriptorSetLayout2D,
			descriptorSetLayouts);

		data2D.fragmentDescriptorPool = deviceOptional->CreateDescriptorPool(
			configs.fragmentDescriptorSetLayout2D,
			1,
			true);

		auto fragmentDescriptorSets = deviceOptional->AllocateDescriptorSets(
			data2D.fragmentDescriptorPool,
			data2D.fragmentDescriptorSetLayout,
			1);
		data2D.fragmentDescriptorSet = fragmentDescriptorSets.at(0);

		data2D.vertexShader = StoreHandle(
			deviceOptional->CreateShaderModule(
				configs.vertexShader2D),
			configs.vertexShader2D,
			shaderModules);

		data2D.fragmentShader = StoreHandle(
			deviceOptional->CreateShaderModule(
				configs.fragmentShader2D),
			configs.fragmentShader2D,
			shaderModules);

		auto imageInfos = std::vector<VkDescriptorImageInfo>();
		uint32_t imageCount = gsl::narrow<uint32_t>(data2D.images.size());
		imageInfos.reserve(imageCount);
		for (auto &imagePair : data2D.images)
		{
			VkDescriptorImageInfo imageInfo = {};
			imageInfo.sampler = VK_NULL_HANDLE;
			imageInfo.imageView = imagePair.second.view.get();
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfos.push_back(imageInfo);
		}

		VkWriteDescriptorSet samplerDescriptorWrite = {};
		samplerDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		samplerDescriptorWrite.pNext = nullptr;
		samplerDescriptorWrite.dstSet = data2D.fragmentDescriptorSet;
		samplerDescriptorWrite.dstBinding = 1;
		samplerDescriptorWrite.dstArrayElement = 0;
		samplerDescriptorWrite.descriptorCount = gsl::narrow<uint32_t>(imageCount);
		samplerDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		samplerDescriptorWrite.pImageInfo = imageInfos.data();
		samplerDescriptorWrite.pBufferInfo = nullptr;
		samplerDescriptorWrite.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(device, 1, &samplerDescriptorWrite, 0, nullptr);

		renderCommandPool = deviceOptional->CreateCommandPool(graphicsQueueID, false, true);
		std::vector<VkCommandBuffer> renderCommandBuffers = 
			deviceOptional->AllocateCommandBuffers(renderCommandPool, BufferCount);
		for (auto i = 0; i < BufferCount; ++i)
		{
			perImageResources[i].renderCommandBufferExecutedFence = deviceOptional->CreateFence(true);
			perImageResources[i].imageRenderedSemaphore = deviceOptional->CreateSemaphore();
		}

		SetClearColor(0.f, 0.f, 0.f, 0.f);

		renderPass = StoreHandle(
			deviceOptional->CreateRenderPass(
				surfaceFormat,
				configs.renderPass),
			configs.renderPass,
			renderPasses);

		CreateSwapchain();

		data2D.pipelineLayout = StoreHandle(
			deviceOptional->CreatePipelineLayout(
				data2D.fragmentDescriptorSetLayout,
				configs.pipelineLayout2D),
			configs.pipelineLayout2D,
			pipelineLayouts);

		std::map<VkShaderModule, gsl::span<gsl::byte>> specData;
		specData[data2D.fragmentShader] = gsl::make_span((gsl::byte*)&imageCount, sizeof(uint32_t));

		data2D.pipeline = StoreHandle(
			deviceOptional->CreateGraphicsPipeline(
				data2D.pipelineLayout,
				renderPass,
				shaderModules,
				specData,
				configs.pipeline2D),
			configs.pipeline2D,
			pipelines);

		startupTimePoint = NowMilliseconds();
		currentSimulationTime = startupTimePoint;

		std::thread gameLoopThread(&VulkanApp::GameThread, this);

		// Event loop
		while (!glfwWindowShouldClose(window))
		{
			glfwWaitEvents();
		}
		gameLoop = false;
		gameLoopThread.join();
		vkDeviceWaitIdle(device);
	}

	void VulkanApp::LoadModelFromFile(std::string path, entt::HashedString fileName)
	{
		auto f = std::ifstream(path + std::string(fileName));
		json j;
		f >> j;

		detail::BufferVector buffers;

		for (const auto &buffer : j["buffers"])
		{
			std::string bufferFileName = buffer["uri"];
			buffers.push_back(fileIO::readFile(path + bufferFileName));
		}

		auto &model = data3D.models[fileName];

		for (const auto &node : j["nodes"])
		{
			if (node["name"] == "Collision")
			{
				detail::LoadMesh(model.collision, node, j, buffers);
			}
			else
			{
				detail::LoadMesh(model.full, node, j, buffers);
			}
		}
	}

	void VulkanApp::CreateImage2D(
		const HashType imageID, 
		const Bitmap &bitmap)
	{
		data2D.images[imageID] = vka::CreateImage2D(
			device,
			utilityCommandBuffer,
			utilityCommandFence,
			deviceOptional->GetAllocator(),
			bitmap,
			deviceOptional->GetGraphicsQueueID(),
			deviceOptional->GetGraphicsQueue());
	}

	void VulkanApp::CreateSprite(
		const HashType imageID, 
		const HashType spriteName, 
		const Quad quad)
	{
		Sprite sprite;
		sprite.imageID = imageID;
		sprite.quad = quad;
		data2D.sprites[spriteName] = sprite;
	}

	void VulkanApp::AcquireNextImage(VkFence & fence)
	{
		auto imagePresentedFence = imagePresentedFencePool.unpoolOrCreate(deviceOptional->CreateFence);
		auto acquireResult = vkAcquireNextImageKHR(device, swapchain,
			0, VK_NULL_HANDLE,
			imagePresentedFence, &nextImage);

		auto successResult = HandleRenderErrors(acquireResult);
		if (successResult == RenderResults::Return)
		{
			imagePresentedFencePool.pool(imagePresentedFence);
			return;
		}
	}

	void VulkanApp::BeginRenderPass(const uint32_t & instanceCount)
	{
		VkFence imagePresentedFence = 0;
		AcquireNextImage(imagePresentedFence);

		auto renderCommandBufferExecutedFence = perImageResources[nextImage].renderCommandBufferExecutedFence;
		auto renderCommandBuffer = perImageResources[nextImage].renderCommandBuffer;
		auto framebuffer = perImageResources[nextImage].swap.framebuffer;



		// Wait for this frame's render command buffer to finish executing
		vkWaitForFences(device,
			1,
			&renderCommandBufferExecutedFence,
			true, std::numeric_limits<uint64_t>::max());
		vkResetFences(device, 1, &renderCommandBufferExecutedFence);

		// record the command buffer
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;
		vkBeginCommandBuffer(renderCommandBuffer, &beginInfo);

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = nullptr;
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.framebuffer = framebuffer;
		renderPassBeginInfo.renderArea = scissorRect;
		renderPassBeginInfo.clearValueCount = 1;
		renderPassBeginInfo.pClearValues = &clearValue;
		vkCmdBeginRenderPass(renderCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void VulkanApp::BindPipeline2D()
	{
		auto renderCommandBuffer = perImageResources[nextImage].renderCommandBuffer;
		vkCmdBindPipeline(renderCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, data2D.pipeline);

		vkCmdSetViewport(renderCommandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(renderCommandBuffer, 0, 1, &scissorRect);

		VkDeviceSize vertexBufferOffset = 0;
		auto vertexBuffer = data2D.vertexBuffer.buffer.get();
		vkCmdBindVertexBuffers(
			renderCommandBuffer, 
			0, 
			1,
			&vertexBuffer,
			&vertexBufferOffset);

		std::array<VkDescriptorSet, 2> sets = { data2D.fragmentDescriptorSet, data2D.vertexDescriptorSet };

		// bind sampler and images uniforms
		vkCmdBindDescriptorSets(
			renderCommandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			data2D.pipelineLayout,
			0,
			2, 
			sets.data(),
			0, 
			nullptr);
	}

	void VulkanApp::BindPipeline3D()
	{
	}

	void VulkanApp::RenderModel(const uint64_t modelIndex, const glm::mat4 modelMatrix, const glm::vec4 modelColor)
	{
	}

	void VulkanApp::EndRenderPass()
	{
	}

	void VulkanApp::PresentImage()
	{
	}

	void VulkanApp::PrepareRender(uint32_t instanceCount)
	{
		if (perImageResources[nextImage].uniforms.matrices.dynamic.capacity < instanceCount)
		{
			VkDeviceSize alignment = 0;
			auto dynamicBufferSize = sizeof(glm::mat4) * 2;
			if (dynamicBufferSize < uniformBufferAlignment)
			{
				alignment = uniformBufferAlignment;
			}
			else
			{
				auto multiple = (float)dynamicBufferSize / (float)uniformBufferAlignment;
				alignment = (VkDeviceSize)std::ceil(multiple) * uniformBufferAlignment;
			}
			VkDeviceSize newSize = instanceCount * alignment;
			CreateMatrixBuffer(nextImage, newSize);
		}
	}

	enum class BufferType
	{
		Index,
		Vertex
	};

	template <typename T>
	static UniqueAllocatedBuffer CreateVertexBufferStageData(VkDevice device,
		Allocator &allocator,
		uint32_t graphicsQueueFamilyID,
		VkQueue graphicsQueue,
		std::vector<T>& data,
		BufferType bufferType,
		VkCommandBuffer commandBuffer,
		VkFence fence)
	{
		auto dataByteLength = data.size() * sizeof(T);
		auto stagingBuffer = CreateBufferUnique(
			device,
			allocator,
			dataByteLength,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			graphicsQueueFamilyID,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			true);

		VkBufferUsageFlags type = 0;
		if (bufferType == BufferType::Index)
		{
			type = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		}
		else
		{
			type = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		}
		auto buffer = CreateBufferUnique(
			device,
			allocator,
			dataByteLength,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT |
			type,
			graphicsQueueFamilyID,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			true);

		void *stagingPtr = nullptr;
		auto memoryMapResult = vkMapMemory(device,
			stagingBuffer.allocation.get().memory,
			stagingBuffer.allocation.get().offsetInDeviceMemory,
			stagingBuffer.allocation.get().size,
			0,
			&stagingPtr);

		std::memcpy(stagingPtr, data.data(), dataByteLength);
		vkUnmapMemory(device, stagingBuffer.allocation.get().memory);

		VkBufferCopy bufferCopy = {};
		bufferCopy.srcOffset = 0;
		bufferCopy.dstOffset = 0;
		bufferCopy.size = dataByteLength;

		CopyToBuffer(
			commandBuffer,
			graphicsQueue,
			stagingBuffer.buffer.get(),
			buffer.buffer.get(),
			bufferCopy,
			fence);

		// wait for vertex buffer copy to finish
		vkWaitForFences(device, 1, &fence, (VkBool32)true,
			std::numeric_limits<uint64_t>::max());
		vkResetFences(device, 1, &fence);

		return std::move(buffer);
	}

	void VulkanApp::CreateVertexBuffers2D()
	{
		auto graphicsQueueFamilyID = deviceOptional->GetGraphicsQueueID();
		auto graphicsQueue = deviceOptional->GetGraphicsQueue();

		if (data2D.quads.size() == 0)
		{
			std::runtime_error("Error: no vertices loaded.");
		}
		// create vertex buffers

		data2D.vertexBuffer = CreateVertexBufferStageData(device,
			deviceOptional->GetAllocator(),
			graphicsQueueFamilyID,
			graphicsQueue,
			data2D.quads,
			BufferType::Vertex,
			utilityCommandBuffer,
			utilityCommandFence);
	}

	void VulkanApp::CreateVertexBuffers3D()
	{
		auto graphicsQueueFamilyID = deviceOptional->GetGraphicsQueueID();
		auto graphicsQueue = deviceOptional->GetGraphicsQueue();

		if (data3D.models.size() == 0)
		{
			std::runtime_error("Error: no vertices loaded.");
		}
		size_t indexCount = 0;
		size_t vertexCount = 0;
		for (auto &[id, model] : data3D.models)
		{
			model.full.firstIndex = indexCount;
			model.full.firstVertex = vertexCount;

			auto newIndicesCount = model.full.indices.size();
			auto newVerticesCount = model.full.positions.size();
			indexCount += newIndicesCount;
			vertexCount += newVerticesCount;

			data3D.vertexIndices.insert(std::end(data3D.vertexIndices),
				model.full.indices.begin(),
				model.full.indices.end());

			data3D.vertexPositions.insert(std::end(data3D.vertexPositions),
				model.full.positions.begin(),
				model.full.positions.end());

			data3D.vertexNormals.insert(std::end(data3D.vertexNormals),
				model.full.normals.begin(),
				model.full.normals.end());
		}

		auto indexBufferSize = data3D.vertexIndices.size() * sizeof(IndexType);
		auto positionBufferSize = data3D.vertexPositions.size() * sizeof(PositionType);
		auto normalBufferSize = data3D.vertexNormals.size() * sizeof(NormalType);
		auto& allocator = deviceOptional->GetAllocator();

		data3D.indexBuffer = CreateVertexBufferStageData<IndexType>(device,
			allocator,
			graphicsQueueFamilyID,
			graphicsQueue,
			data3D.vertexIndices,
			BufferType::Index,
			utilityCommandBuffer,
			utilityCommandFence);

		data3D.positionBuffer = CreateVertexBufferStageData<PositionType>(device,
			allocator,
			graphicsQueueFamilyID,
			graphicsQueue,
			data3D.vertexPositions,
			BufferType::Vertex,
			utilityCommandBuffer,
			utilityCommandFence);

		data3D.normalBuffer = CreateVertexBufferStageData<NormalType>(device,
			allocator,
			graphicsQueueFamilyID,
			graphicsQueue,
			data3D.vertexNormals,
			BufferType::Vertex,
			utilityCommandBuffer,
			utilityCommandFence);
	}

	void VulkanApp::CreateMatrixBuffer(size_t imageIndex, VkDeviceSize newSize)
	{
		VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		if (deviceOptional->HostDeviceCombined())
		{
			memProps |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		}
		perImageResources[imageIndex].uniforms.matrices.dynamic.buffer = CreateBufferUnique(
			device,
			deviceOptional->GetAllocator(),
			newSize,
			VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			deviceOptional->GetGraphicsQueueID(),
			memProps,
			false);
	}

	void VulkanApp::SetClearColor(float r, float g, float b, float a)
	{
		VkClearColorValue clearColor;
		clearColor.float32[0] = r;
		clearColor.float32[1] = g;
		clearColor.float32[2] = b;
		clearColor.float32[3] = a;
		clearValue.color = clearColor;
	}

	void VulkanApp::GameThread()
	{
		while (gameLoop != false)
		{
			auto currentTime = NowMilliseconds();
			// Update simulation to be in sync with actual time
			size_t spriteCount = 0;
			while (currentTime - currentSimulationTime > UpdateDuration)
			{
				currentSimulationTime += UpdateDuration;

				Update(currentSimulationTime);
			}
			try
			{
				VkFence imagePresentedFence = 0;
				AcquireNextImage(nextImage, imagePresentedFence);
				FrameRender(
					device,
					nextImage,
					swapchain,
					imagePresentedFence,
					perImageResources[nextImage].renderCommandBuffer,
					perImageResources[nextImage].renderCommandBufferExecutedFence,
					perImageResources[nextImage].swap.framebuffer,
					perImageResources[nextImage].imageRenderedSemaphore,
					[this]() { Draw(); },
					renderPass,
					data2D.fragmentDescriptorSet,
					data2D.vertexDescriptorSet,
					data3D.vertexDescriptorSet,
					data2D.pipelineLayout,
					data3D.pipelineLayout,
					data2D.pipeline,
					data3D.pipeline,
					data2D.vertexBuffer.buffer.get(),
					data3D.indexBuffer.buffer.get(),
					data3D.positionBuffer.buffer.get(),
					data3D.normalBuffer.buffer.get(),
					deviceOptional->GetGraphicsQueue(),
					surfaceExtent,
					clearValue);
			}
			catch (Results::ErrorDeviceLost)
			{
				return;
			}
			catch (Results::ErrorSurfaceLost)
			{
				CreateSurface();
				UpdateSurfaceSize();
				CleanUpSwapchain();
				CreateSwapchain();
				UpdateCameraSize();
			}
			catch (Results::Suboptimal)
			{
				UpdateSurfaceSize();
				CleanUpSwapchain();
				CreateSwapchain();
				UpdateCameraSize();
			}
			catch (Results::ErrorOutOfDate)
			{
				UpdateSurfaceSize();
				CleanUpSwapchain();
				CreateSwapchain();
				UpdateCameraSize();
			}
		}
	}

	void VulkanApp::UpdateCameraSize()
	{
		camera.setSize(glm::vec2(static_cast<float>(surfaceExtent.width), static_cast<float>(surfaceExtent.height)));
	}
} // namespace vka