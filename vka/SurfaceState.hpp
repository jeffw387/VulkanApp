#pragma once

#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"
#include "ApplicationState.hpp"
#include "InstanceState.hpp"
#include "DeviceState.hpp"
#include <vector>
#include <optional>
#include <mutex>
#include <condition_variable>

namespace vka
{
    struct SurfaceState
    {
		VkSurfaceKHRUnique surface;
		VkExtent2D surfaceExtent;
		VkFormat surfaceFormat;
		VkFormatProperties surfaceFormatProperties;
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		VkColorSpaceKHR surfaceColorSpace;		
		std::vector<VkPresentModeKHR> surfacePresentModes;
        VkPresentModeKHR presentMode;
    };

	static void CreateSurface(SurfaceState& surfaceState, 
		ApplicationState& appState, 
		const InstanceState& instanceState, 
		const DeviceState& deviceState, 
		const VkExtent2D& extent)
	{
		// TODO: remove platform dependence here
		auto win32Window = glfwGetWin32Window(appState.window);
		auto hinstance = GetModuleHandle(NULL);

		VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
		surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surfaceCreateInfo.pNext = nullptr;
		surfaceCreateInfo.flags = (VkWin32SurfaceCreateFlagsKHR)0;
		surfaceCreateInfo.hinstance = hinstance;
		surfaceCreateInfo.hwnd = win32Window;

		VkSurfaceKHR surface;
		vkCreateWin32SurfaceKHR(
			instanceState.instance.get(),
			&surfaceCreateInfo,
			nullptr,
			&surface);

		VkSurfaceKHRDeleter surfaceDeleter;
		surfaceDeleter.instance = instanceState.instance.get();
		surfaceState.surface = VkSurfaceKHRUnique(surface, surfaceDeleter);
		
		// get surface format from supported list
		vkGetPhysicalDeviceSurfaceFormatsKHR(deviceState.physicalDevice, 
			surface, 
			&formatCount, 
			nullptr);

		std::vector<VkSurfaceFormatKHR> surfaceFormats;
		surfaceFormats.resize(formatCount);

		vkGetPhysicalDeviceSurfaceFormatsKHR(deviceState.physicalDevice, 
			surface, 
			&formatCount, 
			surfaceFormats.data());

		if (surfaceFormats.size() == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
			surfaceState.surfaceFormat = VK_FORMAT_B8G8R8_UNORM;
		else
			surfaceState.surfaceFormat = surfaceFormats[0].format;
		// get surface color space
		surfaceState.surfaceColorSpace = surfaceFormats[0].colorSpace;

		vkGetPhysicalDeviceFormatProperties(
			deviceState.physicalDevice,
			surfaceState.surfaceFormat,
			&surfaceState.surfaceFormatProperties);

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
			deviceState.physicalDevice,
			surface,
			&surfaceState.surfaceCapabilities);


		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(
    		deviceState.physicalDevice,
    		surface,
    		&presentModeCount,
    		nullptr);

		surfaceState.surfacePresentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(
    		deviceState.physicalDevice,
    		surface,
    		&presentModeCount,
    		surfaceState.surfacePresentModes.data());

		if (!(surfaceState.surfaceFormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT))
		{
			std::runtime_error("Blend not supported by surface format.");
		}
		// get surface extent
		if (surfaceState.surfaceCapabilities.currentExtent.width == -1 ||
			surfaceState.surfaceCapabilities.currentExtent.height == -1)
		{
			surfaceState.surfaceExtent = extent;
		}
		else
		{
			surfaceState.surfaceExtent = surfaceState.surfaceCapabilities.currentExtent;
		}
	}

	static void SelectPresentMode(SurfaceState& surfaceState)
	{
		// default present mode: immediate
		surfaceState.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;

		// use mailbox present mode if supported
		auto mailboxPresentMode = std::find(
			surfaceState.surfacePresentModes.begin(), 
			surfaceState.surfacePresentModes.end(), 
			VK_PRESENT_MODE_MAILBOX_KHR);
		if (mailboxPresentMode != surfaceState.surfacePresentModes.end())
		{
			surfaceState.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
		}
	}
}