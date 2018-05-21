#pragma once

#include "UniqueVulkan.hpp"
#include "vulkan/vulkan.h"
#include "Surface.hpp"

#include <stdexcept>
#include <algorithm>

namespace vka
{
    class Swapchain
    {
    public:
        Swapchain(
            VkDevice device,
            VkSurfaceKHR surface,
            VkFormat surfaceFormat,
            VkColorSpaceKHR surfaceColorSpace,
            VkExtent2D surfaceExtent,
            VkPresentModeKHR presentMode,
            VkRenderPass renderPass,
            uint32_t graphicsQueueFamilyID, 
            uint32_t presentQueueFamilyID)
             :
            device(device),
            surface(surface), 
            surfaceFormat(surfaceFormat),
            surfaceColorSpace(surfaceColorSpace),
            surfaceExtent(surfaceExtent),
            presentMode(presentMode),
            renderPass(renderPass),
            graphicsQueueFamilyID(graphicsQueueFamilyID),
            presentQueueFamilyID(presentQueueFamilyID)
        {
            CreateSwapchain();
            GetSwapImages();
            CreateSwapImageViews();
            CreateFramebuffers();
        }

        VkSwapchainKHR GetSwapchain()
        {
            return swapchainUnique.get();
        }
        
    private:
        VkDevice device;
        VkSurfaceKHR surface;
        VkFormat surfaceFormat;
        VkColorSpaceKHR surfaceColorSpace;
        VkExtent2D surfaceExtent;
        VkPresentModeKHR presentMode;
        VkRenderPass renderPass;
        uint32_t graphicsQueueFamilyID;
        uint32_t presentQueueFamilyID;

        VkSwapchainCreateInfoKHR swapchainCreateInfo;
        VkSwapchainKHRUnique swapchainUnique;
        std::vector<VkImage> swapImages;
        std::vector<VkImageViewUnique> swapImageViewsUnique;
        std::vector<VkFramebufferUnique> framebuffersUnique;

        void CreateSwapchain()
        {
            VkSharingMode shareMode;
            std::vector<uint32_t> queueFamilyIndices;
            if (graphicsQueueFamilyID == presentQueueFamilyID)
            {
                shareMode = VK_SHARING_MODE_EXCLUSIVE;
            }
            else
            {
                shareMode = VK_SHARING_MODE_CONCURRENT;
                queueFamilyIndices.push_back(graphicsQueueFamilyID);
                queueFamilyIndices.push_back(presentQueueFamilyID);
            }

            swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            swapchainCreateInfo.pNext = nullptr;
            swapchainCreateInfo.flags = 0;
            swapchainCreateInfo.surface = surface;
            swapchainCreateInfo.minImageCount = Surface::BufferCount;
            swapchainCreateInfo.imageFormat = surfaceFormat;
            swapchainCreateInfo.imageColorSpace = surfaceColorSpace;
            swapchainCreateInfo.imageExtent = surfaceExtent;
            swapchainCreateInfo.imageArrayLayers = 1;
            swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            swapchainCreateInfo.imageSharingMode = shareMode;
            swapchainCreateInfo.queueFamilyIndexCount = queueFamilyIndices.size();
            swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
            swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
            swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            swapchainCreateInfo.presentMode = presentMode;
            swapchainCreateInfo.clipped = false;
            swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

            VkSwapchainKHR swapchain = {};
            auto swapchainResult = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain);
            swapchainUnique = VkSwapchainKHRUnique(swapchain, VkSwapchainKHRDeleter(device));
        }

        void GetSwapImages()
        {
            uint32_t swapImageCount = 0;
            vkGetSwapchainImagesKHR(device, GetSwapchain(), &swapImageCount, nullptr);
            swapImages.resize(swapImageCount);
            vkGetSwapchainImagesKHR(device, GetSwapchain(), &swapImageCount, swapImages.data());
        }

        void CreateSwapImageViews()
        {
            VkImageView view;
            VkImageViewCreateInfo viewCreateInfo = {};
            viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewCreateInfo.pNext = nullptr;
            viewCreateInfo.flags = 0;
            viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewCreateInfo.format = surfaceFormat;
            viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewCreateInfo.subresourceRange.baseMipLevel = 0;
            viewCreateInfo.subresourceRange.levelCount = 1;
            viewCreateInfo.subresourceRange.baseArrayLayer = 0;
            viewCreateInfo.subresourceRange.layerCount = 1;

            for (auto i = 0U; i < swapImages.size(); i++)
            {
                viewCreateInfo.image = swapImages[i];
                vkCreateImageView(device, &viewCreateInfo, nullptr, &view);
                swapImageViewsUnique[i] = VkImageViewUnique(view, VkImageViewDeleter(device));
            }
        }

        void CreateFramebuffers()
        {
            VkFramebuffer framebuffer;
            VkFramebufferCreateInfo framebufferCreateInfo = {};
            framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferCreateInfo.pNext = nullptr;
            framebufferCreateInfo.flags = 0;
            framebufferCreateInfo.renderPass = renderPass;
            framebufferCreateInfo.attachmentCount = 1;
            framebufferCreateInfo.width = surfaceExtent.width;
            framebufferCreateInfo.height = surfaceExtent.height;
            framebufferCreateInfo.layers = 1;

            for (auto i = 0U; i < swapImageViewsUnique.size(); i++)
            {
                auto view = swapImageViewsUnique[i].get();
                framebufferCreateInfo.pAttachments = &view;
                vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &framebuffer);
                framebuffersUnique[i] = VkFramebufferUnique(framebuffer, VkFramebufferDeleter(device));
            }
        }
    };
}