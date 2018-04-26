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
        Swapchain(VkSurfaceKHR surface, 
            VkPhysicalDevice physicalDevice, 
            VkDevice device,
            VkRenderPass renderPass,
            uint32_t graphicsQueueFamilyID, 
            uint32_t presentQueueFamilyID)
             :
            surface(surface), 
            physicalDevice(physicalDevice), 
            device(device),
            renderPass(renderPass),
            graphicsQueueFamilyID(graphicsQueueFamilyID),
            presentQueueFamilyID(presentQueueFamilyID)
        {
            CreateSwapchain();
            GetSwapImages();
            CreateSwapImageViews();
            CreateFramebuffers();
        }

        VkSwapchainKHR get()
        {
            return swapchainUnique.get();
        }
        
    private:
        VkSurfaceKHR surface;
        VkPhysicalDevice physicalDevice;
        VkDevice device;
        VkRenderPass renderPass;
        uint32_t graphicsQueueFamilyID;
        uint32_t presentQueueFamilyID;
        VkFormat swapFormat;
        VkExtent2D surfaceExtent;
        VkSwapchainCreateInfoKHR swapchainCreateInfo;
        VkSwapchainKHRUnique swapchainUnique;
        std::vector<VkImage> swapImages;
        std::vector<VkImageViewUnique> swapImageViewsUnique;
        std::vector<VkFramebufferUnique> framebuffersUnique;

        VkExtent2D GetSurfaceExtent()
        {
            VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
            auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                physicalDevice,
                surface,
                &surfaceCapabilities);
            return surfaceCapabilities.currentExtent;
        }

        std::vector<VkSurfaceFormatKHR> GetSurfaceFormats()
        {
            uint32_t formatCount = 0;
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, 
                surface, 
                &formatCount, 
                nullptr);

            std::vector<VkSurfaceFormatKHR> surfaceFormats;
            surfaceFormats.resize(formatCount);

            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, 
                surface, 
                &formatCount, 
                surfaceFormats.data());
            return surfaceFormats;
        }

        VkFormat ChooseSwapFormat(const std::vector<VkSurfaceFormatKHR>& surfaceFormats)
        {
            if (surfaceFormats.size() == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
			    return VK_FORMAT_B8G8R8_UNORM;
            else
                return surfaceFormats[0].format;
        }

        VkColorSpaceKHR ChooseSwapColorSpace(const std::vector<VkSurfaceFormatKHR>& surfaceFormats)
        {
            return surfaceFormats[0].colorSpace;
        }

        std::vector<VkPresentMode> GetPresentModes()
        {
            uint32_t presentModeCount = 0;
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                physicalDevice,
                surface,
                &presentModeCount,
                nullptr);

            std::vector<VkPresentMode> surfacePresentModes;
            surfacePresentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                physicalDevice,
                surface,
                &presentModeCount,
                surfacePresentModes.data());
        }

        VkPresentMode ChoosePresentMode(const std::vector<VkPresentMode>& presentModes)
        {
            auto mailboxMode = std::find(presentModes.begin(), presentModes.end(), VK_PRESENT_MODE_MAILBOX_KHR);
            if (mailboxMode == presentModes.end())
            {
                std::runtime_error("Error: mailbox present mode not supported!");
            }
            return *mailboxMode;
        }

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

            auto surfaceFormats = GetSurfaceFormats();
            auto presentModes = GetPresentModes();
            swapFormat = ChooseSwapFormat(surfaceFormats);
            surfaceExtent = GetSurfaceExtent();
            swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            swapchainCreateInfo.pNext = nullptr;
            swapchainCreateInfo.flags = 0;
            swapchainCreateInfo.surface = surface;
            swapchainCreateInfo.minImageCount = Surface::BufferCount;
            swapchainCreateInfo.imageFormat = swapFormat;
            swapchainCreateInfo.imageColorSpace = ChooseSwapColorSpace(surfaceFormats);
            swapchainCreateInfo.imageExtent = surfaceExtent;
            swapchainCreateInfo.imageArrayLayers = 1;
            swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            swapchainCreateInfo.imageSharingMode = shareMode;
            swapchainCreateInfo.queueFamilyIndexCount = queueFamilyIndices.size();
            swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
            swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
            swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            swapchainCreateInfo.presentMode = ChoosePresentMode(presentModes);
            swapchainCreateInfo.clipped = false;
            swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

            VkSwapchainKHR swapchain = {};
            auto swapchainResult = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain);
            swapchainUnique = VkSwapchainKHRUnique(swapchain, VkSwapchainKHRDeleter(device));
        }

        void GetSwapImages()
        {
            uint32_t swapImageCount = 0;
            vkGetSwapchainImagesKHR(device, swapChain, &swapImageCount, nullptr);
            swapImages.resize(swapImageCount);
            vkGetSwapchainImagesKHR(device, swapChain, &swapImageCount, swapImages.data());
        }

        void CreateSwapImageViews()
        {
            VkImageView view;
            VkImageViewCreateInfo viewCreateInfo = {};
            viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewCreateInfo.pNext = nullptr;
            viewCreateInfo.flags = 0;
            viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewCreateInfo.format = swapFormat;
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
                renderState.framebuffers[i] = VkFramebufferUnique(framebuffer, VkFramebufferDeleter(device));
            }
        }
    };
}