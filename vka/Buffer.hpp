#pragma once

#include <vulkan/vulkan.hpp>

namespace vka
{
    struct AllocatedBuffer
    {
        vk::UniqueBuffer buffer;
        vk::BufferCreateInfo bufferCreateInfo;
        UniqueAllocation allocation;
    };

    AllocatedBuffer&& CreateBuffer(vk::Device device, 
        Allocator& allocator, 
        vk::DeviceSize size,
        vk::BufferUsageFlags usageFlags,
        uint32_t queueFamilyIndex,
        vk::MemoryPropertyFlags memoryFlags,
        bool DedicatedAllocation)
    {
        AllocatedBuffer allocatedBuffer;
        allocatedBuffer.bufferCreateInfo = vk::BufferCreateInfo(
            vk::BufferCreateFlags(), 
            size, 
            usageFlags, 
            vk::SharingMode::eExclusive, 
            1U, 
            &queueFamilyIndex);
        allocatedBuffer.buffer = device.createBufferUnique(allocatedBuffer.bufferCreateInfo);
        allocatedBuffer.allocation = allocator.AllocateForBuffer(DedicatedAllocation, allocatedBuffer.buffer.get(), memoryFlags);
        device.bindBufferMemory(allocatedBuffer.buffer.get(),
            allocatedBuffer.allocation->memory,
            allocatedBuffer.allocation->offsetInDeviceMemory);
        return std::move(allocatedBuffer);
    }

    void CopyToBuffer(
			const vk::CommandBuffer commandBuffer,
            const vk::Queue graphicsQueue,
			const vk::Buffer source, 
			const vk::Buffer destination, 
			const vk::BufferCopy bufferCopy,
			const std::optional<vk::Fence> fence,
			const std::vector<vk::Semaphore> waitSemaphores = {},
			const std::vector<vk::Semaphore> signalSemaphores = {}
		)
		{
			commandBuffer.begin(vk::CommandBufferBeginInfo(
				vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

			commandBuffer.copyBuffer(source, destination, 
			{ bufferCopy });

			commandBuffer.end();

			graphicsQueue.submit( 
                { 
                    vk::SubmitInfo(
                        static_cast<uint32_t>(waitSemaphores.size()),
                        waitSemaphores.data(),
                        nullptr,
                        1U,
                        &commandBuffer,
                        static_cast<uint32_t>(signalSemaphores.size()),
                        signalSemaphores.data()) 
                }, 
                fence.value_or(vk::Fence()));
		}
}