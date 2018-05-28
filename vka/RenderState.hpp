#pragma once

#include "UniqueVulkan.hpp"
#include "vulkan/vulkan.h"
#include "VulkanFunctions.hpp"
#include "Allocator.hpp"
#include "InitState.hpp"
#include "Buffer.hpp"
#include "Camera.hpp"
#include "glm/glm.hpp"
#include "Sprite.hpp"
#include "Vertex.hpp"
#include "CopyState.hpp"
#include "DeviceState.hpp"
#include "SurfaceState.hpp"
#include "Image2D.hpp"
#include "fileIO.hpp"
#include "entt.hpp"
#include <array>
#include <cstring>
#include <vector>
#include <string>
#include <map>
#include <algorithm>

namespace vka
{
    struct Supports
    {
        VkCommandPool renderCommandPool;
        VkCommandBuffer renderCommandBuffer;
        VkFence renderBufferExecuted;
        VkFence imageReadyFence;
        VkSemaphore imageRenderCompleteSemaphore;
    };

    struct RenderState
    {
        UniqueAllocatedBuffer vertexBuffer;
        VkRenderPassUnique renderPass;
		VkSamplerUnique sampler;
        VkClearValue clearValue;
        VkSwapchainKHRUnique swapchain;
		std::vector<VkImage> swapImages;
        std::array<Supports, BufferCount> supports;
		std::array<VkImageViewUnique, BufferCount> swapViews;
		std::array<VkFramebufferUnique, BufferCount> framebuffers;
        uint32_t nextImage;
        std::array<VkFenceUnique, BufferCount> imageFences;
        std::vector<VkFence> fencePool;
        Camera2D camera;
        std::map<uint64_t, UniqueImage2D> images;
        std::map<uint64_t, Sprite> sprites;
        std::vector<Quad> quads;
    };

    struct VertexPushConstants
	{
		glm::uint32 imageOffset;
        glm::vec4 color;
        glm::mat4 mvp;
	};

    struct ShaderState
    {
        VkShaderModuleUnique vertexShader;
		VkShaderModuleUnique fragmentShader;
		VkPushConstantRange pushConstantRange;
		VkDescriptorSetLayoutUnique fragmentDescriptorSetLayout;
		VkDescriptorPoolUnique fragmentLayoutDescriptorPool;
		VkDescriptorSet fragmentDescriptorSet;
    };

    static void SetClearColor(RenderState& renderState, float r, float g, float b, float a)
    {
        VkClearColorValue clearColor;
        clearColor.float32[0] = r;
        clearColor.float32[1] = g;
        clearColor.float32[2] = b;
        clearColor.float32[3] = a;
        renderState.clearValue.color = clearColor;
    }

    static void CreateShaderModules(ShaderState& shaderState, const DeviceState& deviceState, const InitState& copyState)
	{
        auto device = deviceState.device.get();
		// read from file
		auto vertexShaderBinary = fileIO::readFile(copyState.vertexShaderPath);
		// create shader module
        VkShaderModuleCreateInfo vertexShaderCreateInfo = {};
        vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vertexShaderCreateInfo.pNext = nullptr;
        vertexShaderCreateInfo.flags = (VkShaderModuleCreateFlags)0;
        vertexShaderCreateInfo.codeSize = vertexShaderBinary.size();
        vertexShaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vertexShaderBinary.data());

        VkShaderModule vertexModule;
        vkCreateShaderModule(device, &vertexShaderCreateInfo, nullptr, &vertexModule);

        VkShaderModuleDeleter vertexModuleDeleter = {};
        vertexModuleDeleter.device = device;
		shaderState.vertexShader = VkShaderModuleUnique(vertexModule, vertexModuleDeleter);

		// read from file
		auto fragmentShaderBinary = fileIO::readFile(copyState.fragmentShaderPath);
		// create shader module
        VkShaderModuleCreateInfo fragmentShaderCreateInfo = {};
        fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        fragmentShaderCreateInfo.pNext = nullptr;
        fragmentShaderCreateInfo.flags = (VkShaderModuleCreateFlags)0;
        fragmentShaderCreateInfo.codeSize = fragmentShaderBinary.size();
        fragmentShaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(fragmentShaderBinary.data());

        VkShaderModule fragmentModule;
        vkCreateShaderModule(device, &fragmentShaderCreateInfo, nullptr, &fragmentModule);

        VkShaderModuleDeleter fragmentModuleDeleter = {};
        fragmentModuleDeleter.device = device;
        shaderState.fragmentShader = VkShaderModuleUnique(fragmentModule, fragmentModuleDeleter);
	}

	static void CreateFragmentDescriptorPool(ShaderState& shaderState, 
		const DeviceState& deviceState, 
		const RenderState& renderState)
	{
		auto textureCount = renderState.images.size();

        VkDescriptorPoolSize samplerPoolSize = {};
        samplerPoolSize.type = VK_DESCRIPTOR_TYPE_SAMPLER;
        samplerPoolSize.descriptorCount = 1;

        VkDescriptorPoolSize imagesPoolSize = {};
        imagesPoolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        imagesPoolSize.descriptorCount = textureCount;

		auto poolSizes = std::vector<VkDescriptorPoolSize>
		{
			samplerPoolSize,
            imagesPoolSize
		};

        VkDescriptorPoolCreateInfo poolCreateInfo = {};
        poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolCreateInfo.pNext = nullptr;
        poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolCreateInfo.maxSets = 1;
        poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolCreateInfo.pPoolSizes = poolSizes.data();

        VkDescriptorPool pool;
        vkCreateDescriptorPool(deviceState.device.get(), &poolCreateInfo, nullptr, &pool);

        VkDescriptorPoolDeleter poolDeleter;
        poolDeleter.device = deviceState.device.get();
        shaderState.fragmentLayoutDescriptorPool = VkDescriptorPoolUnique(pool, poolDeleter);
	}

	static void CreateFragmentSetLayout(ShaderState& shaderState, 
		const DeviceState& deviceState, 
		const RenderState& renderState)
	{
		auto textureCount = renderState.images.size();

        VkDescriptorSetLayoutBinding samplerBinding;
        VkSampler sampler = renderState.sampler.get();
        samplerBinding.binding = 0;
        samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        samplerBinding.descriptorCount = 1;
        samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        samplerBinding.pImmutableSamplers = &sampler;

        VkDescriptorSetLayoutBinding imagesBinding;
        imagesBinding.binding = 1;
        imagesBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        imagesBinding.descriptorCount = textureCount;
        imagesBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        imagesBinding.pImmutableSamplers = nullptr;

		auto bindings = std::vector<VkDescriptorSetLayoutBinding>
		{
			samplerBinding,
            imagesBinding
		};

        VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutCreateInfo.pNext = nullptr;
        layoutCreateInfo.flags = VkDescriptorSetLayoutCreateFlags(0);
        layoutCreateInfo.bindingCount = bindings.size();
        layoutCreateInfo.pBindings = bindings.data();

        VkDescriptorSetLayout layout;
        vkCreateDescriptorSetLayout(
            deviceState.device.get(),
            &layoutCreateInfo,
            nullptr,
            &layout);

        VkDescriptorSetLayoutDeleter layoutDeleter = {};
        layoutDeleter.device = deviceState.device.get();
		shaderState.fragmentDescriptorSetLayout = VkDescriptorSetLayoutUnique(layout, layoutDeleter);
	}

	static void SetupPushConstants(ShaderState& shaderState)
	{
		shaderState.pushConstantRange = {};
        shaderState.pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        shaderState.pushConstantRange.offset = 0;
        shaderState.pushConstantRange.size = sizeof(VertexPushConstants);
	}

	static void CreateAndUpdateFragmentDescriptorSet(ShaderState& shaderState, 
		const DeviceState& deviceState, 
		const RenderState& renderState)
	{
		auto textureCount = renderState.images.size();
        auto device = deviceState.device.get();
		// create fragment descriptor set

        VkDescriptorSetAllocateInfo setAllocInfo = {};
        VkDescriptorSetLayout layout = shaderState.fragmentDescriptorSetLayout.get();
        setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        setAllocInfo.pNext = nullptr;
        setAllocInfo.descriptorPool = shaderState.fragmentLayoutDescriptorPool.get();
        setAllocInfo.descriptorSetCount = 1;
        setAllocInfo.pSetLayouts = &layout;

        auto setAllocResult = vkAllocateDescriptorSets(device, &setAllocInfo, &shaderState.fragmentDescriptorSet);

        auto imageInfos = std::vector<VkDescriptorImageInfo>();
		imageInfos.reserve(textureCount);
		for (auto& imagePair : renderState.images)
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
        samplerDescriptorWrite.dstSet = shaderState.fragmentDescriptorSet;
        samplerDescriptorWrite.dstBinding = 1;
        samplerDescriptorWrite.dstArrayElement = 0;
        samplerDescriptorWrite.descriptorCount = textureCount;
        samplerDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        samplerDescriptorWrite.pImageInfo = imageInfos.data();
        samplerDescriptorWrite.pBufferInfo = nullptr;
        samplerDescriptorWrite.pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(device, 1, &samplerDescriptorWrite, 0, nullptr);
	}

    

    static void CreateRenderPass(RenderState& renderState, const DeviceState& deviceState, const SurfaceState& surfaceState)
    {
        auto device = deviceState.device.get();
        // create render pass
        VkAttachmentDescription colorAttachmentDescription = {};
        colorAttachmentDescription.flags = VkAttachmentDescriptionFlags(0);
        colorAttachmentDescription.format = surfaceState.surfaceFormat;
        colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpassDescription = {};
        subpassDescription.flags = VkSubpassDescriptionFlags(0);
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.inputAttachmentCount = 0;
        subpassDescription.pInputAttachments = nullptr;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments = &colorAttachmentRef;
        subpassDescription.pResolveAttachments = 0;
        subpassDescription.pDepthStencilAttachment = nullptr;
        subpassDescription.preserveAttachmentCount = 0;
        subpassDescription.pPreserveAttachments = nullptr;

        VkSubpassDependency dependency0 = {};
        dependency0.srcSubpass = 0;
        dependency0.dstSubpass = 0;
        dependency0.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependency0.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency0.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependency0.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency0.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkSubpassDependency dependency1 = {};
        dependency1.srcSubpass = 0;
        dependency1.dstSubpass = 0;
        dependency1.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency1.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependency1.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency1.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependency1.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        std::vector<VkSubpassDependency> dependencies =
        {
            dependency0,
            dependency1
        };

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.pNext = nullptr;
        renderPassInfo.flags = VkRenderPassCreateFlags(0);
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachmentDescription;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpassDescription;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        VkRenderPass renderPass;
        VkRenderPassDeleter renderPassDeleter = {};
        renderPassDeleter.device = device;
        auto renderPassResult = vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass);
        renderState.renderPass = VkRenderPassUnique(renderPass, renderPassDeleter);
    }

    static void CreateSampler(RenderState& renderState, const DeviceState& deviceState)
    {
        VkSamplerCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = VkSamplerCreateFlags(0);
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

        VkSampler sampler;
        vkCreateSampler(deviceState.device.get(), &createInfo, nullptr, &sampler);
        VkSamplerDeleter deleter;
        deleter.device = deviceState.device.get();
        renderState.sampler = VkSamplerUnique(sampler, deleter);
    }

    static void CheckForPresentationSupport(const DeviceState& deviceState, const SurfaceState& surfaceState)
    {
        VkBool32 presentSupport;
        vkGetPhysicalDeviceSurfaceSupportKHR(deviceState.physicalDevice, 
            deviceState.graphicsQueueFamilyID, 
            surfaceState.surface.get(), 
            &presentSupport);
        if (!presentSupport)
        {
            std::runtime_error("Presentation to this surface isn't supported.");
            exit(1);
        }
    }

    static void DestroySwapchain(RenderState& renderState, const DeviceState& deviceState)
    {
        vkDeviceWaitIdle(deviceState.device.get());
        renderState.swapchain.reset();
    }

    static VkExtent2D ChooseSwapExtent(const SurfaceState& surfaceState)
    {
        if (surfaceState.surfaceExtent.width == -1 || surfaceState.surfaceExtent.height == -1)
        {
            return surfaceState.defaultExtent;
        }
        else
        {
            auto& min = surfaceState.surfaceCapabilities.minImageExtent;
            auto& max = surfaceState.surfaceCapabilities.maxImageExtent;
            auto& currentWidth = surfaceState.surfaceExtent.width;
            auto& currentHeight = surfaceState.surfaceExtent.height;

            VkExtent2D result;
            result.width = std::clamp(currentWidth, min.width, max.width);
            result.height = std::clamp(currentHeight, min.height, max.height);
            return result;
        }
    }

    static void CreateSwapchain(RenderState& renderState, 
        const DeviceState& deviceState, 
        const SurfaceState& surfaceState)
    {
        auto device = deviceState.device.get();

        VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
        swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo.pNext = nullptr;
        swapchainCreateInfo.flags = VkSwapchainCreateFlagsKHR(0);
        swapchainCreateInfo.surface = surfaceState.surface.get();
        swapchainCreateInfo.minImageCount = BufferCount;
        swapchainCreateInfo.imageFormat = surfaceState.surfaceFormat;
        swapchainCreateInfo.imageColorSpace = surfaceState.surfaceColorSpace;
        swapchainCreateInfo.imageExtent = ChooseSwapExtent(surfaceState);
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 1;
        swapchainCreateInfo.pQueueFamilyIndices = &deviceState.graphicsQueueFamilyID;
        swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.presentMode = surfaceState.presentMode;
        swapchainCreateInfo.clipped = false;
        swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

        VkSwapchainKHR swapChain = {};
        auto swapchainResult = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapChain);
        VkSwapchainKHRDeleter swapchainDeleter;
        swapchainDeleter.device = device;
        renderState.swapchain = VkSwapchainKHRUnique(swapChain, swapchainDeleter);
        
        uint32_t swapImageCount = 0;
        vkGetSwapchainImagesKHR(device, swapChain, &swapImageCount, nullptr);
        renderState.swapImages.resize(swapImageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &swapImageCount, renderState.swapImages.data());
        
        VkImageView view;
        VkImageViewCreateInfo viewCreateInfo = {};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.pNext = nullptr;
        viewCreateInfo.flags = 0;
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCreateInfo.format = surfaceState.surfaceFormat;
        viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewCreateInfo.subresourceRange.baseMipLevel = 0;
        viewCreateInfo.subresourceRange.levelCount = 1;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;
        viewCreateInfo.subresourceRange.layerCount = 1;

        for (auto i = 0U; i < BufferCount; i++)
        {
            viewCreateInfo.image = renderState.swapImages[i];
            vkCreateImageView(device, &viewCreateInfo, nullptr, &view);
            VkImageViewDeleter viewDeleter;
            viewDeleter.device = device;
            renderState.swapViews[i] = VkImageViewUnique(view, viewDeleter);

            VkFramebuffer framebuffer;
            VkFramebufferCreateInfo framebufferCreateInfo = {};
            framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferCreateInfo.pNext = nullptr;
            framebufferCreateInfo.flags = 0;
            framebufferCreateInfo.renderPass = renderState.renderPass.get();
            framebufferCreateInfo.attachmentCount = 1;
            framebufferCreateInfo.pAttachments = &view;
            framebufferCreateInfo.width = surfaceState.surfaceExtent.width;
            framebufferCreateInfo.height = surfaceState.surfaceExtent.height;
            framebufferCreateInfo.layers = 1;
            vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &framebuffer);
            VkFramebufferDeleter framebufferDeleter = {};
            framebufferDeleter.device = device;
            renderState.framebuffers[i] = VkFramebufferUnique(framebuffer, framebufferDeleter);
        }
    }

    static void InitializeSupportStructs(RenderState& renderState, const DeviceState& deviceState, const ShaderState& shaderState)
    {
        auto device = deviceState.device.get();
        for (auto& support : renderState.supports)
        {
            VkCommandPool commandPool;
            VkCommandPoolCreateInfo poolCreateInfo = {};
            poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolCreateInfo.pNext = nullptr;
            poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            poolCreateInfo.queueFamilyIndex = deviceState.graphicsQueueFamilyID;
            vkCreateCommandPool(device, &poolCreateInfo, nullptr, &commandPool);
            VkCommandPoolDeleter commandPoolDeleter = {};
            commandPoolDeleter.device = device;
            support.renderCommandPool = VkCommandPoolUnique(commandPool, commandPoolDeleter);

            VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
            commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            commandBufferAllocateInfo.pNext = nullptr;
            commandBufferAllocateInfo.commandPool = commandPool;
            commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            commandBufferAllocateInfo.commandBufferCount = 1;
            vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &support.renderCommandBuffer);

            VkFence fence = {};
            VkFenceCreateInfo fenceCreateInfo = {};
            fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            vkCreateFence(device, &fenceCreateInfo, nullptr, &fence);
            VkFenceDeleter fenceDeleter = {};
            fenceDeleter.device = device;
            support.renderBufferExecuted = VkFenceUnique(fence, fenceDeleter);

            VkSemaphore semaphore;
            VkSemaphoreCreateInfo semaphoreCreateInfo = {};
            semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphore);
            VkSemaphoreDeleter semaphoreDeleter = {};
            semaphoreDeleter.device = device;
            support.imageRenderCompleteSemaphore = VkSemaphoreUnique(semaphore, semaphoreDeleter);
        }
    }

    static void SetupImageFencePool(RenderState& renderState, const DeviceState& deviceState)
    {
        auto device = deviceState.device.get();

        VkFenceDeleter deleter;
        deleter.device = device;
        VkFenceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;

        for (auto& fenceUnique : renderState.imageFences)
        {
            VkFence fence = {};
            auto fenceResult = vkCreateFence(device, &createInfo, nullptr, &fence);
            renderState.fencePool.push_back(fence);
            fenceUnique = VkFenceUnique(fence, deleter);
        }
    }

    
}