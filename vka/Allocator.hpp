#pragma once

#include "vulkan/vulkan.h"
#include "UniqueVulkan.hpp"
#include <memory>
#include <optional>
#include <map>
#include <list>

namespace vka
{
    struct AllocationHandle
    {
        VkDeviceMemory memory;
        VkDeviceSize size;
        VkDeviceSize offsetInDeviceMemory;
        uint32_t typeID;
    };

    struct MemoryBlock;
    struct AllocationHandleDeleter
    {
        using pointer = AllocationHandle;
        void operator()(AllocationHandle allocation);
        AllocationHandleDeleter(const AllocationHandleDeleter&) noexcept = default;
        AllocationHandleDeleter() noexcept = default;
        MemoryBlock* block;
    };
    using UniqueAllocationHandle = std::unique_ptr<AllocationHandle, AllocationHandleDeleter>;

    struct Allocation
    {
        VkDeviceSize size = 0U;
        bool allocated = false;
    };
    using AllocationMap = std::map<VkDeviceSize, Allocation>;

    struct MemoryBlock
    {
        MemoryBlock(VkDeviceMemoryUnique&& memory, const VkMemoryAllocateInfo& allocateInfo);
        std::optional<VkDeviceSize> CanAllocate(const VkMemoryRequirements& requirements);
        VkDeviceSize DivideAllocation(
            const VkDeviceSize allocationOffset,
            const VkMemoryRequirements& requirements);
        UniqueAllocationHandle CreateHandleFromAllocation(VkDeviceSize allocationOffset);
        void DeallocateMemory(AllocationHandle allocation);

        VkDeviceMemoryUnique m_DeviceMemory;
        VkMemoryAllocateInfo m_AllocateInfo;
        AllocationMap m_Allocations;
    };

    class Allocator
    {
    public:
        static const VkDeviceSize DefaultMemoryBlockSize = 1000000U;
        Allocator() = default;
        Allocator(VkPhysicalDevice physicalDevice, 
            VkDevice device, 
            VkDeviceSize defaultBlockSize = DefaultMemoryBlockSize);
        std::optional<uint32_t> Allocator::ChooseMemoryType(VkMemoryPropertyFlags memoryFlags, 
        const VkMemoryRequirements& requirements);
        UniqueAllocationHandle AllocateMemory(const bool DedicatedAllocation, 
        const VkMemoryRequirements& requirements, 
        const VkMemoryPropertyFlags memoryFlags);
        UniqueAllocationHandle AllocateForImage(
            const bool DedicatedAllocation, 
            const VkImage image, 
            const VkMemoryPropertyFlags memoryFlags);
        UniqueAllocationHandle AllocateForBuffer(
            const bool DedicatedAllocation, 
            const VkBuffer buffer, 
            const VkMemoryPropertyFlags memoryFlags);

    private:
        MemoryBlock& AllocateNewBlock(const VkMemoryAllocateInfo& allocateInfo);

        std::list<MemoryBlock> m_MemoryBlocks;
        VkPhysicalDevice m_PhysicalDevice;
        VkDevice m_Device;
        VkDeviceSize m_DefaultBlockSize;
        VkPhysicalDeviceMemoryProperties m_MemoryProperties;
    };
}// namespace vka