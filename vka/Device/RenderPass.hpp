#pragma once

#include "UniqueVulkan.hpp"
#include "vulkan/vulkan.h"

namespace vka
{
    class RenderPass
    {
    public:
        RenderPass(VkDevice device, VkFormat surfaceFormat) : device(device), surfaceFormat(surfaceFormat)
        {}
    private:
        VkDevice device;
        VkFormat surfaceFormat;
        VkAttachmentDescription colorAttachmentDescription;
        VkAttachmentReference colorAttachmentRef;
        VkSubpassDescription subpassDescription;
        std::vector<VkSubpassDependency> dependencies;
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

            VkSubpassDescription subpassDescription = {};
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

            dependencies.push_back(dependency0);
            dependencies.push_back(dependency1);

            createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = VkRenderPassCreateFlags(0);
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