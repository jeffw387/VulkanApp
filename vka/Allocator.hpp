#pragma once

#include "vulkan/vulkan.h"
#include "VulkanFunctions.hpp"
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

    bool operator!=(const AllocationHandle& lhs, const AllocationHandle& rhs);

    struct MemoryBlock;
    struct AllocationHandleDeleter
    {
        using pointer = AllocationHandle;
        MemoryBlock* block;

        void functor(AllocationHandle allocation);
        
        void operator()(AllocationHandle allocation)
        {
            functor(allocation);
        }

        AllocationHandleDeleter(MemoryBlock* block) : block(block)
        {}

        AllocationHandleDeleter() noexcept = default;
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
        MemoryBlock(const VkDevice& device, const VkMemoryAllocateInfo& allocateInfo);
        MemoryBlock(MemoryBlock&& other) = default;
        MemoryBlock& operator =(MemoryBlock&& other) = default;
        std::optional<VkDeviceSize> CanAllocate(const VkMemoryRequirements& requirements);
        VkDeviceSize DivideAllocation(
            const VkDeviceSize allocationOffset,
            const VkMemoryRequirements& requirements);
        UniqueAllocationHandle CreateHandleFromAllocation(VkDeviceSize allocationOffset);
        void DeallocateMemory(AllocationHandle allocation);

        VkDevice device;
        VkDeviceMemoryUnique deviceMemory;
        VkMemoryAllocateInfo allocateInfo;
        AllocationMap allocations;
    };

    class Allocator
    {
    public:
        static const VkDeviceSize DefaultMemoryBlockSize = 1000000U;
        Allocator() = default;
        Allocator(Allocator&& other) = default;
        Allocator& operator =(Allocator&& other) = default;
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

        std::list<MemoryBlock> memoryBlocks;
        VkPhysicalDevice physicalDevice;
        VkDevice device;
        VkDeviceSize defaultBlockSize;
        VkPhysicalDeviceMemoryProperties memoryProperties;
    };
}// namespace vka