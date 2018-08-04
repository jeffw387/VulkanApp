#pragma once
#include "vulkan/vulkan.h"
#include "VulkanFunctions.hpp"
#include "UniqueVulkan.hpp"
#include "gsl.hpp"
#include "Allocator.hpp"

#include <vector>

namespace vka
{
struct BufferData
{
	VkBuffer buffer;
    AllocationHandle allocation;
	VkDeviceSize size;
	VkBufferUsageFlags usage;
	bool dedicatedAllocation;
	bool mapped;
	void* mapPtr;
};
} // namespace vka