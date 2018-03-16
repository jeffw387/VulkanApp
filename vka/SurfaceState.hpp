#pragma once

#include "vulkan/vulkan.hpp"
#include "GLFW/glfw3.h"
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
        bool surfaceNeedsRecreation = false;
		std::mutex recreateSurfaceMutex;
		std::condition_variable recreateSurfaceCondition;
		vk::UniqueSurfaceKHR surface;
		vk::Extent2D surfaceExtent;
		vk::Format surfaceFormat;
		vk::FormatProperties surfaceFormatProperties;
		vk::SurfaceCapabilitiesKHR surfaceCapabilities;
		std::vector<vk::PresentModeKHR> surfacePresentModes;
		vk::ColorSpaceKHR surfaceColorSpace;		
        vk::PresentModeKHR presentMode;
    };

	void CreateSurface(SurfaceState& surfaceState, 
		ApplicationState& appState, 
		const InstanceState& instanceState, 
		const DeviceState& deviceState, 
		const vk::Extent2D& extent)
	{
		VkSurfaceKHR surface;
		glfwCreateWindowSurface(instanceState.instance.get(), appState.window, nullptr, &surface);
		surfaceState.surface = vk::UniqueSurfaceKHR(surface, vk::SurfaceKHRDeleter(instanceState.instance.get()));
		
		// get surface format from supported list
		auto surfaceFormats = deviceState.physicalDevice.getSurfaceFormatsKHR(surfaceState.surface.get());
		if (surfaceFormats.size() == 1 && surfaceFormats[0].format == vk::Format::eUndefined)
			surfaceState.surfaceFormat = vk::Format::eB8G8R8A8Unorm;
		else
			surfaceState.surfaceFormat = surfaceFormats[0].format;
		// get surface color space
		surfaceState.surfaceColorSpace = surfaceFormats[0].colorSpace;

		surfaceState.surfaceFormatProperties = deviceState.physicalDevice.getFormatProperties(vk::Format::eR8G8B8A8Unorm);
		surfaceState.surfaceCapabilities = deviceState.physicalDevice.getSurfaceCapabilitiesKHR(surfaceState.surface.get());
		surfaceState.surfacePresentModes = deviceState.physicalDevice.getSurfacePresentModesKHR(surfaceState.surface.get());

		if (!(surfaceState.surfaceFormatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachmentBlend))
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

	void SelectPresentMode(SurfaceState& surfaceState)
	{
		// default present mode: immediate
		surfaceState.presentMode = vk::PresentModeKHR::eImmediate;

		// use mailbox present mode if supported
		auto mailboxPresentMode = std::find(
			surfaceState.surfacePresentModes.begin(), 
			surfaceState.surfacePresentModes.end(), 
			vk::PresentModeKHR::eMailbox);
		if (mailboxPresentMode != surfaceState.surfacePresentModes.end())
		{
			surfaceState.presentMode = *mailboxPresentMode;
		}
	}
}