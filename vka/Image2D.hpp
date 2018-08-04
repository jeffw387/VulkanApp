#pragma once

#undef max
#include "Allocator.hpp"
#include "Bitmap.hpp"
#include "Buffer.hpp"
#include "UniqueVulkan.hpp"
#include "VulkanFunctions.hpp"
#include "VulkanInitializers.hpp"
#include "vulkan/vulkan.h"

#include <limits>
#include <vector>

namespace vka {
struct Image2D {
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
}  // namespace vka