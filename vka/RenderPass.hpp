#pragma once

#include "UniqueVulkan.hpp"
#include "vulkan/vulkan.h"

#include <array>

namespace vka
{
    class RenderPass
    {
    public:
        RenderPass(
            VkDevice device,
            VkFormat surfaceFormat)
            :
            device(device),
            surfaceFormat(surfaceFormat)
        {
            CreateRenderPass();
        }

        RenderPass(RenderPass&&) = default;
        RenderPass& operator =(RenderPass&&) = default;

        VkRenderPass GetRenderPass()
        {
            return renderPassUnique.get();
        }
    private:
        VkDevice device;
        VkFormat surfaceFormat;
        VkAttachmentDescription colorAttachmentDescription;
        VkAttachmentReference colorAttachmentRef;
        VkSubpassDescription subpassDescription;
        std::array<VkSubpassDependency, 2> dependencies;
        VkRenderPassCreateInfo createInfo;
        VkRenderPassUnique renderPassUnique;
        
        void CreateRenderPass()
        {
            colorAttachmentDescription.flags = 0;
            colorAttachmentDescription.format = surfaceFormat;
            colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            colorAttachmentRef.attachment = 0;
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            subpassDescription.flags = 0;
            subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpassDescription.inputAttachmentCount = 0;
            subpassDescription.pInputAttachments = nullptr;
            subpassDescription.colorAttachmentCount = 1;
            subpassDescription.pColorAttachments = &colorAttachmentRef;
            subpassDescription.pResolveAttachments = 0;
            subpassDescription.pDepthStencilAttachment = nullptr;
            subpassDescription.preserveAttachmentCount = 0;
            subpassDescription.pPreserveAttachments = nullptr;

            dependencies[0].srcSubpass = 0;
            dependencies[0].dstSubpass = 0;
            dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            dependencies[1].srcSubpass = 0;
            dependencies[1].dstSubpass = 0;
            dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;
            createInfo.attachmentCount = 1;
            createInfo.pAttachments = &colorAttachmentDescription;
            createInfo.subpassCount = 1;
            createInfo.pSubpasses = &subpassDescription;
            createInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
            createInfo.pDependencies = dependencies.data();

            VkRenderPass renderPass;
            vkCreateRenderPass(device, &createInfo, nullptr, &renderPass);
            renderPassUnique = VkRenderPassUnique(renderPass, VkRenderPassDeleter(device));
        }
    };
}

// Instance
// SurfaceManager, DeviceManager
// RenderPass, Swapchain, Descriptor Set Layouts/Pools/Sets, Shader Modules  
// Framebuffers
// Pipeline
// Command Buffer