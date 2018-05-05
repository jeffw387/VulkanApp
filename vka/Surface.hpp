#pragma once

#include "UniqueVulkan.hpp"
#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"

#include <stdexcept>
#include <vector>

namespace vka
{
    class Surface
    {
    public:
        static constexpr size_t BufferCount = 3U;

        Surface(VkInstance instance, VkPhysicalDevice physicalDevice, GLFWwindow* window) : 
            instance(instance), physicalDevice(physicalDevice), window(window)
        {
            GetPlatformHandle();
            CreateSurface();
            GetCapabilities();
            BufferSupportCheck();
            ChooseFormat();
            ChoosePresentMode();
        }

        VkSurfaceKHR GetSurface()
        {
            return surfaceUnique.get();
        }

        VkFormat GetFormat()
        {
            return surfaceFormat;
        }

        VkColorSpaceKHR GetColorSpace()
        {
            return surfaceColorSpace;
        }

        VkExtent2D GetExtent()
        {
            return surfaceCapabilities.currentExtent;
        }

        VkPresentModeKHR GetPresentMode()
        {
            return presentMode;
        }

        void UpdateCapabilities()
        {
            GetCapabilities();
        }

    private:
#ifdef VK_USE_PLATFORM_WIN32_KHR
        HWND hwnd;
        HINSTANCE hinstance;
        VkWin32SurfaceCreateInfoKHR surfaceCreateInfoWin32;
#endif
        VkInstance instance;
        VkPhysicalDevice physicalDevice;
        VkSurfaceKHRUnique surfaceUnique;
        GLFWwindow* window;
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        VkFormat surfaceFormat;
        VkColorSpaceKHR surfaceColorSpace;
        VkPresentModeKHR presentMode;

        void GetPlatformHandle()
        {
#ifdef VK_USE_PLATFORM_WIN32_KHR
            hwnd = glfwGetWin32Window(window);
            hinstance = GetModuleHandle(NULL);
#endif
        }

        void CreateSurface()
        {
#ifdef VK_USE_PLATFORM_WIN32_KHR
            surfaceCreateInfoWin32.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            surfaceCreateInfoWin32.pNext = nullptr;
            surfaceCreateInfoWin32.flags = 0;
            surfaceCreateInfoWin32.hinstance = hinstance;
            surfaceCreateInfoWin32.hwnd = win32Window;

            VkSurfaceKHR surface;
            vkCreateWin32SurfaceKHR(
                instance,
                &surfaceCreateInfoWin32,
                nullptr,
                &surface);
            surfaceUnique = VkSurfaceKHRUnique(surface, VkSurfaceKHRDeleter(instance));
#endif
            VkSurfaceCapabilitiesKHR capabilities = {};
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);
            
        }

        void GetCapabilities()
        {
            auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                physicalDevice,
                get(),
                &surfaceCapabilities);
        }

        void BufferSupportCheck();
        {
            if (surfaceCapabilities.minImageCount < Surface::BufferCount)
            {
                std::runtime_error("Error: surface does not support enough swap images!");
            }
        }

        std::vector<VkSurfaceFormatKHR> GetSurfaceFormats()
        {
            
            return surfaceFormats;
        }

        void ChooseFormat()
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

            if (surfaceFormats.size() == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
			    surfaceFormat = VK_FORMAT_B8G8R8_UNORM;
            else
                surfaceFormat = surfaceFormats[0].format;
            surfaceColorSpace = surfaceFormats[0].colorSpace;
        }

        void ChoosePresentMode()
        {
            uint32_t presentModeCount = 0;
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                physicalDevice,
                surface,
                &presentModeCount,
                nullptr);

            std::vector<VkPresentModeKHR> presentModes;
            presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                physicalDevice,
                surface,
                &presentModeCount,
                presentModes.data());

            auto mailboxMode = std::find(presentModes.begin(), presentModes.end(), VK_PRESENT_MODE_MAILBOX_KHR);
            if (mailboxMode == presentModes.end())
            {
                std::runtime_error("Error: mailbox present mode not supported!");
            }
            presentMode = *mailboxMode;
        }
    };
}