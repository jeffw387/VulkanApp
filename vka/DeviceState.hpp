#pragma once

#include "vulkan/vulkan.h"
#include "glm/glm.hpp"
#include "Allocator.hpp"
#include "UniqueVulkan.hpp"
#include <vector>
#include <iostream>

namespace vka
{
	struct DeviceState
    {
        VkPhysicalDevice physicalDevice;
		uint32_t graphicsQueueFamilyID;
		float graphicsPriority;
		VkDeviceQueueCreateInfo queueCreateInfo;
		VkDeviceUnique device;
		VkQueue graphicsQueue;
		Allocator allocator;
    };

	static void InitPhysicalDevice(DeviceState& deviceState, const VkInstance& instance)
	{
		std::vector<VkPhysicalDevice> physicalDevices;
		uint32_t physicalDeviceCount;
		vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
		physicalDevices.resize(physicalDeviceCount);
		auto result = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());
		
		if (physicalDevices.size() > 1)
		{
			deviceState.physicalDevice = physicalDevices[1];
			std::cout << "Picked device 1.\n";
		}
		else
		{
			deviceState.physicalDevice = physicalDevices[0];
			std::cout << "Picked device 0.\n";
		}
	}

	static void SelectGraphicsQueue(DeviceState& deviceState)
	{
		uint32_t propCount;
		std::vector<VkQueueFamilyProperties> queueFamilyProperties;
		vkGetPhysicalDeviceQueueFamilyProperties(deviceState.physicalDevice, &propCount, nullptr);
		queueFamilyProperties.resize(propCount);
		vkGetPhysicalDeviceQueueFamilyProperties(deviceState.physicalDevice, 
			&propCount, 
			queueFamilyProperties.data());
		
		auto graphicsQueueInfo = std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(),
			[](VkQueueFamilyProperties q)
			{
				return q.queueFlags & VK_QUEUE_GRAPHICS_BIT;
			});

		// this gets the position of the queue, which is also its family ID
		deviceState.graphicsQueueFamilyID = static_cast<uint32_t>(graphicsQueueInfo - queueFamilyProperties.begin());
		// priority of 0 since we only have one queue
		deviceState.graphicsPriority = 0.f;
		deviceState.queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		deviceState.queueCreateInfo.pNext = nullptr;
		deviceState.queueCreateInfo.flags = (VkDeviceQueueCreateFlags)0;
		deviceState.queueCreateInfo.queueFamilyIndex = deviceState.graphicsQueueFamilyID;
		deviceState.queueCreateInfo.queueCount = 1;
		deviceState.queueCreateInfo.pQueuePriorities = &deviceState.graphicsPriority;
	}

	static void CreateLogicalDevice(DeviceState& deviceState, const std::vector<const char*>& deviceExtensions)
	{
		// VkPhysicalDeviceFeatures physicalDeviceFeatures;
		// vkGetPhysicalDeviceFeatures(deviceState.physicalDevice, &physicalDeviceFeatures);

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pNext = nullptr;
		deviceCreateInfo.flags = (VkDeviceCreateFlags)0;
		deviceCreateInfo.queueCreateInfoCount = 1;
		deviceCreateInfo.pQueueCreateInfos = &deviceState.queueCreateInfo;
		deviceCreateInfo.enabledLayerCount = 0;
		deviceCreateInfo.ppEnabledLayerNames = nullptr;
		deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		deviceCreateInfo.pEnabledFeatures = nullptr;

		VkDevice device;
		auto result = VkCreateDevice(deviceState.physicalDevice, &deviceCreateInfo, nullptr, &device);
		deviceState.device = VkDeviceUnique(device, VkDeviceDeleter());
	}

	static void GetGraphicsQueue(DeviceState& deviceState)
	{
		vkGetDeviceQueue(deviceState.device.get(), deviceState.graphicsQueueFamilyID, 0U, &deviceState.graphicsQueue);
	}

	static void CreateAllocator(DeviceState& deviceState)
	{
		constexpr auto allocSize = 512000U;
		deviceState.allocator = Allocator(deviceState.physicalDevice, deviceState.device.get(), VkDeviceSize(allocSize));
	}
}