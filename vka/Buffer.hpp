#pragma once
#include "vulkan/vulkan.h"
#include "UniqueVulkan.hpp"

namespace vka
{
    struct UniqueAllocatedBuffer
    {
        VkBufferUnique buffer;
        UniqueAllocationHandle allocation;
        VkBufferCreateInfo bufferCreateInfo;
    };

    UniqueAllocatedBuffer CreateBuffer(VkDevice device, 
        Allocator& allocator, 
        VkDeviceSize size,
        VkBufferUsageFlags usageFlags,
        uint32_t queueFamilyIndex,
        VkMemoryPropertyFlags memoryFlags,
        bool DedicatedAllocation)
    {
        UniqueAllocatedBuffer allocatedBuffer;
        allocatedBuffer.bufferCreateInfo = {};
        
        allocatedBuffer.bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        allocatedBuffer.bufferCreateInfo.pNext = nullptr;
        allocatedBuffer.bufferCreateInfo.flags = VkBufferCreateFlags(0);
        allocatedBuffer.bufferCreateInfo.size = size;
        allocatedBuffer.bufferCreateInfo.usage = usageFlags;
        allocatedBuffer.bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        allocatedBuffer.bufferCreateInfo.queueFamilyIndexCount = 1;
        allocatedBuffer.bufferCreateInfo.pQueueFamilyIndices = &queueFamilyIndex;

        VkBuffer buffer;
        vkCreateBuffer(device,
            &allocatedBuffer.bufferCreateInfo,
            nullptr,
            &buffer);

        allocatedBuffer.allocation = allocator.AllocateForBuffer(DedicatedAllocation, buffer, memoryFlags);

        vkBindBufferMemory(device, buffer,
            allocatedBuffer.allocation.memory,
            allocatedBuffer.allocation.offsetInDeviceMemory)

        auto bufferDeleter = VkBufferDeleter();
        bufferDeleter.device = device;
        allocatedBuffer.buffer = VkBufferUnique(buffer, bufferDeleter);

        return std::move(allocatedBuffer);
    }

    void CopyToBuffer(
			const VkCommandBuffer commandBuffer,
            const VkQueue graphicsQueue,
			const VkBuffer source, 
			const VkBuffer destination, 
			const VkBufferCopy& bufferCopy,
			const std::optional<VkFence> fence,
			const std::vector<VkSemaphore> waitSemaphores = {},
			const std::vector<VkSemaphore> signalSemaphores = {}
		)
		{
            auto cmdBufferBeginInfo = VkCommandBufferBeginInfo();
            cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cmdBufferBeginInfo.pNext = nullptr;
            cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            cmdBufferBeginInfo.pInheritanceInfo = nullptr;

			vkBeginCommandBuffer(commandBuffer, &cmdBufferBeginInfo);
            vkCmdCopyBuffer(commandBuffer, source, destination, 1, &bufferCopy);
            vkEndCommandBuffer(commandBuffer);

            auto submitInfo = VkSubmitInfo();
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.pNext = nullptr;
            submitInfo.waitSemaphoreCount = waitSemaphores.size();
            submitInfo.pWaitSemaphores = waitSemaphores.data();
            submitInfo.pWaitDstStageMask = nullptr;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;
            submitInfo.signalSemaphoreCount = signalSemaphores.size();
            submitInfo.pSignalSemaphores = signalSemaphores.data();

            vkQueueSubmit(graphicsQueue, 1U, &submitInfo, fence.value_or((VkFence)0));
		}
}