#pragma once

#include "UniqueVulkan.hpp"
#include "vulkan/vulkan.h"

namespace vka
{
    class Swapchain
    {
    public:
        Swapchain(VkSurfaceKHR surface, 
            VkPhysicalDevice physicalDevice, 
            VkDevice device, 
            uint32_t graphicsQueueFamilyID, 
            uint32_t presentQueueFamilyID)
             :
            surface(surface), 
            physicalDevice(physicalDevice), 
            device(device),
            graphicsQueueFamilyID(graphicsQueueFamilyID),
            presentQueueFamilyID(presentQueueFamilyID)
        {
            CreateSwapchain();
        }
    private:
        VkSurfaceKHR surface;
        VkPhysicalDevice physicalDevice;
        VkDevice device;
        uint32_t graphicsQueueFamilyID;
        uint32_t presentQueueFamilyID;
        VkSwapchainCreateInfoKHR swapchainCreateInfo;
        VkSwapchainKHRUnique swapchainUnique;

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
            swapchainCreateInfo.minImageCount = BufferCount;
            swapchainCreateInfo.imageFormat = ChooseSwapFormat();
            swapchainCreateInfo.imageColorSpace = ChooseSwapColorSpace();
            swapchainCreateInfo.imageExtent = GetSurfaceExtent();
            swapchainCreateInfo.imageArrayLayers = 1;
            swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            swapchainCreateInfo.imageSharingMode = shareMode;
            swapchainCreateInfo.queueFamilyIndexCount = queueFamilyIndices.size();
            swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
            swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
            swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            swapchainCreateInfo.presentMode = ChoosePresentMode();
            swapchainCreateInfo.clipped = false;
            swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

        VkSwapchainKHR swapChain = {};
        auto swapchainResult = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapChain);
        renderState.swapchain = VkSwapchainKHRUnique(swapChain, VkSwapchainKHRDeleter(device));
        }
    };
}