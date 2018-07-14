#pragma once

#include "vulkan/vulkan.h"
#include "vka/UniqueVulkan.hpp"

constexpr size_t BufferCount = 3;

namespace vka
{

	struct VulkanState
	{
		LibraryHandle VulkanLibrary;
		VkInstanceUnique instanceUnique;
		VkPhysicalDevice physicalDevice;
		VkDeviceUnique deviceUnique;
		VkSurfaceKHRUnique surfaceUnique;
		VkSwapchainKHRUnique swapchainUnique;
		VkRenderPassUnique renderPassUnique;

		struct {
			struct pipeline2D {
				VkDescriptorSetLayoutUnique staticLayout;
				VkDescriptorSetLayoutUnique dynamicLayout;
			};
		} pipelines;

		struct {
			VkDescriptorPool staticDescriptorPool;
			VkDescriptorPool dynamicDescriptorPool;

		} frameData[BufferCount];
	};
}