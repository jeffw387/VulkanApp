#pragma once
#include "vulkan/vulkan.hpp"
#include "TimeHelper.hpp"
#include "Sprite.hpp"
#include "DeviceState.hpp"
#include <string>
#include <vector>
#include <functional>

namespace vka
{
    struct VulkanApp;
    using ImageLoadFuncPtr = std::function<void()>;
	using UpdateFuncPtr = std::function<void(TimePoint_ms)>;
	using RenderFuncPtr = std::function<void()>;

    struct InitState
	{
		std::string windowTitle;
		int width; 
        int height;
		VkInstanceCreateInfo instanceCreateInfo;
		std::vector<const char*> deviceExtensions;
		std::string vertexShaderPath;
		std::string fragmentShaderPath;
		ImageLoadFuncPtr imageLoadCallback;
		UpdateFuncPtr updateCallback;
		RenderFuncPtr renderCallback;
		VkCommandPoolUnique copyCommandPool;
		VkCommandBuffer copyCommandBuffer;
		VkFenceUnique copyCommandFence;
	};

    static void InitCopyStructures(InitState& initState, const DeviceState& deviceState)
    {
		auto device = deviceState.device.get();
        // create copy command objects
		VkCommandPoolCreateInfo copyPoolCreateInfo = {};
		copyPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		copyPoolCreateInfo.pNext = nullptr;
		copyPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | 
			VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		copyPoolCreateInfo.queueFamilyIndex = deviceState.graphicsQueueFamilyID;

		VkCommandPool commandPool;
		auto cmdPoolResult = vkCreateCommandPool(device, 
			&copyPoolCreateInfo, 
			nullptr, 
			&commandPool);
		VkCommandPoolDeleter poolDeleter;
		poolDeleter.device = device;
		initState.copyCommandPool = VkCommandPoolUnique(commandPool, poolDeleter);

		VkCommandBufferAllocateInfo bufferAllocateInfo;
		bufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		bufferAllocateInfo.pNext = nullptr;
		bufferAllocateInfo.commandPool = commandPool;
		bufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		bufferAllocateInfo.commandBufferCount = 1;

		auto cmdBufferResult = vkAllocateCommandBuffers(device, &bufferAllocateInfo, &initState.copyCommandBuffer);

		VkFenceCreateInfo fenceCreateInfo;
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.pNext = nullptr;
		fenceCreateInfo.flags = VkFenceCreateFlags(0);

		VkFence fence;
		auto fenceResult = vkCreateFence(device, nullptr, &fence);
		VkFenceDeleter fenceDeleter;
		fenceDeleter.device = device;
		initState.copyCommandFence = VkFenceUnique(fence, fenceDeleter);
    }
}