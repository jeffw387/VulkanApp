#include "VulkanApp.hpp"

namespace vka
{
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
    camera.setSize({static_cast<float>(width),
                    static_cast<float>(height)});

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

    instanceOptional = Instance(instanceCreateInfo);

    debugCallbackUnique = InitDebugCallback(instanceOptional->GetInstance());

    deviceOptional = Device(instanceOptional->GetInstance(),
                            window,
                            deviceExtensionsCstrings,
                            vertexShaderPath,
                            fragmentShaderPath);
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

    CreateVertexBuffer();

    auto imageInfos = std::vector<VkDescriptorImageInfo>();
    auto imageCount = images.size();
    imageInfos.reserve(imageCount);
    for (auto &imagePair : images)
    {
        VkDescriptorImageInfo imageInfo = {};
        imageInfo.sampler = VK_NULL_HANDLE;
        imageInfo.imageView = imagePair.second.view.get();
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos.push_back(imageInfo);
    }

    auto &fragmentDescriptorSetManager = deviceOptional.value().fragmentDescriptorSetOptional.value();
    auto fragmentDescriptorSets = fragmentDescriptorSetManager.AllocateSets(1);
    fragmentDescriptorSet = fragmentDescriptorSets.at(0);

    VkWriteDescriptorSet samplerDescriptorWrite = {};
    samplerDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    samplerDescriptorWrite.pNext = nullptr;
    samplerDescriptorWrite.dstSet = fragmentDescriptorSet;
    samplerDescriptorWrite.dstBinding = 1;
    samplerDescriptorWrite.dstArrayElement = 0;
    samplerDescriptorWrite.descriptorCount = gsl::narrow<uint32_t>(imageCount);
    samplerDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    samplerDescriptorWrite.pImageInfo = imageInfos.data();
    samplerDescriptorWrite.pBufferInfo = nullptr;
    samplerDescriptorWrite.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(device, 1, &samplerDescriptorWrite, 0, nullptr);

    renderCommandPool = deviceOptional->CreateCommandPool(graphicsQueueID, false, true);
    renderCommandBuffers = deviceOptional->AllocateCommandBuffers(renderCommandPool, Surface::BufferCount);
    for (auto i = 0; i < Surface::BufferCount; ++i)
    {
        renderCommandBufferExecutedFences[i] = deviceOptional->CreateFence(true);
        // imagePresentedFencePool.pool(deviceOptional->CreateFence(false));
        imageRenderedSemaphores[i] = deviceOptional->CreateSemaphore();
    }

    SetClearColor(0.f, 0.f, 0.f, 0.f);

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

void LoadModelFromFile(std::string path, std::string fileName)
{
	auto f = std::ifstream(path + fileName);
	json j;
	f >> j;

	detail::BufferVector buffers;

	for (const auto &buffer : j["buffers"])
	{
		std::string bufferFileName = buffer["uri"];
		buffers.push_back(fileIO::readFile(path + bufferFileName));
	}

	Model model;

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
    images[imageID] = CreateImage2D(
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
    sprites[spriteName] = sprite;
}

void VulkanApp::RenderSpriteInstance(
    uint64_t spriteIndex,
    glm::mat4 transform,
    glm::vec4 color)
{
    // TODO: may need to change sprites.at() access to array index access for performance
    auto sprite = sprites.at(spriteIndex);
    auto &renderCommandBuffer = renderCommandBuffers.at(nextImage);
    auto vp = camera.getMatrix();
    auto m = transform;
    auto mvp = vp * m;

    PushConstants pushConstants;
    pushConstants.vertexPushConstants.mvp = mvp;
    pushConstants.fragmentPushConstants.imageOffset = gsl::narrow<glm::uint32>(sprite.imageOffset);
    pushConstants.fragmentPushConstants.color = color;

    vkCmdPushConstants(renderCommandBuffer,
                       deviceOptional->GetPipelineLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT,
                       offsetof(PushConstants, vertexPushConstants),
                       sizeof(VertexPushConstants),
                       &pushConstants.vertexPushConstants);

    vkCmdPushConstants(renderCommandBuffer,
                       deviceOptional->GetPipelineLayout(),
                       VK_SHADER_STAGE_FRAGMENT_BIT,
                       offsetof(PushConstants, fragmentPushConstants),
                       sizeof(FragmentPushConstants),
                       &pushConstants.fragmentPushConstants);

    // draw the sprite
    vkCmdDraw(renderCommandBuffer, VerticesPerQuad, 1, gsl::narrow<uint32_t>(sprite.vertexOffset), 0);
}

void VulkanApp::FinalizeImageOrder()
{
    auto imageOffset = 0;
    for (auto &image : images)
    {
        image.second.imageOffset = imageOffset;
        ++imageOffset;
    }
}

void VulkanApp::FinalizeSpriteOrder()
{
    auto spriteOffset = 0;
    for (auto &[spriteID, sprite] : sprites)
    {
        quads.push_back(sprite.quad);
        sprite.vertexOffset = spriteOffset;
        sprite.imageOffset = images[sprite.imageID].imageOffset;
        ++spriteOffset;
    }
}

void VulkanApp::CreateVertexBuffer()
{
    auto graphicsQueueFamilyID = deviceOptional->GetGraphicsQueueID();
    auto graphicsQueue = deviceOptional->GetGraphicsQueue();

    if (quads.size() == 0)
    {
        std::runtime_error("Error: no vertices loaded.");
    }
    // create vertex buffers
    constexpr auto quadSize = sizeof(Quad);
    size_t vertexBufferSize = quadSize * quads.size();
    auto vertexStagingBufferUnique = CreateBufferUnique(
        device,
        deviceOptional->GetAllocator(),
        vertexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        graphicsQueueFamilyID,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        true);

    vertexBufferUnique = CreateBufferUnique(
        device,
        deviceOptional->GetAllocator(),
        vertexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        graphicsQueueFamilyID,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        true);

    void *vertexStagingData = nullptr;
    auto memoryMapResult = vkMapMemory(device,
                                       vertexStagingBufferUnique.allocation.get().memory,
                                       vertexStagingBufferUnique.allocation.get().offsetInDeviceMemory,
                                       vertexStagingBufferUnique.allocation.get().size,
                                       0,
                                       &vertexStagingData);

    memcpy(vertexStagingData, quads.data(), vertexBufferSize);
    vkUnmapMemory(device, vertexStagingBufferUnique.allocation.get().memory);

    VkBufferCopy bufferCopy = {};
    bufferCopy.srcOffset = 0;
    bufferCopy.dstOffset = 0;
    bufferCopy.size = vertexBufferSize;

    CopyToBuffer(
        utilityCommandBuffer,
        graphicsQueue,
        vertexStagingBufferUnique.buffer.get(),
        vertexBufferUnique.buffer.get(),
        bufferCopy,
        utilityCommandFence);

    // wait for vertex buffer copy to finish
    vkWaitForFences(device, 1, &utilityCommandFence, (VkBool32) true,
                    std::numeric_limits<uint64_t>::max());
    vkResetFences(device, 1, &utilityCommandFence);
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

                                                      std::vector<T> data,
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
            auto newFrame = FrameRender(*this);
        }
        catch (Results::ErrorDeviceLost)
        {
            return;
        }
        catch (Results::ErrorSurfaceLost result)
        {
            (*deviceOptional)(result);
            UpdateCameraSize();
        }
        catch (Results::Suboptimal result)
        {
            (*deviceOptional)(result);
            UpdateCameraSize();
        }
        catch (Results::ErrorOutOfDate result)
        {
            (*deviceOptional)(result);
            UpdateCameraSize();
        }
    }
}

VkFence VulkanApp::GetFenceFromImagePresentedPool()
{
    auto fence = imagePresentedFencePool.unpool();
    auto fenceCount = imagePresentedFencePool.size();
    if (fence.has_value() == false)
    {
        fence = deviceOptional->CreateFence(false);
    }
    return fence.value();
}

void VulkanApp::ReturnFenceToImagePresentedPool(VkFence fence)
{
    if (fence != VK_NULL_HANDLE)
    {
        imagePresentedFencePool.pool(fence);
    }
}

void VulkanApp::UpdateCameraSize()
{
    auto &extent = deviceOptional->GetSurfaceExtent();
    camera.setSize(glm::vec2(static_cast<float>(extent.width), static_cast<float>(extent.height)));
}

enum class RenderResults
{
    Continue,
    Return
};

static RenderResults HandleRenderErrors(VkResult result)
{
    switch (result)
    {
    // successes
    case VK_NOT_READY:
        return RenderResults::Return;
    case VK_SUCCESS:
        return RenderResults::Continue;

    // recoverable errors
    case VK_SUBOPTIMAL_KHR:
        throw Results::Suboptimal();
    case VK_ERROR_OUT_OF_DATE_KHR:
        throw Results::ErrorOutOfDate();
    case VK_ERROR_SURFACE_LOST_KHR:
        throw Results::ErrorSurfaceLost();

    // unrecoverable errors
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        throw Results::ErrorOutOfHostMemory();
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        throw Results::ErrorOutOfDeviceMemory();
    case VK_ERROR_DEVICE_LOST:
        throw Results::ErrorDeviceLost();
    }
    return RenderResults::Continue;
}

FrameRender::FrameRender(VulkanApp &app) : app(app)
{
    // Frame-independent resources
    device = app.deviceOptional->GetDevice();
    auto &nextImage = app.nextImage;
    swapchain = app.deviceOptional->GetSwapchain();
    imagePresentedFence = app.GetFenceFromImagePresentedPool();
    renderPass = app.deviceOptional->GetRenderPass();
    fragmentDescriptorSet = app.fragmentDescriptorSet;
    pipelineLayout = app.deviceOptional->GetPipelineLayout();
    pipeline = app.deviceOptional->GetPipeline();
    vertexBuffer = app.vertexBufferUnique.buffer.get();
    graphicsQueue = app.deviceOptional->GetGraphicsQueue();
    extent = app.deviceOptional->GetSurfaceExtent();
    clearValue = app.clearValue;

    auto acquireResult = vkAcquireNextImageKHR(device, swapchain,
                                               0, VK_NULL_HANDLE,
                                               imagePresentedFence, &nextImage);

    auto successResult = HandleRenderErrors(acquireResult);
    if (successResult == RenderResults::Return)
    {
        return;
    }

    // frame-dependent resources
    renderCommandBuffer = app.renderCommandBuffers[nextImage];
    renderCommandBufferExecutedFence = app.renderCommandBufferExecutedFences[nextImage];
    framebuffer = app.deviceOptional->GetFramebuffer(nextImage);
    imageRenderedSemaphore = app.imageRenderedSemaphores[nextImage];

    VkViewport viewport = {};
    viewport.x = 0.f;
    viewport.y = 0.f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;

    VkRect2D scissorRect = {};
    scissorRect.offset.x = 0;
    scissorRect.offset.y = 0;
    scissorRect.extent = extent;

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

    vkCmdBindPipeline(renderCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    vkCmdSetViewport(renderCommandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(renderCommandBuffer, 0, 1, &scissorRect);

    VkDeviceSize vertexBufferOffset = 0;
    vkCmdBindVertexBuffers(renderCommandBuffer, 0, 1,
                           &vertexBuffer,
                           &vertexBufferOffset);

    // bind sampler and images uniforms
    vkCmdBindDescriptorSets(renderCommandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout,
                            0,
                            1, &fragmentDescriptorSet,
                            0, nullptr);

    app.Draw();

    // Finish recording draw command buffer
    vkCmdEndRenderPass(renderCommandBuffer);
    vkEndCommandBuffer(renderCommandBuffer);

    // Submit draw command buffer
    auto stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pWaitDstStageMask = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &renderCommandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &imageRenderedSemaphore;

    vkWaitForFences(device, 1, &imagePresentedFence, true, std::numeric_limits<uint64_t>::max());
    vkResetFences(device, 1, &imagePresentedFence);

    auto drawSubmitResult = vkQueueSubmit(graphicsQueue, 1,
                                          &submitInfo, renderCommandBufferExecutedFence);

    // Present image
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &imageRenderedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &nextImage;
    presentInfo.pResults = nullptr;

    auto presentResult = vkQueuePresentKHR(
        graphicsQueue,
        &presentInfo);

    HandleRenderErrors(presentResult);
}

FrameRender::~FrameRender()
{
    app.ReturnFenceToImagePresentedPool(imagePresentedFence);
}
} // namespace vka