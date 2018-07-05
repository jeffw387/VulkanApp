#include "VulkanApp.hpp"

namespace vka
{
	void VulkanApp::CleanUpSwapchain()
	{
		vkDeviceWaitIdle(device);
		for (const auto& fb : framebuffers)
		{
			deviceOptional->DestroyFramebuffer(fb);
		}
		for (const auto& view : framebufferImageViews)
		{
			deviceOptional->DestroyImageView(view);
		}
		deviceOptional->DestroySwapchain(swapchain);
	}

	void VulkanApp::CreateSwapchain()
	{
		swapchain = StoreHandle(
			deviceOptional->CreateSwapchain(
				surface,
				surfaceOptional->GetFormat(),
				jsonConfigs.swapchainConfig),
			jsonConfigs.swapchainConfig,
			swapchains);

		uint32_t swapImageCount;
		vkGetSwapchainImagesKHR(device, swapchain, &swapImageCount, nullptr);
		framebufferImages.resize(swapImageCount);
		framebufferImageViews.resize(swapImageCount);
		framebuffers.resize(swapImageCount);
		vkGetSwapchainImagesKHR(device, swapchain, &swapImageCount, framebufferImages.data());

		for (auto i = 0U; i < swapImageCount; ++i)
		{
			auto& fbImage = framebufferImages[i];
			auto& fbView = framebufferImageViews[i];
			auto& fb = framebuffers[i];

			fbView = deviceOptional->CreateColorImageView2D(fbImage, surfaceOptional->GetFormat());
			fb = deviceOptional->CreateFramebuffer(renderPass, surfaceOptional->GetExtent(), fbView);
		}
	}

	void VulkanApp::CreateSurface()
	{
		surfaceOptional = SurfaceManager(instance, physicalDevice, window);
		surface = surfaceOptional->GetSurface();
	}

	void VulkanApp::Run(std::string vulkanInitJsonPath, std::string vertexShaderPath, std::string fragmentShaderPath)
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

		jsonConfigs.swapchainConfig = LoadJson("config/swapchainConfig.json");
		jsonConfigs.renderPass = LoadJson("config/renderPass.json");
		jsonConfigs.fragmentShader2D = LoadJson("config/fragmentShader2D.json");
		jsonConfigs.vertexShader2D = LoadJson("config/vertexShader2D.json");
		jsonConfigs.fragmentDescriptorSetLayout2D = LoadJson("config/fragmentDescriptorSetLayout2D.json");
		jsonConfigs.sampler2D = LoadJson("config/sampler2D.json");
		jsonConfigs.pipelineLayout2D = LoadJson("config/pipelineLayout2D.json");
		jsonConfigs.pipeline2D = LoadJson("config/pipeline2D.json");

		instanceOptional = Instance(instanceCreateInfo);
		instance = instanceOptional->GetInstance();

		debugCallbackUnique = InitDebugCallback(instanceOptional->GetInstance());

		physicalDevice = SelectPhysicalDevice(instance);

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

		FinalizeImageOrder();
		FinalizeSpriteOrder();

		Create2DVertexBuffer();

		sampler2D = StoreHandle(
			deviceOptional->CreateSampler(jsonConfigs.sampler2D),
			jsonConfigs.sampler2D,
			samplers);

		fragmentDescriptorSetLayout2D = StoreHandle(
			deviceOptional->CreateDescriptorSetLayout(
				jsonConfigs.fragmentDescriptorSetLayout2D,
				sampler2D),
			jsonConfigs.fragmentDescriptorSetLayout2D,
			descriptorSetLayouts);

		fragmentDescriptorPool2D = deviceOptional->CreateDescriptorPool(
			jsonConfigs.fragmentDescriptorSetLayout2D,
			1,
			true);

		auto fragmentDescriptorSets = deviceOptional->AllocateDescriptorSets(
			fragmentDescriptorPool2D,
			fragmentDescriptorSetLayout2D,
			1);
		fragmentDescriptorSet2D = fragmentDescriptorSets.at(0);

		vertexShader2D = StoreHandle(
			deviceOptional->CreateShaderModule(
				jsonConfigs.vertexShader2D),
			jsonConfigs.vertexShader2D,
			shaderModules);

		fragmentShader2D = StoreHandle(
			deviceOptional->CreateShaderModule(
				jsonConfigs.fragmentShader2D),
			jsonConfigs.fragmentShader2D,
			shaderModules);

		auto imageInfos = std::vector<VkDescriptorImageInfo>();
		uint32_t imageCount = gsl::narrow<uint32_t>(images2D.size());
		imageInfos.reserve(imageCount);
		for (auto &imagePair : images2D)
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
		samplerDescriptorWrite.dstSet = fragmentDescriptorSet2D;
		samplerDescriptorWrite.dstBinding = 1;
		samplerDescriptorWrite.dstArrayElement = 0;
		samplerDescriptorWrite.descriptorCount = gsl::narrow<uint32_t>(imageCount);
		samplerDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		samplerDescriptorWrite.pImageInfo = imageInfos.data();
		samplerDescriptorWrite.pBufferInfo = nullptr;
		samplerDescriptorWrite.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(device, 1, &samplerDescriptorWrite, 0, nullptr);

		renderCommandPool = deviceOptional->CreateCommandPool(graphicsQueueID, false, true);
		renderCommandBuffers = deviceOptional->AllocateCommandBuffers(renderCommandPool, SurfaceManager::BufferCount);
		renderCommandBufferExecutedFences.resize(SurfaceManager::BufferCount);
		imageRenderedSemaphores.resize(SurfaceManager::BufferCount);
		for (auto i = 0; i < SurfaceManager::BufferCount; ++i)
		{
			renderCommandBufferExecutedFences[i] = deviceOptional->CreateFence(true);
			imageRenderedSemaphores[i] = deviceOptional->CreateSemaphore();
		}

		SetClearColor(0.f, 0.f, 0.f, 0.f);

		renderPass = StoreHandle(
			deviceOptional->CreateRenderPass(
				jsonConfigs.renderPass),
			jsonConfigs.renderPass,
			renderPasses);

		CreateSwapchain();

		pipelineLayout2D = StoreHandle(
			deviceOptional->CreatePipelineLayout(
				fragmentDescriptorSetLayout2D,
				jsonConfigs.pipelineLayout2D),
			jsonConfigs.pipelineLayout2D,
			pipelineLayouts);

		std::map<VkShaderModule, gsl::span<gsl::byte>> specData;
		specData[fragmentShader2D] = gsl::make_span((gsl::byte*)&imageCount, sizeof(uint32_t));

		pipeline2D = StoreHandle(
			deviceOptional->CreateGraphicsPipeline(
				pipelineLayout2D,
				renderPass,
				shaderModules,
				specData,
				jsonConfigs.pipeline2D),
			jsonConfigs.pipeline2D,
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

		auto &model = models3D[fileName];

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

	void VulkanApp::LoadImage2D(const HashType imageID, const Bitmap &bitmap)
	{
		images2D[imageID] = CreateImage2D(
			device,
			utilityCommandBuffer,
			utilityCommandFence,
			deviceOptional->GetAllocator(),
			bitmap,
			deviceOptional->GetGraphicsQueueID(),
			deviceOptional->GetGraphicsQueue());
	}

	void VulkanApp::CreateSprite(const HashType imageID, const HashType spriteName, const Quad quad)
	{
		Sprite sprite;
		sprite.imageID = imageID;
		sprite.quad = quad;
		sprites2D[spriteName] = sprite;
	}

	void VulkanApp::RenderSpriteInstance(
		uint64_t spriteIndex,
		glm::mat4 transform,
		glm::vec4 color)
	{
		// TODO: may need to change sprites.at() access to array index access for performance
		auto sprite = sprites2D.at(spriteIndex);
		auto &renderCommandBuffer = renderCommandBuffers.at(nextImage);
		auto vp = camera.getMatrix();
		auto m = transform;
		auto mvp = vp * m;

		PushConstants pushConstants;
		pushConstants.vertexPushConstants.mvp = mvp;
		pushConstants.fragmentPushConstants.imageOffset = gsl::narrow<glm::uint32>(sprite.imageOffset);
		pushConstants.fragmentPushConstants.color = color;

		vkCmdPushConstants(
			renderCommandBuffer,
			pipelineLayout2D,
			VK_SHADER_STAGE_VERTEX_BIT,
			offsetof(PushConstants, vertexPushConstants),
			sizeof(VertexPushConstants),
			&pushConstants.vertexPushConstants);

		vkCmdPushConstants(
			renderCommandBuffer,
			pipelineLayout2D,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			offsetof(PushConstants, fragmentPushConstants),
			sizeof(FragmentPushConstants),
			&pushConstants.fragmentPushConstants);

		// draw the sprite
		vkCmdDraw(renderCommandBuffer, VerticesPerQuad, 1, gsl::narrow<uint32_t>(sprite.vertexOffset), 0);
	}

	void VulkanApp::RenderModelInstance(uint64_t modelIndex, glm::mat4 transform, glm::vec4 color)
	{
	}

	void VulkanApp::FinalizeImageOrder()
	{
		auto imageOffset = 0;
		for (auto &image : images2D)
		{
			image.second.imageOffset = imageOffset;
			++imageOffset;
		}
	}

	void VulkanApp::FinalizeSpriteOrder()
	{
		auto spriteOffset = 0;
		for (auto &[spriteID, sprite] : sprites2D)
		{
			quads2D.push_back(sprite.quad);
			sprite.vertexOffset = spriteOffset;
			sprite.imageOffset = images2D[sprite.imageID].imageOffset;
			++spriteOffset;
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

	void VulkanApp::Create2DVertexBuffer()
	{
		auto graphicsQueueFamilyID = deviceOptional->GetGraphicsQueueID();
		auto graphicsQueue = deviceOptional->GetGraphicsQueue();

		if (quads2D.size() == 0)
		{
			std::runtime_error("Error: no vertices loaded.");
		}
		// create vertex buffers

		vertexBuffer2D = CreateVertexBufferStageData(device,
			deviceOptional->GetAllocator(),
			graphicsQueueFamilyID,
			graphicsQueue,
			quads2D,
			BufferType::Vertex,
			utilityCommandBuffer,
			utilityCommandFence);
	}

	void VulkanApp::Create3DVertexBuffers()
	{
		auto graphicsQueueFamilyID = deviceOptional->GetGraphicsQueueID();
		auto graphicsQueue = deviceOptional->GetGraphicsQueue();

		if (models3D.size() == 0)
		{
			std::runtime_error("Error: no vertices loaded.");
		}
		size_t indexCount = 0;
		size_t vertexCount = 0;
		for (auto &[id, model] : models3D)
		{
			model.full.firstIndex = indexCount;
			model.full.firstVertex = vertexCount;

			auto newIndicesCount = model.full.indices.size();
			auto newVerticesCount = model.full.positions.size();
			indexCount += newIndicesCount;
			vertexCount += newVerticesCount;

			vertexIndices3D.insert(std::end(vertexIndices3D),
				model.full.indices.begin(),
				model.full.indices.end());

			vertexPositions3D.insert(std::end(vertexPositions3D),
				model.full.positions.begin(),
				model.full.positions.end());

			vertexNormals3D.insert(std::end(vertexNormals3D),
				model.full.normals.begin(),
				model.full.normals.end());
		}

		auto indexBufferSize = vertexIndices3D.size() * sizeof(IndexType);
		auto positionBufferSize = vertexPositions3D.size() * sizeof(PositionType);
		auto normalBufferSize = vertexNormals3D.size() * sizeof(NormalType);
		auto& allocator = deviceOptional->GetAllocator();

		indexBuffer3D = CreateVertexBufferStageData<IndexType>(device,
			allocator,
			graphicsQueueFamilyID,
			graphicsQueue,
			vertexIndices3D,
			BufferType::Index,
			utilityCommandBuffer,
			utilityCommandFence);

		positionBuffer3D = CreateVertexBufferStageData<PositionType>(device,
			allocator,
			graphicsQueueFamilyID,
			graphicsQueue,
			vertexPositions3D,
			BufferType::Vertex,
			utilityCommandBuffer,
			utilityCommandFence);

		normalBuffer3D = CreateVertexBufferStageData<NormalType>(device,
			allocator,
			graphicsQueueFamilyID,
			graphicsQueue,
			vertexNormals3D,
			BufferType::Vertex,
			utilityCommandBuffer,
			utilityCommandFence);
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
				FrameRender(
					device,
					nextImage,
					swapchain,
					imagePresentedFencePool,
					[this]() { return deviceOptional->CreateFence(true); },
					renderCommandBuffers,
					renderCommandBufferExecutedFences,
					framebuffers,
					[this]() { Draw(); },
					imageRenderedSemaphores,
					renderPass,
					fragmentDescriptorSet2D,
					pipelineLayout2D,
					pipelineLayout3D,
					pipeline2D,
					pipeline3D,
					vertexBuffer2D.buffer.get(),
					indexBuffer3D.buffer.get(),
					positionBuffer3D.buffer.get(),
					normalBuffer3D.buffer.get(),
					deviceOptional->GetGraphicsQueue(),
					surfaceOptional->GetExtent(),
					clearValue);
			}
			catch (Results::ErrorDeviceLost)
			{
				return;
			}
			catch (Results::ErrorSurfaceLost)
			{
				CreateSurface();
				CleanUpSwapchain();
				CreateSwapchain();
				UpdateCameraSize();
			}
			catch (Results::Suboptimal)
			{
				CleanUpSwapchain();
				CreateSwapchain();
				UpdateCameraSize();
			}
			catch (Results::ErrorOutOfDate)
			{
				CleanUpSwapchain();
				CreateSwapchain();
				UpdateCameraSize();
			}
		}
	}

	void VulkanApp::UpdateCameraSize()
	{
		auto &extent = surfaceOptional->GetExtent();
		camera.setSize(glm::vec2(static_cast<float>(extent.width), static_cast<float>(extent.height)));
	}
} // namespace vka