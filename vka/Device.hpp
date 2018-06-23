#pragma once

#include "vulkan/vulkan.h"
#include "UniqueVulkan.hpp"
#include "Surface.hpp"
#include "Swapchain.hpp"
#include "RenderPass.hpp"
#include "Shader.hpp"
#include "DescriptorSet.hpp"
#include "Results.hpp"
#include "fileIO.hpp"
#include "Vertex.hpp"
#include "VertexData.hpp"
#include "GLFW/glfw3.h"
#include "Allocator.hpp"
#include "Pipeline.hpp"
#include "VulkanFunctionLoader.hpp"
#include "gsl.hpp"

#include <tuple>
#include <vector>
#include <functional>
#include <optional>
#include <algorithm>
#include <stdexcept>
#include <string>

namespace vka
{
struct VertexPushConstants
{
    glm::mat4 mvp;
};

struct FragmentPushConstants
{
    glm::uint32 imageOffset;
    glm::vec3 padding;
    glm::vec4 color;
};

struct PushConstants
{
    VertexPushConstants vertexPushConstants;
    FragmentPushConstants fragmentPushConstants;
};

constexpr float graphicsQueuePriority = 0.f;
constexpr float presentQueuePriority = 0.f;

class Device
{
  public:
    static constexpr uint32_t ImageCount = 1;
    static constexpr uint32_t PushConstantSize = sizeof(PushConstants);

    Device(VkInstance instance,
           GLFWwindow *window,
           std::vector<const char *> deviceExtensions,
           std::string vertexShaderPath,
           std::string fragmentShaderPath)
        : instance(instance),
          window(window),
          deviceExtensions(deviceExtensions),
          vertexShaderPath(vertexShaderPath),
          fragmentShaderPath(fragmentShaderPath)
    {
        SelectPhysicalDevice();
        GetQueueFamilyProperties();
        CreateSurface();
        SelectGraphicsQueue();
        SelectPresentQueue();
        CreateDevice();
        CreateAllocator();
        CreateRenderPass();
        CreateSwapchain();
        CreateVertexShader();
        CreateFragmentShader();
        CreateSampler();
        CreateFragmentDescriptorSet();
        SetupVertexData();
        CreatePipelineLayout();
        CreatePipeline();
    }

    VkPhysicalDevice GetPhysicalDevice()
    {
        return physicalDevice;
    }

    VkDevice GetDevice()
    {
        return deviceUnique.get();
    }

    auto &GetAllocator()
    {
        return allocator;
    }

    VkQueue GetGraphicsQueue()
    {
        return graphicsQueue;
    }

    VkQueue GetPresentQueue()
    {
        return presentQueue;
    }

    uint32_t GetGraphicsQueueID()
    {
        return graphicsQueueCreateInfo.queueFamilyIndex;
    }

    uint32_t GetPresentQueueID()
    {
        return presentQueueCreateInfo.queueFamilyIndex;
    }

    VkSwapchainKHR GetSwapchain()
    {
        return swapchainOptional.value().GetSwapchain();
    }

    VkRenderPass GetRenderPass()
    {
        return renderPassOptional->GetRenderPass();
    }

    VkPipelineLayout GetPipelineLayout()
    {
        return pipelineLayoutOptional->GetPipelineLayout();
    }

    VkPipeline GetPipeline()
    {
        return pipelineOptional->GetPipeline();
    }

    VkExtent2D GetSurfaceExtent()
    {
        return surfaceOptional->GetExtent();
    }

    VkFramebuffer GetFramebuffer(size_t i)
    {
        return swapchainOptional->GetFramebuffer(i);
    }

    VkFence CreateFence(bool signaled)
    {
        VkFence fence;
        VkFenceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        createInfo.pNext = nullptr;
        if (signaled)
        {
            createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        }
        vkCreateFence(GetDevice(), &createInfo, nullptr, &fence);
        fences[fence] = VkFenceUnique(fence, VkFenceDeleter(GetDevice()));
        return fence;
    }

    VkSemaphore CreateSemaphore()
    {
        VkSemaphore semaphore;
        VkSemaphoreCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        createInfo.pNext = nullptr;
        vkCreateSemaphore(GetDevice(), &createInfo, nullptr, &semaphore);
        semaphores[semaphore] =
            VkSemaphoreUnique(semaphore, VkSemaphoreDeleter(GetDevice()));
        return semaphore;
    }

    VkCommandPool CreateCommandPool(uint32_t queueFamilyIndex, bool transient, bool poolReset)
    {
        VkCommandPool pool;
        VkCommandPoolCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags |= transient ? VK_COMMAND_POOL_CREATE_TRANSIENT_BIT : 0;
        createInfo.flags |= poolReset ? VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT : 0;
        createInfo.queueFamilyIndex = queueFamilyIndex;
        vkCreateCommandPool(GetDevice(), &createInfo, nullptr, &pool);
        commandPools[pool] = VkCommandPoolUnique(pool, VkCommandPoolDeleter(GetDevice()));
        return pool;
    }

    std::vector<VkCommandBuffer> AllocateCommandBuffers(VkCommandPool pool, uint32_t count)
    {
        std::vector<VkCommandBuffer> commandBuffers;
        commandBuffers.resize(count);
        VkCommandBufferAllocateInfo allocateInfo = {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.pNext = nullptr;
        allocateInfo.commandPool = pool;
        allocateInfo.level = (VkCommandBufferLevel)0;
        allocateInfo.commandBufferCount = count;
        vkAllocateCommandBuffers(GetDevice(), &allocateInfo, commandBuffers.data());
        return commandBuffers;
    }

    void operator()(Results::ErrorSurfaceLost result)
    {
        vkDeviceWaitIdle(GetDevice());
        CreateSurface();
        CreateRenderPass();
        CreateSwapchain();
    }

    void recreateSwapchain()
    {
        vkDeviceWaitIdle(GetDevice());
        CreateSwapchain();
    }

    void operator()(Results::Suboptimal result)
    {
        (*surfaceOptional)(result);
        recreateSwapchain();
    }

    void operator()(Results::ErrorOutOfDate result)
    {
        (*surfaceOptional)(result);
        recreateSwapchain();
    }

  private:
    VkInstance instance;
    GLFWwindow *window;
    std::vector<const char *> deviceExtensions;
    std::string vertexShaderPath;
    std::string fragmentShaderPath;

    VkPhysicalDevice physicalDevice;
    std::vector<VkQueueFamilyProperties> queueFamilyProperties;

    VkDeviceQueueCreateInfo graphicsQueueCreateInfo;
    VkDeviceQueueCreateInfo presentQueueCreateInfo;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    VkDeviceCreateInfo createInfo;
    VkDeviceUnique deviceUnique;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    Allocator allocator;

    std::map<VkCommandPool, VkCommandPoolUnique> commandPools;
    std::map<VkFence, VkFenceUnique> fences;
    std::map<VkSemaphore, VkSemaphoreUnique> semaphores;
    std::optional<Surface> surfaceOptional;
    std::optional<RenderPass> renderPassOptional;
    std::optional<Swapchain> swapchainOptional;
    std::optional<Shader> vertexShaderOptional;
    std::optional<Shader> fragmentShaderOptional;
    VkSampler sampler;
    VkSamplerUnique samplerUnique;

  public:
    std::optional<DescriptorSet> fragmentDescriptorSetOptional;

  private:
    std::vector<VkDescriptorSetLayout> setLayouts;
    std::vector<VkPushConstantRange> pushConstantRanges;
    VertexData vertexData;
    std::optional<PipelineLayout> pipelineLayoutOptional;
    std::optional<Pipeline> pipelineOptional;

    void SelectPhysicalDevice()
    {
        std::vector<VkPhysicalDevice> physicalDevices;
        uint32_t physicalDeviceCount;
        vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
        physicalDevices.resize(physicalDeviceCount);
        auto result = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

        physicalDevice = physicalDevices[0];
    }

    void GetQueueFamilyProperties()
    {
        uint32_t propCount;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
                                                 &propCount,
                                                 nullptr);
        queueFamilyProperties.resize(propCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
                                                 &propCount,
                                                 queueFamilyProperties.data());
    }

    void SelectGraphicsQueue()
    {
        std::optional<uint32_t> graphicsQueueFamilyIndex;
        for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i)
        {
            if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                graphicsQueueFamilyIndex = i;
                break;
            }
        }

        if (graphicsQueueFamilyIndex.has_value() == false)
        {
            std::runtime_error("Device does not support graphics queue!");
        }

        graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        graphicsQueueCreateInfo.pNext = nullptr;
        graphicsQueueCreateInfo.flags = 0;
        graphicsQueueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex.value();
        graphicsQueueCreateInfo.queueCount = 1;
        graphicsQueueCreateInfo.pQueuePriorities = &graphicsQueuePriority;
    }

    void SelectPresentQueue()
    {
        std::optional<uint32_t> presentQueueFamilyIndex;
        for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i)
        {
            VkBool32 presentSupport = false;
            auto surface = surfaceOptional->GetSurface();
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surfaceOptional->GetSurface(), &presentSupport);
            if (presentSupport)
            {
                presentQueueFamilyIndex = i;
                break;
            }
        }

        if (presentQueueFamilyIndex.has_value() == false)
        {
            std::runtime_error("Device does not support present queue!");
        }

        presentQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        presentQueueCreateInfo.pNext = nullptr;
        presentQueueCreateInfo.flags = 0;
        presentQueueCreateInfo.queueFamilyIndex = presentQueueFamilyIndex.value();
        presentQueueCreateInfo.queueCount = 1;
        presentQueueCreateInfo.pQueuePriorities = &presentQueuePriority;
    }

    void CreateDevice()
    {
        queueCreateInfos.push_back(graphicsQueueCreateInfo);
        auto graphicsQueueID = GetGraphicsQueueID();
        auto presentQueueID = GetPresentQueueID();
        auto singleQueue = (graphicsQueueID == presentQueueID);
        if (!singleQueue)
        {
            queueCreateInfos.push_back(presentQueueCreateInfo);
        }

        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.queueCreateInfoCount = gsl::narrow<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;
        createInfo.enabledExtensionCount = gsl::narrow<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        createInfo.pEnabledFeatures = nullptr;
        VkDevice device;
        vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
        deviceUnique = VkDeviceUnique(device, VkDeviceDeleter());
        LoadDeviceLevelEntryPoints(device);

        vkGetDeviceQueue(device, graphicsQueueID, 0, &graphicsQueue);
        if (!singleQueue)
        {
            vkGetDeviceQueue(device, presentQueueID, 0, &presentQueue);
        }
    }

    void CreateAllocator()
    {
        constexpr auto allocSize = 512000U;
        allocator = Allocator(physicalDevice, GetDevice(), allocSize);
    }

    void CreateSurface()
    {
        surfaceOptional.reset();
        surfaceOptional = Surface(instance, physicalDevice, window);
    }

    void CreateRenderPass()
    {
        renderPassOptional.reset();
        renderPassOptional = RenderPass(GetDevice(), surfaceOptional->GetFormat());
    }

    void CreateSwapchain()
    {
        swapchainOptional.reset();
        swapchainOptional = Swapchain(GetDevice(),
                                      surfaceOptional->GetSurface(),
                                      surfaceOptional->GetFormat(),
                                      surfaceOptional->GetColorSpace(),
                                      surfaceOptional->GetExtent(),
                                      surfaceOptional->GetPresentMode(),
                                      renderPassOptional->GetRenderPass(),
                                      GetGraphicsQueueID(),
                                      GetPresentQueueID());
    }

    void CreateVertexShader()
    {
        auto vertexShaderBinary = fileIO::readFile(vertexShaderPath);
        vertexShaderOptional = Shader(GetDevice(),
                                      std::move(vertexShaderBinary),
                                      VK_SHADER_STAGE_VERTEX_BIT,
                                      "main");
    }

    void CreateFragmentShader()
    {
        auto fragmentShaderBinary = fileIO::readFile(fragmentShaderPath);
        fragmentShaderOptional = Shader(GetDevice(),
                                        std::move(fragmentShaderBinary),
                                        VK_SHADER_STAGE_FRAGMENT_BIT,
                                        "main");
    }

    void CreateSampler()
    {
        VkSamplerCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.magFilter = VK_FILTER_LINEAR;
        createInfo.minFilter = VK_FILTER_LINEAR;
        createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        createInfo.mipLodBias = 0.f;
        createInfo.anisotropyEnable = false;
        createInfo.maxAnisotropy = 16.f;
        createInfo.compareEnable = false;
        createInfo.compareOp = VK_COMPARE_OP_NEVER;
        createInfo.minLod = 0.f;
        createInfo.maxLod = 0.f;
        createInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        createInfo.unnormalizedCoordinates = false;

        vkCreateSampler(GetDevice(), &createInfo, nullptr, &sampler);
        samplerUnique = VkSamplerUnique(sampler, VkSamplerDeleter(GetDevice()));
    }

    void CreateFragmentDescriptorSet()
    {
        VkDescriptorSetLayoutBinding binding0 = {};
        binding0.binding = 0;
        binding0.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        binding0.descriptorCount = 1;
        binding0.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        binding0.pImmutableSamplers = &sampler;

        VkDescriptorSetLayoutBinding binding1 = {};
        binding1.binding = 1;
        binding1.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        binding1.descriptorCount = Device::ImageCount;
        binding1.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        binding1.pImmutableSamplers = nullptr;

        VkDescriptorPoolSize size0 = {};
        size0.type = VK_DESCRIPTOR_TYPE_SAMPLER;
        size0.descriptorCount = 1;

        VkDescriptorPoolSize size1 = {};
        size1.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        size1.descriptorCount = Device::ImageCount;

        fragmentDescriptorSetOptional = DescriptorSet(GetDevice(),
                                                      std::vector<VkDescriptorSetLayoutBinding>{binding0, binding1},
                                                      std::vector<VkDescriptorPoolSize>{size0, size1},
                                                      1);
    }

    void SetupVertexData()
    {
        vertexData = VertexData(Vertex::vertexBindingDescriptions(),
                                Vertex::vertexAttributeDescriptions());
    }

    void CreatePipelineLayout()
    {
        setLayouts.push_back(fragmentDescriptorSetOptional->GetSetLayout());

        VkPushConstantRange vertexPushRange = {};
        vertexPushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        vertexPushRange.offset = offsetof(PushConstants, vertexPushConstants);
        vertexPushRange.size = sizeof(VertexPushConstants);
        pushConstantRanges.push_back(vertexPushRange);

        VkPushConstantRange fragmentPushRange = {};
        fragmentPushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragmentPushRange.offset = offsetof(PushConstants, fragmentPushConstants);
        fragmentPushRange.size = sizeof(FragmentPushConstants);
        pushConstantRanges.push_back(fragmentPushRange);

        pipelineLayoutOptional = PipelineLayout(GetDevice(),
                                                setLayouts,
                                                pushConstantRanges);
    }

    void CreatePipeline()
    {
        VkSpecializationMapEntry imageArraySize;
        imageArraySize.constantID = 0;
        imageArraySize.offset = 0;
        imageArraySize.size = sizeof(uint32_t);

        auto vertexSpecializationMapEntries = std::vector<VkSpecializationMapEntry>();
        auto fragmentSpecializationMapEntries = std::vector<VkSpecializationMapEntry>{imageArraySize};
        auto vertexSpecialization = MakeSpecialization(
            gsl::span<VkSpecializationMapEntry>(nullptr, 0),
            nullptr);

        auto fragmentSpecialization = MakeSpecialization(
            gsl::span<VkSpecializationMapEntry>(
                fragmentSpecializationMapEntries.data(),
                fragmentSpecializationMapEntries.size()),
            &Device::ImageCount);

        auto vertexShaderStageInfo = vertexShaderOptional->GetShaderStageInfo(&vertexSpecialization);
        auto fragmentShaderStageInfo = fragmentShaderOptional->GetShaderStageInfo(&fragmentSpecialization);

        pipelineOptional = Pipeline(GetDevice(),
                                    pipelineLayoutOptional->GetPipelineLayout(),
                                    renderPassOptional->GetRenderPass(),
                                    vertexShaderStageInfo,
                                    fragmentShaderStageInfo,
                                    vertexData);
    }
};
} // namespace vka