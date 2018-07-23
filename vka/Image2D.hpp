#pragma once

#undef max
#include "vulkan/vulkan.h"
#include "VulkanFunctions.hpp"
#include "Allocator.hpp"
#include "Buffer.hpp"
#include "Bitmap.hpp"
#include "UniqueVulkan.hpp"
#include "VulkanInitializers.hpp"

#include <limits>
#include <vector>

namespace vka
{
	struct Image2D
	{
		VkImage image;
		VkImageUsageFlags usage;
		VkFormat format;
		uint32_t width;
		uint32_t height;
		VkImageView view;
		VkImageLayout currentLayout;
		VkImageAspectFlags aspects;
		AllocationHandle allocation;
	};
}