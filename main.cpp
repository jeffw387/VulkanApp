
#define STB_IMAGE_IMPLEMENTATION
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "main.hpp"

namespace Fonts {
// constexpr auto AeroviasBrasil =
// entt::HashedString("Content/Fonts/AeroviasBrasilNF.ttf");
}

auto ClientApp::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                             VkMemoryPropertyFlags memoryProperties,
                             bool dedicatedAllocation = true) {
  return vka::CreateAllocatedBuffer(
      vs.device, vs.allocator, size, usage,
      vs.queueFamilyIndices[get(Queues::Graphics)], memoryProperties,
      dedicatedAllocation);
};

void ClientApp::PrepareECS() {
  // 3D render views
  ecs.prepare<cmp::Transform, cmp::Material, cmp::Mesh>();
  ecs.prepare<cmp::Transform, cmp::Material, cmp::Mesh>();
  ecs.prepare<cmp::Transform, cmp::Material, cmp::Mesh>();
  ecs.prepare<cmp::Transform, cmp::Material, cmp::Mesh>();
  ecs.prepare<cmp::Transform, cmp::Material, cmp::Mesh>();

  triangle.set<cmp::Mesh>(get(Models::Triangle));
  triangle.set<cmp::Transform>(glm::mat4(1.f));
  triangle.set<cmp::Material>(get(Materials::Red));
  triangle.set<cmp::Drawable>();

  cube.set<cmp::Mesh>(get(Models::Cube));
  cube.set<cmp::Transform>(glm::mat4(1.f));
  cube.set<cmp::Material>(get(Materials::Red));
  cube.set<cmp::Drawable>();

  cylinder.set<cmp::Mesh>(get(Models::Cylinder));
  cylinder.set<cmp::Transform>(glm::mat4(1.f));
  cylinder.set<cmp::Material>(get(Materials::Red));
  cylinder.set<cmp::Drawable>();

  icosphereSub2.set<cmp::Mesh>(get(Models::IcosphereSub2));
  icosphereSub2.set<cmp::Transform>(glm::mat4(1.f));
  icosphereSub2.set<cmp::Material>(get(Materials::Red));
  icosphereSub2.set<cmp::Drawable>();

  pentagon.set<cmp::Mesh>(get(Models::Pentagon));
  pentagon.set<cmp::Transform>(glm::mat4(1.f));
  pentagon.set<cmp::Material>(get(Materials::Red));
  pentagon.set<cmp::Drawable>();
}

void ClientApp::InputThread() {
  std::thread updateThread(&ClientApp::UpdateThread, this);
  while (!exitInputThread) {
    glfwWaitEvents();
    if (glfwWindowShouldClose(window)) {
      exitInputThread = true;
    }
  }
  exitUpdateThread = true;
  updateThread.join();
  return;
}

void ClientApp::UpdateThread() {
  startupTimePoint = NowMilliseconds();
  currentSimulationTime = startupTimePoint;

  while (!exitUpdateThread) {
    auto currentTime = NowMilliseconds();
    while (currentTime - currentSimulationTime > UpdateDuration)
      Update(currentSimulationTime);
    Draw();
  }
  exitInputThread = true;
}

void ClientApp::Update(TimePoint_ms updateTime) {
  bool inputToProcess = true;
  while (inputToProcess) {
    auto messageOptional =
        is.inputBuffer.popFirstIf([updateTime](vka::InputMessage message) {
          return message.time < updateTime;
        });

    if (messageOptional.has_value()) {
      auto& message = *messageOptional;
      auto bindHash = is.inputBindMap[message.signature];
      auto stateVariant = is.inputStateMap[bindHash];
      vka::StateVisitor visitor;
      visitor.signature = message.signature;
      std::visit(visitor, stateVariant);
    } else {
      inputToProcess = false;
    }
  }
}

void ClientApp::SortComponents() {
  ecs.sort<cmp::Mesh>(
      [](const auto& lhs, const auto& rhs) { return lhs.index < rhs.index; });
  ecs.sort<cmp::Transform, cmp::Mesh>();
  ecs.sort<cmp::Material, cmp::Mesh>();
}

void ClientApp::BindPipeline3D(VkCommandBuffer& cmd,
                               VkDescriptorSet& staticSet) {
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    vs.pipelines[get(Pipelines::P3D)]);

  std::array<VkBuffer, 2> vertexBuffers3D = {vs.Position3D.bufferData.buffer,
                                             vs.Normal3D.bufferData.buffer};
  std::array<VkDeviceSize, 2> vertexBufferOffsets3D = {0, 0};
  vkCmdBindVertexBuffers(cmd, 0, 2, vertexBuffers3D.data(),
                         vertexBufferOffsets3D.data());

  vkCmdBindIndexBuffer(cmd, vs.Index3D.bufferData.buffer, 0,
                       vka::VulkanIndexType);

  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          vs.pipelineLayouts[get(PipelineLayouts::Primary)], 0,
                          1, &staticSet, 0, nullptr);
}

void ClientApp::PushConstants(VkCommandBuffer& cmd,
                              PushConstantData& pushData) {
  vkCmdPushConstants(cmd, vs.pipelineLayouts[get(PipelineLayouts::Primary)],
                     VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantData),
                     &pushData);
}

void ClientApp::Draw() {
  // acquire image
  // update matrix buffers
  // update matrix descriptor set if needed

  auto imageReadyFence = vs.imageReadyFencePool.unpool().value();
  auto acquireResult = vkAcquireNextImageKHR(vs.device, vs.swapchain, ~(0Ui64),
                                             0, imageReadyFence, &vs.nextImage);
  switch (acquireResult) {
    case VK_SUCCESS:
    case VK_SUBOPTIMAL_KHR:
      break;
    case VK_TIMEOUT:
    case VK_NOT_READY:
      vs.imageReadyFencePool.pool(imageReadyFence);
      return;
    case VK_ERROR_OUT_OF_DATE_KHR:
      recreateSwapchain();
    default:
      exitUpdateThread = true;
      return;
  }

  SortComponents();

  auto& bufferCapacity = vs.instanceBufferCapacities[vs.nextImage];
  auto& instanceBuffer = vs.instanceUniformBuffers[vs.nextImage];
  auto& staticSet = vs.staticSets[vs.nextImage];
  auto& dynamicSet = vs.dynamicSets[vs.nextImage];
  auto& framebuffer = vs.framebuffers[vs.nextImage];
  auto& commandPool = vs.renderCommandPools[vs.nextImage];
  auto& frameDataCopySemaphore = vs.frameDataCopySemaphores[vs.nextImage];
  auto& imageRenderSemaphore = vs.imageRenderSemaphores[vs.nextImage];

  vkWaitForFences(vs.device, 1, &imageReadyFence, true, ~(0Ui64));
  vkResetFences(vs.device, 1, &imageReadyFence);
  vs.imageReadyFencePool.pool(imageReadyFence);

  vkResetCommandPool(vs.device, commandPool, 0);
  auto allocateInfo = vka::commandBufferAllocateInfo(
      commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 2);
  std::array<VkCommandBuffer, 2> cmd;
  constexpr size_t copy = 0U;
  constexpr size_t render = 1U;
  vkAllocateCommandBuffers(vs.device, &allocateInfo, cmd.data());

  auto drawableView =
      ecs.view<cmp::Transform, cmp::Mesh, cmp::Material>(entt::persistent_t{});
  auto matrixCount = drawableView.size();
  auto requiredBufferSize = matrixCount * vs.instanceBuffersOffsetAlignment;
  bool needResize = matrixCount > bufferCapacity;
  if (needResize) {
    destroyStagedBuffer(instanceBuffer);

    instanceBuffer =
        createStagedBuffer(requiredBufferSize, BufferType::Uniform);
    bufferCapacity = matrixCount;
  }

  vka::BeginTransientCommandBuffer(cmd[render]);
  vka::BeginTransientCommandBuffer(cmd[copy]);

  BeginRenderPass(framebuffer, cmd[render]);

  BindPipeline3D(cmd[render], staticSet);

  PushConstantData pushData{};
  PushConstants(cmd[render], pushData);
  CameraUniform cameraUniformData{};
  cameraUniformData.projection = camera.getProjectionMatrix();
  cameraUniformData.view = camera.getViewMatrix();

  auto copyFunc = [&](void* mapPtr) {
    uint32_t byteOffset{};
    for (const auto& entity : drawableView) {
      auto& transformComponent = ecs.get<cmp::Transform>(entity);
      auto& meshComponent = ecs.get<cmp::Mesh>(entity);
      auto& materialComponent = ecs.get<cmp::Material>(entity);

      // push new constants if they changed from the last instance
      if (pushData.materialIndex != materialComponent.index) {
        pushData.materialIndex = materialComponent.index;
        PushConstants(cmd[render], pushData);
      }
      auto& model = vs.models[meshComponent.index].full;

      auto mvp = cameraUniformData.projection * cameraUniformData.view *
                 transformComponent.matrix;
      std::memcpy((gsl::byte*)mapPtr + byteOffset, &transformComponent.matrix,
                  sizeof(glm::mat4));
      std::memcpy((gsl::byte*)mapPtr + (byteOffset + sizeof ::glm::mat4), &mvp,
                  sizeof(glm::mat4));

      vkCmdBindDescriptorSets(cmd[render], VK_PIPELINE_BIND_POINT_GRAPHICS,
                              vs.pipelineLayouts[get(PipelineLayouts::Primary)],
                              2, 1, &dynamicSet, 1, &byteOffset);

      vkCmdDrawIndexed(cmd[render], static_cast<uint32_t>(model.indices.size()),
                       1, static_cast<int32_t>(model.firstIndex),
                       static_cast<int32_t>(model.firstVertex), 0);

      byteOffset += static_cast<uint32_t>(vs.instanceBuffersOffsetAlignment);
    }
  };

  stageCameraData(cmd[copy], &cameraUniformData);
  stageLightData(cmd[copy]);
  stageDataRecordCopy(cmd[copy], instanceBuffer, copyFunc);

  vkCmdEndRenderPass(cmd[render]);

  vkEndCommandBuffer(cmd[render]);
  vkEndCommandBuffer(cmd[copy]);

  std::array<VkSubmitInfo, 2> submits;
  submits[copy] = vka::CreateSubmitInfo(cmd[copy], gsl::span<VkSemaphore>{},
                                        nullptr, frameDataCopySemaphore);

  VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  submits[render] = vka::CreateSubmitInfo(cmd[render], frameDataCopySemaphore,
                                          &waitStage, imageRenderSemaphore);

  vka::QueueSubmit(vs.queues[get(Queues::Graphics)], 0, submits);

  auto presentInfo = VkPresentInfoKHR{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.pImageIndices = &vs.nextImage;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &vs.swapchain;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &imageRenderSemaphore;

  auto presentResult =
      vkQueuePresentKHR(vs.queues[get(Queues::Present)], &presentInfo);
}

void ClientApp::BeginRenderPass(VkFramebuffer& framebuffer,
                                VkCommandBuffer& cmd) {
  auto renderPassBeginInfo = vka::renderPassBeginInfo();
  renderPassBeginInfo.renderPass = vs.renderPass;
  renderPassBeginInfo.framebuffer = framebuffer;
  renderPassBeginInfo.clearValueCount = get(ClearValues::COUNT);
  renderPassBeginInfo.pClearValues = vs.clearValues.data();
  renderPassBeginInfo.renderArea = vs.scissor;
  vkCmdBeginRenderPass(cmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

  vkCmdSetViewport(cmd, 0, 1, &vs.viewport);

  vkCmdSetScissor(cmd, 0, 1, &vs.scissor);
}

void ClientApp::cleanup() {
  vkDeviceWaitIdle(vs.device);
  cleanupFrameResources();
  cleanupVertexBuffers();
  cleanupPipelines();
  cleanupShaderModules();
  cleanupPipelineLayout();
  cleanupDescriptorPools();
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

void ClientApp::createInstance() {
  auto appInfo = vka::applicationInfo();
  appInfo.pApplicationName = appName;
  appInfo.pEngineName = engineName;
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

  auto createInfo = vka::instanceCreateInfo();
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledLayerCount =
      gsl::narrow_cast<uint32_t>(instanceLayers.size());
  createInfo.ppEnabledLayerNames = instanceLayers.data();
  createInfo.enabledExtensionCount =
      gsl::narrow_cast<uint32_t>(instanceExtensions.size());
  createInfo.ppEnabledExtensionNames = instanceExtensions.data();
  vkCreateInstance(&createInfo, nullptr, &vs.instance);

  vka::LoadInstanceLevelEntryPoints(vs.instance);
}

void ClientApp::cleanupInstance() { vkDestroyInstance(vs.instance, nullptr); }

void ClientApp::selectPhysicalDevice() {
  auto physicalDevices = vka::GetPhysicalDevices(vs.instance);
  // TODO: error handling if no physical devices found
  vs.physicalDevice = physicalDevices.at(0);

  vs.physicalDeviceProperties = vka::GetProperties(vs.physicalDevice);
  if (vs.physicalDeviceProperties.deviceType ==
      VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
    vs.unifiedMemory = true;
  }
}

void ClientApp::selectUniformBufferOffsetAlignments() {
  vs.uniformBufferOffsetAlignment =
      vs.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;

  vs.materialsBufferOffsetAlignment = SelectUniformBufferOffset(
      sizeof(MaterialUniform), vs.uniformBufferOffsetAlignment);
  vs.lightsBuffersOffsetAlignment = SelectUniformBufferOffset(
      sizeof(LightUniform), vs.uniformBufferOffsetAlignment);
  vs.instanceBuffersOffsetAlignment = SelectUniformBufferOffset(
      sizeof(InstanceUniform), vs.uniformBufferOffsetAlignment);
}

void ClientApp::createDevice() {
  auto queueSearch = [](std::vector<VkQueueFamilyProperties>& properties,
                        VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                        VkQueueFlags flags, VkBool32 presentSupportNeeded) {
    std::optional<uint32_t> result;
    for (uint32_t i = 0; i < properties.size(); ++i) {
      const auto& prop = properties[i];
      auto flagMatch = ((flags & prop.queueFlags) == flags);
      if (presentSupportNeeded) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface,
                                             &presentSupport);
        if (flagMatch && presentSupport) {
          result = i;
          return result;
        }
      } else {
        if (flagMatch) {
          result = i;
          return result;
        }
      }
    }
    return result;
  };

  vs.familyProperties = vka::GetQueueFamilyProperties(vs.physicalDevice);

  vs.queueFamilyIndices[get(Queues::Graphics)] =
      queueSearch(vs.familyProperties, vs.physicalDevice, vs.surface,
                  VK_QUEUE_GRAPHICS_BIT, false)
          .value();

  vs.queueFamilyIndices[get(Queues::Present)] =
      queueSearch(vs.familyProperties, vs.physicalDevice, vs.surface, 0, true)
          .value();

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

  queueCreateInfos.push_back(vka::deviceQueueCreateInfo());
  auto& graphicsQueueCreateInfo = queueCreateInfos.back();
  graphicsQueueCreateInfo.queueFamilyIndex =
      vs.queueFamilyIndices[get(Queues::Graphics)];
  graphicsQueueCreateInfo.queueCount = 1;
  graphicsQueueCreateInfo.pQueuePriorities =
      &vs.queuePriorities[get(Queues::Graphics)];

  if (vs.queueFamilyIndices[get(Queues::Graphics)] !=
      vs.queuePriorities[get(Queues::Graphics)]) {
    queueCreateInfos.push_back(vka::deviceQueueCreateInfo());
    auto& presentQueueCreateInfo = queueCreateInfos.back();
    presentQueueCreateInfo.queueFamilyIndex =
        vs.queueFamilyIndices[get(Queues::Present)];
    presentQueueCreateInfo.queueCount = 1;
    presentQueueCreateInfo.pQueuePriorities =
        &vs.queuePriorities[get(Queues::Present)];
  }

  auto deviceCreateInfo = vka::deviceCreateInfo();
  deviceCreateInfo.enabledExtensionCount =
      gsl::narrow_cast<uint32_t>(deviceExtensions.size());
  deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
  deviceCreateInfo.queueCreateInfoCount =
      gsl::narrow_cast<uint32_t>(queueCreateInfos.size());
  deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
  vkCreateDevice(vs.physicalDevice, &deviceCreateInfo, nullptr, &vs.device);

  vka::LoadDeviceLevelEntryPoints(vs.device);

  vkGetDeviceQueue(vs.device, vs.queueFamilyIndices[get(Queues::Graphics)], 0,
                   &vs.queues[get(Queues::Graphics)]);
  vkGetDeviceQueue(vs.device, vs.queueFamilyIndices[get(Queues::Present)], 0,
                   &vs.queues[get(Queues::Present)]);
}

void ClientApp::createAllocator() {
  vs.allocator = vka::Allocator(vs.physicalDevice, vs.device);
}

void ClientApp::cleanupAllocator() { vs.allocator = vka::Allocator(); }

void ClientApp::cleanupDevice() { vkDestroyDevice(vs.device, nullptr); }

void ClientApp::createWindow(const char* title, uint32_t width,
                             uint32_t height) {
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window = glfwCreateWindow(width, height, title, nullptr, nullptr);
}

void ClientApp::cleanupWindow() { glfwDestroyWindow(window); }

void ClientApp::createUtilityResources() {
  auto poolCreateInfo = vka::commandPoolCreateInfo();
  poolCreateInfo.queueFamilyIndex =
      vs.queueFamilyIndices[get(Queues::Graphics)];
  poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  vkCreateCommandPool(vs.device, &poolCreateInfo, nullptr, &vs.utility.pool);

  auto allocateInfo = vka::commandBufferAllocateInfo(
      vs.utility.pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
  vkAllocateCommandBuffers(vs.device, &allocateInfo, &vs.utility.buffer);

  auto fenceCreateInfo = vka::fenceCreateInfo();
  vkCreateFence(vs.device, &fenceCreateInfo, nullptr, &vs.utility.fence);
}

void ClientApp::cleanupUtilityResources() {
  vkDestroyFence(vs.device, vs.utility.fence, nullptr);
  vkDestroyCommandPool(vs.device, vs.utility.pool, nullptr);
}

void ClientApp::createSurface() {
  glfwCreateWindowSurface(vs.instance, window, nullptr, &vs.surface);

  auto chooseSurfaceFormat =
      [](const std::vector<VkSurfaceFormatKHR> supportedFormats,
         VkFormat preferredFormat, VkColorSpaceKHR preferredColorSpace) {
        if (supportedFormats[0].format == VK_FORMAT_UNDEFINED) {
          return VkSurfaceFormatKHR{preferredFormat, preferredColorSpace};
        }
        for (const auto& surfFormat : supportedFormats) {
          if (surfFormat.format == preferredFormat &&
              surfFormat.colorSpace == preferredColorSpace) {
            return surfFormat;
          }
        }
        return supportedFormats[0];
      };

  vs.surfaceData.supportedFormats =
      vka::GetSurfaceFormats(vs.physicalDevice, vs.surface);
  vs.surfaceData.swapFormat = chooseSurfaceFormat(
      vs.surfaceData.supportedFormats, VK_FORMAT_R8G8B8A8_SNORM,
      VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);

  auto choosePresentMode = [](const std::vector<VkPresentModeKHR>& supported,
                              VkPresentModeKHR preferredPresentMode) {
    for (const auto& presentMode : supported) {
      if (presentMode == preferredPresentMode) return presentMode;
    }
    return supported[0];
  };

  vs.surfaceData.supportedPresentModes =
      vka::GetPresentModes(vs.physicalDevice, vs.surface);
  vs.surfaceData.presentMode = choosePresentMode(
      vs.surfaceData.supportedPresentModes, VK_PRESENT_MODE_FIFO_KHR);
}

void ClientApp::cleanupSurface() {
  vkDestroySurfaceKHR(vs.instance, vs.surface, nullptr);
}

void ClientApp::chooseSwapExtent() {
  auto capabilities =
      vka::GetSurfaceCapabilities(vs.physicalDevice, vs.surface);
  vs.surfaceData.extent = capabilities.currentExtent;

  camera.setSize(glm::vec2((float)vs.surfaceData.extent.width,
                           (float)vs.surfaceData.extent.height));
}

void ClientApp::createRenderPass() {
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
  depthAttachment.finalLayout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
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
  dep3Dto2D.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dep3Dto2D.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dep3Dto2D.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
  dep3Dto2D.srcSubpass = 0;
  dep3Dto2D.dstSubpass = 1;
  dep3Dto2D.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  auto createInfo = vka::renderPassCreateInfo();
  createInfo.attachmentCount =
      gsl::narrow_cast<uint32_t>(attachmentDescriptions.size());
  createInfo.pAttachments = attachmentDescriptions.data();
  createInfo.subpassCount =
      gsl::narrow_cast<uint32_t>(subpassDescriptions.size());
  createInfo.pSubpasses = subpassDescriptions.data();
  createInfo.dependencyCount = 1;
  createInfo.pDependencies = &dep3Dto2D;
  vkCreateRenderPass(vs.device, &createInfo, nullptr, &vs.renderPass);
}

void ClientApp::cleanupRenderPass() {
  vkDestroyRenderPass(vs.device, vs.renderPass, nullptr);
}

void ClientApp::createSwapchain() {
  std::vector<uint32_t> queueFamilyIndices;
  queueFamilyIndices.push_back(vs.queueFamilyIndices[get(Queues::Graphics)]);
  VkSharingMode imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if (vs.queueFamilyIndices[get(Queues::Graphics)] ==
      vs.queueFamilyIndices[get(Queues::Present)]) {
    queueFamilyIndices.push_back(vs.queueFamilyIndices[get(Queues::Present)]);
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
  createInfo.queueFamilyIndexCount =
      gsl::narrow_cast<uint32_t>(queueFamilyIndices.size());
  createInfo.pQueueFamilyIndices = queueFamilyIndices.data();

  vkCreateSwapchainKHR(vs.device, &createInfo, nullptr, &vs.swapchain);

  vs.depthImage = vka::CreateAllocatedImage2D(
      vs.device, vs.allocator, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      vs.surfaceData.depthFormat, vs.surfaceData.extent.width,
      vs.surfaceData.extent.height,
      vs.queueFamilyIndices[get(Queues::Graphics)], VK_IMAGE_ASPECT_DEPTH_BIT);

  auto swapImages = vka::GetSwapImages(vs.device, vs.swapchain);
  auto swapImageCount = BufferCount;
  vkGetSwapchainImagesKHR(vs.device, vs.swapchain, &swapImageCount,
                          vs.swapImages.data());
  for (auto i = 0U; i < BufferCount; ++i) {
    vs.swapViews[i] = vka::CreateImageView2D(vs.device, vs.swapImages[i],
                                             vs.surfaceData.swapFormat.format,
                                             VK_IMAGE_ASPECT_COLOR_BIT);

    std::array<VkImageView, 2> attachments = {vs.swapViews[i],
                                              vs.depthImage.view};

    auto framebufferCreateInfo = vka::framebufferCreateInfo();
    framebufferCreateInfo.renderPass = vs.renderPass;
    framebufferCreateInfo.attachmentCount = 2;
    framebufferCreateInfo.pAttachments = attachments.data();
    framebufferCreateInfo.layers = 1;
    framebufferCreateInfo.width = vs.surfaceData.extent.width;
    framebufferCreateInfo.height = vs.surfaceData.extent.height;
    vkCreateFramebuffer(vs.device, &framebufferCreateInfo, nullptr,
                        &vs.framebuffers[i]);
  }
}

void ClientApp::cleanupSwapchain() {
  vkDeviceWaitIdle(vs.device);
  vkDestroyImageView(vs.device, vs.depthImage.view, nullptr);
  vkDestroyImage(vs.device, vs.depthImage.image, nullptr);
  for (auto i = 0U; i < BufferCount; ++i) {
    vkDestroyFramebuffer(vs.device, vs.framebuffers[i], nullptr);
    vkDestroyImageView(vs.device, vs.swapViews[i], nullptr);
  }
  vkDestroySwapchainKHR(vs.device, vs.swapchain, nullptr);
}

void ClientApp::recreateSwapchain() {
  cleanupSwapchain();
  chooseSwapExtent();
  createSwapchain();
}

void ClientApp::createSampler() {
  auto createInfo = vka::samplerCreateInfo();
  createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  createInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
  createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  vkCreateSampler(vs.device, &createInfo, nullptr,
                  &vs.samplers[get(Samplers::Texture2D)]);
}

void ClientApp::cleanupSampler() {
  vkDestroySampler(vs.device, vs.samplers[get(Samplers::Texture2D)], nullptr);
}

void ClientApp::createDescriptorSetLayouts() {
  auto& samplerBinding = vs.staticLayoutBindings.emplace_back();
  samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  samplerBinding.binding = 0;
  samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
  samplerBinding.descriptorCount = 1;
  samplerBinding.pImmutableSamplers = &vs.samplers[get(Samplers::Texture2D)];

  auto& texturesBinding = vs.staticLayoutBindings.emplace_back();
  texturesBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  texturesBinding.binding = 1;
  texturesBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  texturesBinding.descriptorCount = std::max(ImageCount, 1U);

  auto& materialsBinding = vs.staticLayoutBindings.emplace_back();
  materialsBinding.binding = 2;
  materialsBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  materialsBinding.descriptorCount = std::max(get(Materials::COUNT), 1U);
  materialsBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  auto& cameraBinding = vs.staticLayoutBindings.emplace_back();
  cameraBinding.binding = 3;
  cameraBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  cameraBinding.descriptorCount = 1;
  cameraBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  auto& lightsBinding = vs.staticLayoutBindings.emplace_back();
  lightsBinding.binding = 4;
  lightsBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  lightsBinding.descriptorCount = LightCount;
  lightsBinding.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

  auto& instanceBinding = vs.instanceLayoutBindings.emplace_back();
  instanceBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  instanceBinding.binding = 0;
  instanceBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  instanceBinding.descriptorCount = 1;

  auto staticLayoutCreateInfo =
      vka::descriptorSetLayoutCreateInfo(vs.staticLayoutBindings);
  auto drawLayoutCreateInfo =
      vka::descriptorSetLayoutCreateInfo(vs.instanceLayoutBindings);

  vkCreateDescriptorSetLayout(vs.device, &staticLayoutCreateInfo, nullptr,
                              &vs.setLayouts[get(SetLayouts::Static)]);
  vkCreateDescriptorSetLayout(vs.device, &drawLayoutCreateInfo, nullptr,
                              &vs.setLayouts[get(SetLayouts::Dynamic)]);

  std::fill(std::begin(vs.staticSetLayouts), std::end(vs.staticSetLayouts),
            vs.setLayouts[get(SetLayouts::Static)]);

  std::fill(std::begin(vs.dynamicSetLayouts), std::end(vs.dynamicSetLayouts),
            vs.setLayouts[get(SetLayouts::Dynamic)]);
}

void ClientApp::cleanupDescriptorSetLayouts() {
  for (auto& layout : vs.setLayouts) {
    vkDestroyDescriptorSetLayout(vs.device, layout, nullptr);
  }
}

void ClientApp::createPushRanges() {
  vs.pushRanges[get(PushRanges::Primary)] = vka::pushConstantRange(
      VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PushConstantData), 0);
}

void ClientApp::createPipelineLayout() {
  auto createInfo = vka::pipelineLayoutCreateInfo(
      vs.setLayouts.data(), gsl::narrow_cast<uint32_t>(vs.setLayouts.size()));
  createInfo.pushConstantRangeCount = get(PushRanges::COUNT);
  createInfo.pPushConstantRanges = vs.pushRanges.data();
  vkCreatePipelineLayout(vs.device, &createInfo, nullptr,
                         &vs.pipelineLayouts[get(PipelineLayouts::Primary)]);
}

void ClientApp::cleanupPipelineLayout() {
  vkDestroyPipelineLayout(
      vs.device, vs.pipelineLayouts[get(PipelineLayouts::Primary)], nullptr);
}

void ClientApp::createSpecializationData() {
  vs.specData.lightCount = LightCount;
  vs.specData.imageCount = ImageCount;
  vs.specData.materialCount = get(Materials::COUNT);
  vs.specializationMapEntries[get(SpecConsts::MaterialCount)] =
      vka::specializationMapEntry(0,
                                  offsetof(SpecializationData, materialCount),
                                  sizeof(SpecializationData::MaterialCount));
  vs.specializationMapEntries[get(SpecConsts::ImageCount)] =
      vka::specializationMapEntry(1, offsetof(SpecializationData, imageCount),
                                  sizeof(SpecializationData::ImageCount));
  vs.specializationMapEntries[get(SpecConsts::LightCount)] =
      vka::specializationMapEntry(2, offsetof(SpecializationData, lightCount),
                                  sizeof(SpecializationData::LightCount));
}

void ClientApp::createShader2DModules() {
  auto vertexBytes = fileIO::readFile(std::string(paths.shader2D.vertex));
  auto vertexCreateInfo = vka::shaderModuleCreateInfo();
  vertexCreateInfo.codeSize = vertexBytes.size();
  vertexCreateInfo.pCode =
      reinterpret_cast<const uint32_t*>(vertexBytes.data());
  vkCreateShaderModule(vs.device, &vertexCreateInfo, nullptr,
                       &vs.shaderModules[get(Shaders::Vertex2D)]);

  auto fragmentBytes = fileIO::readFile(std::string(paths.shader2D.fragment));
  auto fragmentCreateInfo = vka::shaderModuleCreateInfo();
  fragmentCreateInfo.codeSize = fragmentBytes.size();
  fragmentCreateInfo.pCode =
      reinterpret_cast<const uint32_t*>(fragmentBytes.data());
  vkCreateShaderModule(vs.device, &fragmentCreateInfo, nullptr,
                       &vs.shaderModules[get(Shaders::Fragment2D)]);
}

void ClientApp::createShader3DModules() {
  auto vertexBytes = fileIO::readFile(std::string(paths.shader3D.vertex));
  auto vertexCreateInfo = vka::shaderModuleCreateInfo();
  vertexCreateInfo.codeSize = vertexBytes.size();
  vertexCreateInfo.pCode =
      reinterpret_cast<const uint32_t*>(vertexBytes.data());
  vkCreateShaderModule(vs.device, &vertexCreateInfo, nullptr,
                       &vs.shaderModules[get(Shaders::Vertex3D)]);

  auto fragmentBytes = fileIO::readFile(std::string(paths.shader3D.fragment));
  auto fragmentCreateInfo = vka::shaderModuleCreateInfo();
  fragmentCreateInfo.codeSize = fragmentBytes.size();
  fragmentCreateInfo.pCode =
      reinterpret_cast<const uint32_t*>(fragmentBytes.data());
  vkCreateShaderModule(vs.device, &fragmentCreateInfo, nullptr,
                       &vs.shaderModules[get(Shaders::Fragment3D)]);
}

void ClientApp::cleanupShaderModules() {
  for (const auto& shader : vs.shaderModules) {
    vkDestroyShaderModule(vs.device, shader, nullptr);
  }
}

void ClientApp::setupPipeline2D() {
  auto& state = vs.pipelineStates[get(Pipelines::P2D)];

  state.inputAssemblyState = vka::pipelineInputAssemblyStateCreateInfo(
      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, false);

  state.dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  state.dynamicState = vka::pipelineDynamicStateCreateInfo(state.dynamicStates);

  state.multisampleState =
      vka::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

  state.viewportState = vka::pipelineViewportStateCreateInfo(1, 1);

  state.inputAssemblyState = vka::pipelineInputAssemblyStateCreateInfo(
      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, false);

  state.specializationInfo = vka::specializationInfo(
      (uint32_t)SpecConsts::COUNT, vs.specializationMapEntries.data(),
      sizeof(SpecializationData), &vs.specData);

  auto vertexShaderStage = vka::pipelineShaderStageCreateInfo();
  vertexShaderStage.module = vs.shaderModules[get(Shaders::Vertex2D)];
  vertexShaderStage.pName = "main";
  vertexShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertexShaderStage.pSpecializationInfo = &state.specializationInfo;
  state.shaderStages.push_back(vertexShaderStage);

  auto fragmentShaderStage = vka::pipelineShaderStageCreateInfo();
  fragmentShaderStage.module = vs.shaderModules[get(Shaders::Fragment2D)];
  fragmentShaderStage.pName = "main";
  fragmentShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragmentShaderStage.pSpecializationInfo = &state.specializationInfo;
  state.shaderStages.push_back(fragmentShaderStage);

  state.colorBlendAttachmentState = vka::pipelineColorBlendAttachmentState(
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
      true);
  state.colorBlendAttachmentState.srcColorBlendFactor =
      VK_BLEND_FACTOR_SRC_ALPHA;
  state.colorBlendAttachmentState.dstColorBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  state.colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  state.colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  state.colorBlendState = vka::pipelineColorBlendStateCreateInfo(
      1, &state.colorBlendAttachmentState);

  state.rasterizationState = vka::pipelineRasterizationStateCreateInfo(
      VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);

  auto& vertexPositionAttribute = state.vertexAttributes.emplace_back();
  vertexPositionAttribute.binding = 0;
  vertexPositionAttribute.location = 0;
  vertexPositionAttribute.offset = 0;
  vertexPositionAttribute.format = VK_FORMAT_R32G32_SFLOAT;

  auto& vertexUVAttribute = state.vertexAttributes.emplace_back();
  vertexUVAttribute.binding = 0;
  vertexUVAttribute.location = 1;
  vertexUVAttribute.offset = 8;
  vertexUVAttribute.format = VK_FORMAT_R32G32_SFLOAT;

  auto vertexBindingDescription =
      vka::vertexInputBindingDescription(0, 16, VK_VERTEX_INPUT_RATE_VERTEX);
  state.vertexBindings.push_back(vertexBindingDescription);

  state.vertexInputState = vka::pipelineVertexInputStateCreateInfo();
  state.vertexInputState.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(state.vertexAttributes.size());
  state.vertexInputState.pVertexAttributeDescriptions =
      state.vertexAttributes.data();
  state.vertexInputState.vertexBindingDescriptionCount =
      static_cast<uint32_t>(state.vertexBindings.size());
  state.vertexInputState.pVertexBindingDescriptions =
      state.vertexBindings.data();

  vs.pipelineCreateInfos[get(Pipelines::P2D)] = vka::pipelineCreateInfo(
      state.shaderStages, vs.pipelineLayouts[get(PipelineLayouts::Primary)],
      vs.renderPass, 1, &state.colorBlendState, nullptr, &state.dynamicState,
      &state.inputAssemblyState, &state.multisampleState,
      &state.rasterizationState, nullptr, &state.vertexInputState,
      &state.viewportState);
}

void ClientApp::setupPipeline3D() {
  auto& state = vs.pipelineStates[get(Pipelines::P3D)];

  state.inputAssemblyState = vka::pipelineInputAssemblyStateCreateInfo(
      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, false);

  state.multisampleState =
      vka::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

  state.viewportState = vka::pipelineViewportStateCreateInfo(1, 1);

  state.dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  state.dynamicState = vka::pipelineDynamicStateCreateInfo(state.dynamicStates);

  state.specializationInfo = vka::specializationInfo(
      (uint32_t)SpecConsts::COUNT, vs.specializationMapEntries.data(),
      sizeof(SpecializationData), &vs.specData);

  auto vertexShaderStage = vka::pipelineShaderStageCreateInfo();
  vertexShaderStage.module = vs.shaderModules[get(Shaders::Vertex3D)];
  vertexShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertexShaderStage.pName = "main";
  vertexShaderStage.pSpecializationInfo = &state.specializationInfo;

  auto fragmentShaderStage = vka::pipelineShaderStageCreateInfo();
  fragmentShaderStage.module = vs.shaderModules[get(Shaders::Fragment3D)];
  fragmentShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragmentShaderStage.pName = "main";
  fragmentShaderStage.pSpecializationInfo = &state.specializationInfo;

  state.shaderStages.push_back(vertexShaderStage);
  state.shaderStages.push_back(fragmentShaderStage);

  state.colorBlendAttachmentState =
      vka::pipelineColorBlendAttachmentState(0, false);
  state.colorBlendState = vka::pipelineColorBlendStateCreateInfo(
      1, &state.colorBlendAttachmentState);

  state.depthStencilState =
      vka::pipelineDepthStencilStateCreateInfo(true, true, VK_COMPARE_OP_LESS);

  state.rasterizationState = vka::pipelineRasterizationStateCreateInfo(
      VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT,
      VK_FRONT_FACE_COUNTER_CLOCKWISE);

  auto positionAttribute =
      vka::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);
  auto normalAttribute =
      vka::vertexInputAttributeDescription(1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0);
  auto positionBinding =
      vka::vertexInputBindingDescription(0, 12, VK_VERTEX_INPUT_RATE_VERTEX);
  auto normalBinding =
      vka::vertexInputBindingDescription(1, 12, VK_VERTEX_INPUT_RATE_VERTEX);

  state.vertexAttributes.push_back(positionAttribute);
  state.vertexAttributes.push_back(normalAttribute);
  state.vertexBindings.push_back(positionBinding);
  state.vertexBindings.push_back(normalBinding);
  state.vertexInputState = vka::pipelineVertexInputStateCreateInfo();

  state.vertexInputState.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(state.vertexAttributes.size());
  state.vertexInputState.pVertexAttributeDescriptions =
      state.vertexAttributes.data();
  state.vertexInputState.vertexBindingDescriptionCount =
      static_cast<uint32_t>(state.vertexBindings.size());
  state.vertexInputState.pVertexBindingDescriptions =
      state.vertexBindings.data();

  vs.pipelineCreateInfos[get(Pipelines::P3D)] = vka::pipelineCreateInfo(
      state.shaderStages, vs.pipelineLayouts[get(PipelineLayouts::Primary)],
      vs.renderPass, 0, &state.colorBlendState, &state.depthStencilState,
      &state.dynamicState, &state.inputAssemblyState, &state.multisampleState,
      &state.rasterizationState, nullptr, &state.vertexInputState,
      &state.viewportState);
}

void ClientApp::createPipelines() {
  vkCreateGraphicsPipelines(vs.device, VK_NULL_HANDLE, get(Pipelines::COUNT),
                            vs.pipelineCreateInfos.data(), nullptr,
                            vs.pipelines.data());
}

void ClientApp::cleanupPipelines() {
  for (const auto& pipeline : vs.pipelines) {
    vkDestroyPipeline(vs.device, pipeline, nullptr);
  }
}

void ClientApp::loadImages() {}

void ClientApp::loadQuads() {}

void ClientApp::loadModels() {
  auto loadFunc = [&](uint32_t index) {
    vs.models[index] = vka::LoadModel(modelPaths[index]);
  };
  loadFunc(get(Models::Cube));
  loadFunc(get(Models::Cylinder));
  loadFunc(get(Models::IcosphereSub2));
  loadFunc(get(Models::Pentagon));
  loadFunc(get(Models::Triangle));
}

void ClientApp::createVertexBuffers() {
  VkDeviceSize vertexBufferSize = vka::Quad::QuadSize * vs.quads.size();
  if (!vertexBufferSize) {
    // TODO: handle error case: no vertices loaded
    return;
  }

  vs.Vertex2D = createStagedBuffer(vertexBufferSize, BufferType::Vertex);

  VkDeviceSize positionBufferSize = 0;
  VkDeviceSize normalBufferSize = 0;
  VkDeviceSize indexBufferSize = 0;
  for (const auto& model : vs.models) {
    const auto& mesh = model.full;

    positionBufferSize += mesh.positions.size() * sizeof(glm::vec3);

    normalBufferSize += mesh.normals.size() * sizeof(glm::vec3);

    indexBufferSize += mesh.indices.size() * sizeof(vka::IndexType);
  }
  if (!positionBufferSize || !normalBufferSize || !indexBufferSize) {
    // TODO: handle error case: no vertices loaded
    return;
  }

  vs.Position3D = createStagedBuffer(positionBufferSize, BufferType::Vertex);

  vs.Normal3D = createStagedBuffer(normalBufferSize, BufferType::Vertex);

  vs.Index3D = createStagedBuffer(indexBufferSize, BufferType::Index);
}

void ClientApp::stageVertexData() {
  auto vert2DCopyFunc = [&](void* mapPtr) {
    size_t vertexOffset = 0;
    size_t vertexBufferOffset = 0;
    for (auto& quad : vs.quads) {
      auto vertexData = gsl::span(quad.vertices);
      memcpy((gsl::byte*)(mapPtr) + vertexBufferOffset, vertexData.data(),
             vertexData.length_bytes());

      quad.firstVertex = vertexOffset;

      vertexOffset += vertexData.size();
      vertexBufferOffset += vertexData.length_bytes();
    }
  };

  auto positionCopyFunc = [&](void* mapPtr) {
    size_t positionOffset = 0;
    size_t positionBufferOffset = 0;
    for (auto& model : vs.models) {
      auto& mesh = model.full;
      auto& positionData = mesh.positions;

      mesh.firstVertex = positionOffset;

      auto positionLengthBytes = positionData.size() * sizeof(glm::vec3);
      std::memcpy((gsl::byte*)mapPtr + positionBufferOffset,
                  positionData.data(), positionLengthBytes);

      positionOffset += positionData.size();
      positionBufferOffset += positionLengthBytes;
    }
  };

  auto normalCopyFunc = [&](void* mapPtr) {
    size_t normalOffset = 0;
    size_t normalBufferOffset = 0;
    for (auto& model : vs.models) {
      auto& mesh = model.full;
      auto& normalData = mesh.normals;

      auto normalLengthBytes = normalData.size() * sizeof(glm::vec3);
      std::memcpy((gsl::byte*)mapPtr + normalBufferOffset, normalData.data(),
                  normalLengthBytes);

      normalOffset += normalData.size();
      normalBufferOffset += normalLengthBytes;
    }
  };

  auto indexCopyFunc = [&](void* mapPtr) {
    size_t indexOffset = 0;
    size_t indexBufferOffset = 0;
    for (auto& model : vs.models) {
      auto& mesh = model.full;
      auto& indexData = mesh.indices;

      mesh.firstIndex = indexOffset;

      auto indexLengthBytes = indexData.size() * sizeof(vka::IndexType);
      std::memcpy((gsl::byte*)mapPtr + indexBufferOffset, indexData.data(),
                  indexLengthBytes);

      indexOffset += indexData.size();
      indexBufferOffset += indexLengthBytes;
    }
  };

  stageDataRecordCopy(vs.utility.buffer, vs.Position3D, positionCopyFunc);

  stageDataRecordCopy(vs.utility.buffer, vs.Normal3D, normalCopyFunc);

  stageDataRecordCopy(vs.utility.buffer, vs.Index3D, indexCopyFunc);

  stageDataRecordCopy(vs.utility.buffer, vs.Vertex2D, vert2DCopyFunc);
}

void ClientApp::cleanupVertexBuffers() {
  vka::DestroyAllocatedBuffer(
      vs.device, vs.Vertex2D.stagingBufferData.value_or(vka::BufferData{}));
  vka::DestroyAllocatedBuffer(vs.device, vs.Vertex2D.bufferData);
  vka::DestroyAllocatedBuffer(
      vs.device, vs.Position3D.stagingBufferData.value_or(vka::BufferData{}));
  vka::DestroyAllocatedBuffer(vs.device, vs.Position3D.bufferData);
  vka::DestroyAllocatedBuffer(
      vs.device, vs.Normal3D.stagingBufferData.value_or(vka::BufferData{}));
  vka::DestroyAllocatedBuffer(vs.device, vs.Normal3D.bufferData);
  vka::DestroyAllocatedBuffer(
      vs.device, vs.Index3D.stagingBufferData.value_or(vka::BufferData{}));
  vka::DestroyAllocatedBuffer(vs.device, vs.Index3D.bufferData);
}

StagedBuffer ClientApp::createStagedBuffer(
    VkDeviceSize bufferSize, BufferType type = BufferType::Uniform) {
  StagedBuffer result;

  VkBufferUsageFlags bufferUsageFlags{};
  switch (type) {
    case BufferType::Uniform:
      bufferUsageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
      break;
    case BufferType::Vertex:
      bufferUsageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
      break;
    case BufferType::Index:
      bufferUsageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
      break;
  }

  VkMemoryPropertyFlags bufferMemProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  VkMemoryPropertyFlags stagingBufferMemProps =
      (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  if (vs.unifiedMemory) {
    bufferMemProps |= stagingBufferMemProps;
  } else {
    bufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }

  result.bufferData =
      createBuffer(bufferSize, bufferUsageFlags, bufferMemProps);

  if (!vs.unifiedMemory) {
    result.stagingBufferData = createBuffer(
        bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingBufferMemProps);
  }

  return result;
}

void ClientApp::destroyStagedBuffer(const StagedBuffer& stagedBuffer) {
  if (stagedBuffer.stagingBufferData) {
    vka::DestroyAllocatedBuffer(vs.device, *stagedBuffer.stagingBufferData);
  }
  vka::DestroyAllocatedBuffer(vs.device, stagedBuffer.bufferData);
}

void ClientApp::stageDataRecordCopy(VkCommandBuffer copyCommandBuffer,
                                    StagedBuffer stagedBuffer,
                                    std::function<void(void*)> copyFunc) {
  vka::BufferData hostBuffer =
      stagedBuffer.stagingBufferData.value_or(stagedBuffer.bufferData);
  vka::MapBuffer(vs.device, hostBuffer);
  copyFunc(hostBuffer.mapPtr);

  if (stagedBuffer.stagingBufferData.has_value()) {
    vka::RecordBufferCopy(copyCommandBuffer, hostBuffer.buffer,
                          stagedBuffer.bufferData.buffer,
                          VkBufferCopy{0, 0, VK_WHOLE_SIZE});
  }
}

void ClientApp::createMaterialUniformBuffer() {
  auto materialBufferSize =
      vs.materialsBufferOffsetAlignment * get(Materials::COUNT);

  vs.materialsUniformBuffer = createStagedBuffer(materialBufferSize);
}

void ClientApp::cleanupMaterialUniformBuffers() {
  destroyStagedBuffer(vs.materialsUniformBuffer);
}

void ClientApp::stageMaterialData(VkCommandBuffer cmd) {
  auto copyFunc = [&](void* mapPtr) {
    VkDeviceSize destinationOffset = 0;
    for (const auto& material : vs.materialUniforms) {
      std::memcpy((gsl::byte*)mapPtr + destinationOffset, &material,
                  sizeof(MaterialUniform));
      destinationOffset += vs.materialsBufferOffsetAlignment;
    }
  };

  stageDataRecordCopy(cmd, vs.materialsUniformBuffer, copyFunc);
}

void ClientApp::stageCameraData(VkCommandBuffer cmd,
                                const CameraUniform* cameraUniformData) {
  auto& stagedBuffer = vs.cameraUniformBuffers[vs.nextImage];

  auto copyFunc = [&](void* mapPtr) {
    std::memcpy(mapPtr, cameraUniformData, sizeof(CameraUniform));
  };

  stageDataRecordCopy(cmd, stagedBuffer, copyFunc);
}

void ClientApp::stageLightData(VkCommandBuffer cmd) {
  auto copyFunc = [&](void* mapPtr) {
    VkDeviceSize lightsBufferOffset = 0U;
    for (auto i = 0U; i < LightCount; ++i) {
      std::memcpy((gsl::byte*)mapPtr + lightsBufferOffset, &vs.lightUniforms[i],
                  sizeof(LightUniform));

      lightsBufferOffset += vs.lightsBuffersOffsetAlignment;
    }
  };

  auto stagedBuffer = vs.lightsUniformBuffers[vs.nextImage];

  stageDataRecordCopy(cmd, stagedBuffer, copyFunc);
}

void ClientApp::createDescriptorPools() {
  auto samplerPoolSize =
      vka::descriptorPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, get(Samplers::COUNT));
  auto imagesPoolSize =
      vka::descriptorPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, ImageCount);
  auto cameraPoolSize =
      vka::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1);
  auto lightsPoolSize =
      vka::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, LightCount);

  std::vector<VkDescriptorPoolSize> staticPoolSizes = {
      samplerPoolSize, imagesPoolSize, cameraPoolSize, lightsPoolSize};
  auto staticPoolCreateInfo = vka::descriptorPoolCreateInfo(staticPoolSizes, 3);
  vkCreateDescriptorPool(vs.device, &staticPoolCreateInfo, nullptr,
                         &vs.staticLayoutPool);

  auto instancePoolSize =
      vka::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1);
  std::vector<VkDescriptorPoolSize> dynamicPoolSizes = {instancePoolSize};
  auto dynamicPoolCreateInfo =
      vka::descriptorPoolCreateInfo(dynamicPoolSizes, 3);
  vkCreateDescriptorPool(vs.device, &dynamicPoolCreateInfo, nullptr,
                         &vs.dynamicLayoutPool);
}

void ClientApp::cleanupDescriptorPools() {
  vkDestroyDescriptorPool(vs.device, vs.staticLayoutPool, nullptr);
  vkDestroyDescriptorPool(vs.device, vs.dynamicLayoutPool, nullptr);
}

void ClientApp::allocateStaticSets() {
  auto allocInfo = vka::descriptorSetAllocateInfo(
      vs.staticLayoutPool, vs.staticSetLayouts.data(), BufferCount);
  vkAllocateDescriptorSets(vs.device, &allocInfo, vs.staticSets.data());
}

void ClientApp::allocateDynamicSets() {
  auto allocInfo = vka::descriptorSetAllocateInfo(
      vs.dynamicLayoutPool, vs.dynamicSetLayouts.data(), BufferCount);
  vkAllocateDescriptorSets(vs.device, &allocInfo, vs.dynamicSets.data());
}

void ClientApp::writeStaticSets() {
  std::array<VkDescriptorImageInfo, ImageCount> imageInfos;
  for (auto i = 0U; i < ImageCount; ++i) {
    imageInfos[i] = vka::descriptorImageInfo(VK_NULL_HANDLE, vs.images[i].view,
                                             vs.images[i].currentLayout);
  }

  std::array<VkDescriptorBufferInfo, get(Materials::COUNT)> materialBufferInfos;
  VkDeviceSize materialBufferOffset = 0;
  for (auto i = 0U; i < get(Materials::COUNT); ++i) {
    materialBufferInfos[i].buffer = vs.materialsUniformBuffer.bufferData.buffer;
    materialBufferInfos[i].offset = materialBufferOffset;
    materialBufferInfos[i].range = sizeof(MaterialUniform);

    materialBufferOffset += vs.materialsBufferOffsetAlignment;
  }

  VkDescriptorBufferInfo cameraBufferInfo{};
  cameraBufferInfo.range = sizeof(CameraUniform);

  std::array<VkDescriptorBufferInfo, LightCount> lightBufferInfos;
  VkDeviceSize lightOffset{};
  for (auto lightIndex = 0U; lightIndex < LightCount; ++lightIndex) {
    lightBufferInfos[lightIndex].offset = lightOffset;
    lightBufferInfos[lightIndex].range = sizeof(LightUniform);

    lightOffset += vs.lightsBuffersOffsetAlignment;
  }

  std::array<VkWriteDescriptorSet, 4> writes;
  constexpr auto imageDescriptors = 0U;
  constexpr auto materialDescriptors = 1U;
  constexpr auto cameraDescriptor = 2U;
  constexpr auto lightDescriptors = 3U;

  writes[imageDescriptors] = vka::writeDescriptorSet(
      vs.staticSets[0], VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, imageInfos.data(),
      imageInfos.size());

  writes[materialDescriptors] = vka::writeDescriptorSet(
      vs.staticSets[0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2,
      materialBufferInfos.data(), materialBufferInfos.size());

  writes[cameraDescriptor] = vka::writeDescriptorSet(
      vs.staticSets[0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3, &cameraBufferInfo,
      1);

  writes[lightDescriptors] = vka::writeDescriptorSet(
      vs.staticSets[0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4,
      lightBufferInfos.data(), lightBufferInfos.size());

  for (auto bufferIndex = 0U; bufferIndex < BufferCount; ++bufferIndex) {
    cameraBufferInfo.buffer =
        vs.cameraUniformBuffers[bufferIndex].bufferData.buffer;
    for (auto lightIndex = 0U; lightIndex < LightCount; ++lightIndex) {
      lightBufferInfos[lightIndex].buffer =
          vs.lightsUniformBuffers[bufferIndex].bufferData.buffer;
    }

    for (auto& write : writes) {
      write.dstSet = vs.staticSets[bufferIndex];
    }

    vkUpdateDescriptorSets(vs.device, writes.size(), writes.data(), 0, nullptr);
  }
}

void ClientApp::writeInstanceDescriptorSet() {
  VkDescriptorBufferInfo bufferInfo{};
  bufferInfo.buffer = vs.instanceUniformBuffers[vs.nextImage].bufferData.buffer;
  bufferInfo.offset = 0U;
  bufferInfo.range = sizeof(InstanceUniform);

  auto write = vka::writeDescriptorSet(
      vs.dynamicSets[vs.nextImage], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
      0, &bufferInfo);

  vkUpdateDescriptorSets(vs.device, 1, &write, 0, nullptr);
}

void ClientApp::createFrameResources() {
  for (auto i = 0U; i < BufferCount; ++i) {
    vs.cameraUniformBuffers[i] = createStagedBuffer(sizeof(CameraUniform));

    vs.lightsUniformBuffers[i] =
        createStagedBuffer(vs.lightsBuffersOffsetAlignment * LightCount);

    constexpr auto defaultInstanceCapacity = 1U;
    vs.instanceUniformBuffers[i] =
        createStagedBuffer(sizeof(InstanceUniform) * defaultInstanceCapacity);
    vs.instanceBufferCapacities[i] = defaultInstanceCapacity;

    auto commandPoolCreateInfo = vka::commandPoolCreateInfo();
    commandPoolCreateInfo.queueFamilyIndex =
        vs.queueFamilyIndices[get(Queues::Graphics)];
    vkCreateCommandPool(vs.device, &commandPoolCreateInfo, nullptr,
                        &vs.renderCommandPools[i]);

    auto fenceCreateInfo = vka::fenceCreateInfo();
    vkCreateFence(vs.device, &fenceCreateInfo, nullptr,
                  &vs.imageReadyFences[i]);
    vs.imageReadyFencePool.pool(vs.imageReadyFences[i]);

    auto semaphoreCreateInfo = vka::semaphoreCreateInfo();
    vkCreateSemaphore(vs.device, &semaphoreCreateInfo, nullptr,
                      &vs.imageRenderSemaphores[i]);
  }
}

void ClientApp::cleanupFrameResources() {
  for (auto i = 0U; i < BufferCount; ++i) {
    vkDestroyFence(vs.device, vs.imageReadyFences[i], nullptr);

    vkDestroySemaphore(vs.device, vs.imageRenderSemaphores[i], nullptr);

    vkDestroyCommandPool(vs.device, vs.renderCommandPools[i], nullptr);

    destroyStagedBuffer(vs.cameraUniformBuffers[i]);
    destroyStagedBuffer(vs.lightsUniformBuffers[i]);
    destroyStagedBuffer(vs.instanceUniformBuffers[i]);
  }
}

void ClientApp::initVulkan() {
  glfwInit();
  createWindow(appName, DefaultWindowSize.width, DefaultWindowSize.height);

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

  createDescriptorPools();

  createPushRanges();

  createPipelineLayout();

  createShader2DModules();

  createShader3DModules();

  createSpecializationData();

  setupPipeline2D();

  setupPipeline3D();

  createPipelines();

  createFrameResources();

  loadImages();

  loadQuads();

  loadModels();

  createVertexBuffers();
}

void ClientApp::initInput() {
  is.inputBindMap[vka::MakeSignature(GLFW_KEY_LEFT, GLFW_PRESS)] =
      Bindings::Left;
  is.inputBindMap[vka::MakeSignature(GLFW_KEY_RIGHT, GLFW_PRESS)] =
      Bindings::Right;
  is.inputBindMap[vka::MakeSignature(GLFW_KEY_UP, GLFW_PRESS)] = Bindings::Up;
  is.inputBindMap[vka::MakeSignature(GLFW_KEY_DOWN, GLFW_PRESS)] =
      Bindings::Down;

  is.inputStateMap[Bindings::Left] =
      vka::MakeAction([]() { std::cout << "Left Pressed.\n"; });
  is.inputStateMap[Bindings::Right] =
      vka::MakeAction([]() { std::cout << "Right Pressed.\n"; });
  is.inputStateMap[Bindings::Up] =
      vka::MakeAction([]() { std::cout << "Up Pressed.\n"; });
  is.inputStateMap[Bindings::Down] =
      vka::MakeAction([]() { std::cout << "Down Pressed.\n"; });
}

static void PushBackInput(GLFWwindow* window, vka::InputMessage&& msg) {
  auto& app = *GetUserPointer(window);
  msg.time = NowMilliseconds();
  app.is.inputBuffer.pushLast(std::move(msg));
}

static ClientApp* GetUserPointer(GLFWwindow* window) {
  return reinterpret_cast<ClientApp*>(glfwGetWindowUserPointer(window));
}

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action,
                        int mods) {
  if (action == GLFW_REPEAT) return;
  vka::InputMessage msg;
  msg.signature.code = key;
  msg.signature.action = action;
  PushBackInput(window, std::move(msg));
}

static void CharacterCallback(GLFWwindow* window, unsigned int codepoint) {}

static void CursorPositionCallback(GLFWwindow* window, double xpos,
                                   double ypos) {
  GetUserPointer(window)->is.cursorX = xpos;
  GetUserPointer(window)->is.cursorY = ypos;
}

static void MouseButtonCallback(GLFWwindow* window, int button, int action,
                                int mods) {
  vka::InputMessage msg;
  msg.signature.code = button;
  msg.signature.action = action;
  PushBackInput(window, std::move(msg));
}

int main() {
  _putenv("DISABLE_VK_LAYER_VALVE_steam_overlay_1=1");
  ClientApp app{};

  app.initVulkan();
  app.initInput();
  app.PrepareECS();
  // load scene
  auto entity0 = app.triangle();
  app.ecs.get<cmp::Material>(entity0).index = get(Materials::Green);
  app.InputThread();
  app.cleanup();
}