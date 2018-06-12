#pragma once

#include "UniqueVulkan.hpp"
#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"
#include "Results.hpp"

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
            surfaceCapabilities = GetCapabilities();
            BufferSupportCheck();
            ChooseFormat();
            ChoosePresentMode();
        }

        Surface(Surface&&) = default;
        Surface& operator =(Surface&&) = default;

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

        VkPresentModeKHR GetPresentMode()
        {
            return presentMode;
        }

        VkExtent2D GetExtent()
        {
            return surfaceCapabilities.currentExtent;
        }

        void operator()(Results::ErrorOutOfDate result)
        {
            surfaceCapabilities = GetCapabilities();
        }

        void operator()(Results::Suboptimal result)
        {
            surfaceCapabilities = GetCapabilities();
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

        VkSurfaceCapabilitiesKHR GetCapabilities()
        {
            VkSurfaceCapabilitiesKHR capabilities;
            auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                physicalDevice,
                GetSurface(),
                &capabilities);
            return capabilities;
        }

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
            surfaceCreateInfoWin32.hwnd = hwnd;

            VkSurfaceKHR surface;
            vkCreateWin32SurfaceKHR(
                instance,
                &surfaceCreateInfoWin32,
                nullptr,
                &surface);
            surfaceUnique = VkSurfaceKHRUnique(surface, VkSurfaceKHRDeleter(instance));
#endif
        }

        void BufferSupportCheck()
        {
            if (surfaceCapabilities.maxImageCount != 0 && surfaceCapabilities.maxImageCount < Surface::BufferCount)
            {
                std::runtime_error("Error: surface does not support enough swap images!");
            }
        }

        void ChooseFormat()
        {
            uint32_t formatCount = 0;
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, 
                GetSurface(), 
                &formatCount, 
                nullptr);

            std::vector<VkSurfaceFormatKHR> surfaceFormats;
            surfaceFormats.resize(formatCount);

            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, 
                GetSurface(), 
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
                GetSurface(),
                &presentModeCount,
                nullptr);

            std::vector<VkPresentModeKHR> presentModes;
            presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                physicalDevice,
                GetSurface(),
                &presentModeCount,
                presentModes.data());

            auto presentIterator = presentModes.begin();
            auto presentEnd = presentModes.end();
            presentIterator = std::find(presentModes.begin(), presentModes.end(), VK_PRESENT_MODE_MAILBOX_KHR);
            if (presentIterator == presentModes.end())
            {
                presentMode = VK_PRESENT_MODE_FIFO_KHR;
            }
            else
            {
                presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            }
        }

    };
}