#pragma once

#include "vulkan/vulkan.h"
#include "UniqueVulkan.hpp"
#include "Surface.hpp"
#include "Swapchain.hpp"
#include "RenderPass.hpp"
#include "Shader.hpp"
#include "DescriptorSet.hpp"
#include "vka/Results.hpp"
#include "fileIO.hpp"
#include "Vertex.hpp"

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
		glm::uint32 imageOffset;
        glm::vec4 color;
        glm::mat4 mvp;
	};

    class Device
    {
    public:
        static constexpr uint32_t ImageCount = 1;
        static constexpr uint32_t PushConstantSize = sizeof(VertexPushConstants);

        Device(VkInstance instance,
            GLFWWindow* window,
            std::vector<const char*> deviceExtensions,
            std::string vertexShaderPath,
            std::string fragmentShaderPath)
            : 
            instance(instance),
            window(window),
            deviceExtensions(deviceExtensions),
            vertexShaderPath(vertexShaderPath),
            fragmentShaderPath(fragmentShaderPath)
        {
            SelectPhysicalDevice();
            GetQueueFamilyProperties();
            SelectGraphicsQueue();
            SelectPresentQueue();
            CreateDevice();
            CreateSurface();
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

        VkDevice GetDevice()
        {
            return deviceUnique.get();
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

        VkFence CreateFence(bool Signaled)
        {
            VkFence fence;
            VkFenceCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            createInfo.pNext = nullptr;
            if (Signaled)
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

        void operator()(Results::ErrorSurfaceLost result)
        {
            vkDeviceWaitIdle(GetDevice());
            CreateSurface();
            CreateRenderPass();
            CreateSwapchain();
        }

        void operator()(Results::Suboptimal result)
        {
            vkDeviceWaitIdle(GetDevice());
            CreateSwapchain();
        }

        void operator()(Results::ErrorOutOfDate result)
        {
            vkDeviceWaitIdle(GetDevice());
            CreateSwapchain();
        }
        
    private:
        VkInstance instance;
        GLFWWindow* window;
        std::vector<const char*> deviceExtensions;
        std::string vertexShaderPath;
        std::string fragmentShaderPath;

        VkPhysicalDevice physicalDevice;
        std::vector<VkQueueFamilyProperties> queueFamilyProperties;
        const float graphicsQueuePriority = 0.f;
        VkDeviceQueueCreateInfo graphicsQueueCreateInfo;
        const float presentQueuePriority = 0.f;
        VkDeviceQueueCreateInfo presentQueueCreateInfo;
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        VkDeviceCreateInfo createInfo;
        VkDeviceUnique deviceUnique;
        VkQueue graphicsQueue;
        VkQueue presentQueue;

        std::map<VkFence, VkFenceUnique> fences;
        std::map<VkSemaphore, VkSemaphoreUnique> semaphores;
        std::optional<Surface> surface;
        std::optional<RenderPass> renderPass;
        std::optional<Swapchain> swapchain;
        std::optional<Shader> vertexShader;
        std::optional<Shader> fragmentShader;
        VkSampler sampler;
        VkSamplerUnique samplerUnique;
        std::optional<DescriptorSet> fragmentDescriptorSet;
        std::vector<VkDescriptorSetLayout> setLayouts;
        std::vector<VkPushConstantRange> pushConstantRanges;
        VertexData vertexData;
        std::optional<PipelineLayout> pipelineLayout;
        std::optional<Pipeline> pipeline;
        
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
                bool presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
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
            if (graphicsQueueCreateInfo.queueFamilyIndex != presentQueueCreateInfo.queueFamilyIndex)
            {
                queueCreateInfos.push_back(presentQueueCreateInfo);
            }

            createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;
            createInfo.m_GraphicsQueueCreateInfoCount = queueCreateInfos.size();
            createInfo.pQueueCreateInfos = queueCreateInfos.data();
            createInfo.enabledLayerCount = 0;
            createInfo.ppEnabledLayerNames = nullptr;
            createInfo.enabledExtensionCount = deviceExtensions.size();
            createInfo.ppEnabledExtensionNames = deviceExtensions.data();
            createInfo.pEnabledFeatures = nullptr;
            VkDevice device;
            vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
            deviceUnique = VkDeviceUnique(device, VkDeviceDeleter());

            vkGetDeviceQueue(GetDevice(), GetGraphicsQueueID(), 0, &graphicsQueue);
            vkGetDeviceQueue(GetDevice(), GetPresentQueueID(), 0, &presentQueue);
        }

        void CreateSurface()
        {
            surface = Surface(instance, physicalDevice, window);
        }

        void CreateRenderPass()
        {
            renderPass = RenderPass(GetDevice(), surface.format());
        }

        void CreateSwapchain()
        {
            swapchain = Swapchain(GetDevice(), 
                surface->GetSurface(), 
                surface->GetFormat(), 
                surface->GetColorSpace(), 
                surface->GetExtent(), 
                surface->GetPresentMode(), 
                renderPass->GetRenderPass(),
                GetGraphicsQueueID(),
                GetPresentQueueID());
        }

        void CreateVertexShader()
        {
            auto vertexShaderBinary = fileIO::readFile(vertexShaderPath);
            vertexShader = Shader(GetDevice(), 
                vertexShaderBinary, 
                VK_SHADER_STAGE_VERTEX_BIT);
        }

        void CreateFragmentShader()
        {
            auto fragmentShaderBinary = fileIO::readFile(fragmentShaderPath);
            fragmentShader = Shader(GetDevice(), 
                fragmentShaderBinary, 
                VK_SHADER_STAGE_FRAGMENT_BIT);
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
            size0.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            size0.descriptorCount = 1;

            VkDescriptorPoolSize size1 = {};
            size1.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            size1.descriptorCount = Device::ImageCount;

            fragmentDescriptorSet = DescriptorSet(GetDevice(), 
                std::vector<VkDescriptorSetLayoutBinding>{binding0, binding1},
                std::vector<VkDescriptorPoolSize>{size0, size1},
                1);
        }

        void SetupVertexData()
        {
            vertexData = VertexData(Vertex::vertexBindingDescriptions(),
                vertexAttributeDescriptions());
        }

        void CreatePipelineLayout()
        {
            setLayouts.push_back(fragmentDescriptorSet.GetSetLayout());

            VkPushConstantRange range = {};
            range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            range.offset = 0;
            range.size = Device::PushConstantSize;
            pushConstantRanges.push_back(range);

            pipelineLayout = PipelineLayout(GetDevice(),
                setLayouts,
                pushConstantRanges);
        }

        void CreatePipeline()
        {
            VkSpecializationMapEntry imageArraySize;
            imageArraySize.constantID = 0;
            imageArraySize.offset = 0;
            imageArraySize.size = sizeof(uint32_t);

            pipeline = Pipeline(GetDevice(),
                pipelineLayout->GetPipelineLayout(),
                renderPass->GetRenderPass(),
                vertexShader->GetShaderData(),
                fragmentShader->GetShaderData(
                    std::vector<VkSpecializationMapEntry>{imageArraySize}, 
                    Device::ImageCount),
                vertexData);
        }
    };
}