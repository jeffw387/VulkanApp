#pragma once

#include "UniqueVulkan.hpp"
#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"

#include <vector>

namespace vka
{
    class Surface
    {
    public:
        Surface(VkInstance instance, VkPhysicalDevice physicalDevice, GLFWwindow* window) : 
            instance(instance), physicalDevice(physicalDevice), window(window)
        {
            GetPlatformHandle();
            CreateSurface();
        }

        VkSurfaceKHR get()
        {
            return surfaceUnique.get();
        }

        VkExtent2D GetSurfaceExtent()
        {
            VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
            auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                physicalDevice,
                get(),
                &surfaceCapabilities);
            return surfaceCapabilities.currentExtent;
        }

        

    private:
        VkInstance instance;
        VkPhysicalDevice physicalDevice;
        VkSurfaceKHRUnique surfaceUnique;
        GLFWwindow* window;
#ifdef VK_USE_PLATFORM_WIN32_KHR
        HWND hwnd;
        HINSTANCE hinstance;
        VkWin32SurfaceCreateInfoKHR surfaceCreateInfoWin32;
#endif

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
        }

    };
}