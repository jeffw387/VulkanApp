#pragma once

#include <vulkan/vulkan.hpp>

namespace vka
{
    struct DeviceState
    {
        vk::PhysicalDevice physicalDevice;
		vk::DeviceSize uniformBufferOffsetAlignment;
		vk::DeviceSize matrixBufferOffsetAlignment;
		uint32_t graphicsQueueFamilyID;
		vk::UniqueDevice logicalDevice;
		vk::Queue graphicsQueue;
		vk::UniqueDebugReportCallbackEXT debugBreakpointCallbackData;
		Allocator allocator;
    };
}