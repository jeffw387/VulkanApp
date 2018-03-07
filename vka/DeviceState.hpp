#pragma once

#include "VulkanFunctions.hpp"
#include "vulkan/vulkan.hpp"
#include "glm/glm.hpp"
#include "Allocator.hpp"
#include <vector>

namespace vka
{
    struct DeviceState
    {
        vk::PhysicalDevice physicalDevice;
		vk::DeviceSize uniformBufferOffsetAlignment;
		vk::DeviceSize matrixBufferOffsetAlignment;
		uint32_t graphicsQueueFamilyID;
		float graphicsPriority;
		vk::DeviceQueueCreateInfo queueCreateInfo;
		vk::UniqueDevice logicalDevice;
		vk::Queue graphicsQueue;
		Allocator allocator;
    };

	static void InitPhysicalDevice(DeviceState& deviceState, const vk::Instance& instance)
	{
		auto devices = instance->enumeratePhysicalDevices();
			deviceState.physicalDevice = devices[0];

			deviceState.uniformBufferOffsetAlignment = deviceState.physicalDevice.getProperties().limits.minUniformBufferOffsetAlignment;
			auto offsetsForMatrix = sizeof(glm::mat4) / deviceState.uniformBufferOffsetAlignment;
			if (offsetsForMatrix == 0)
			{
				offsetsForMatrix = 1;
			}
			deviceState.matrixBufferOffsetAlignment = offsetsForMatrix * deviceState.uniformBufferOffsetAlignment;
	}

	static void SelectGraphicsQueue(DeviceState& deviceState)
	{
		auto gpuQueueProperties = deviceState.physicalDevice.getQueueFamilyProperties();
		auto graphicsQueueInfo = std::find_if(gpuQueueProperties.begin(), gpuQueueProperties.end(),
			[&](vk::QueueFamilyProperties q)
			{
				return q.queueFlags & vk::QueueFlagBits::eGraphics;
			});

		// this gets the position of the queue, which is also its family ID
		deviceState.graphicsQueueFamilyID = static_cast<uint32_t>(graphicsQueueInfo - gpuQueueProperties.begin());
		// priority of 0 since we only have one queue
		deviceState.graphicsPriority = 0.f;
		deviceState.queueCreateInfo = vk::DeviceQueueCreateInfo(
			vk::DeviceQueueCreateFlags(),
			deviceState.graphicsQueueFamilyID,
			1U,
			&deviceState.graphicsPriority);
	}

	static void CreateLogicalDevice(DeviceState& deviceState, const std::vector<const char*>& deviceExtensions)
	{
		auto gpuFeatures = deviceState.physicalDevice.getFeatures();
		deviceState.logicalDevice = deviceState.physicalDevice.createDeviceUnique(
			vk::DeviceCreateInfo(vk::DeviceCreateFlags(),
				1,
				&deviceState.queueCreateInfo,
				0,
				nullptr,
				static_cast<uint32_t>(deviceExtensions.size()),
				deviceExtensions.data(),
				&gpuFeatures));
	}

	static void GetGraphicsQueue(DeviceState& deviceState)
	{
		deviceState.graphicsQueue = deviceState.logicalDevice->getQueue(deviceState.graphicsQueueFamilyID, 0U);
	}

	static void CreateAllocator(DeviceState& deviceState)
	{
		deviceState.allocator = Allocator(deviceState.physicalDevice, deviceState.logicalDevice);
	}

	static bool LoadVulkanDeviceFunctions(DeviceState& deviceState)
	{
#define VK_DEVICE_LEVEL_FUNCTION( fun )                                                   		\
	if( !(fun = (PFN_##fun)vkGetDeviceProcAddr( deviceState.logicalDevice.get(), #fun )) ) {    \
		std::cout << "Could not load device level function: " << #fun << "!" << std::endl;  	\
	}

#include "VulkanFunctions.inl"

		return true;
	}
}