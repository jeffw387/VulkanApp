#pragma once

#include "UniqueVulkan.hpp"

namespace vka
{
    struct CopyState
    {
		VkCommandPoolUnique copyCommandPool;
		VkCommandBuffer copyCommandBuffer;
		VkFenceUnique copyCommandFence;
    };

    static void InitCopyStructures(CopyState& copyState, const DeviceState& deviceState)
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
		copyState.copyCommandPool = VkCommandPoolUnique(commandPool, VkCommandPoolDeleter(device));

		VkCommandBufferAllocateInfo bufferAllocateInfo;
		bufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		bufferAllocateInfo.pNext = nullptr;
		bufferAllocateInfo.commandPool = commandPool;
		bufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		bufferAllocateInfo.commandBufferCount = 1;

		auto cmdBufferResult = vkAllocateCommandBuffers(device, &bufferAllocateInfo, &copyState.copyCommandBuffer);

		VkFenceCreateInfo fenceCreateInfo;
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.pNext = nullptr;
		fenceCreateInfo.flags = VkFenceCreateFlags(0);

		VkFence fence;
		auto fenceResult = vkCreateFence(device, &fenceCreateInfo, nullptr, &fence);
		copyState.copyCommandFence = VkFenceUnique(fence, VkFenceDeleter(device));
    }
}