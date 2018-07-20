#pragma once
#include "vulkan/vulkan.h"
#include "VulkanFunctions.hpp"
#include "UniqueVulkan.hpp"
#include "gsl.hpp"
#include "Allocator.hpp"

#include <vector>

namespace vka
{
struct UniqueAllocatedBuffer
{
    VkBufferUnique buffer;
    UniqueAllocationHandle allocation;
    VkBufferCreateInfo bufferCreateInfo;
	void* mapPtr;
};

static UniqueAllocatedBuffer CreateBufferUnique(
	VkDevice device,
	Allocator &allocator,
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
                       allocatedBuffer.allocation.get().memory,
                       allocatedBuffer.allocation.get().offsetInDeviceMemory);

    allocatedBuffer.buffer = VkBufferUnique(buffer, VkBufferDeleter(device));

    return std::move(allocatedBuffer);
}

static void CopyToBuffer(
    const VkCommandBuffer commandBuffer,
    const VkQueue graphicsQueue,
    const VkBuffer source,
    const VkBuffer destination,
    const VkBufferCopy &bufferCopy,
    const VkFence fence,
    const std::vector<VkSemaphore> signalSemaphores = {})
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
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pWaitDstStageMask = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = gsl::narrow<uint32_t>(signalSemaphores.size());
    submitInfo.pSignalSemaphores = signalSemaphores.data();

    vkQueueSubmit(graphicsQueue, 1U, &submitInfo, fence);
}
} // namespace vka