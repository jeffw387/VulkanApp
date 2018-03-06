#pragma once

#include <vulkan/vulkan.hpp>

namespace vka
{
    struct SurfaceState
    {
        bool surfaceNeedsRecreation = false;
		vk::UniqueSurfaceKHR surface;
		vk::Extent2D surfaceExtent;
		vk::Format surfaceFormat;
		vk::FormatProperties surfaceFormatProperties;
		vk::SurfaceCapabilitiesKHR surfaceCapabilities;
		std::vector<vk::PresentModeKHR> surfacePresentModes;
		vk::ColorSpaceKHR surfaceColorSpace;		
        vk::PresentModeKHR presentMode;
    };
}