#pragma once
#include "Allocator.hpp"
#include "UniqueVulkan.hpp"
#include "VulkanFunctions.hpp"
#include "gsl.hpp"
#include "vulkan/vulkan.h"

#include <vector>

namespace vka {
struct BufferData {
  VkBuffer buffer;
  AllocationHandle allocation;
  VkDeviceSize size;
  VkBufferUsageFlags usage;
  bool dedicatedAllocation;
  bool mapped;
  void* mapPtr;
};
}  // namespace vka