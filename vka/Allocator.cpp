#include <stdexcept>
#include <algorithm>

#include "Allocator.hpp"
#include "mymath.hpp"
namespace vka
{
    

    bool operator!=(const AllocationHandle& lhs, const AllocationHandle& rhs)
    {
        return (lhs.memory != rhs.memory) ||
            (lhs.size != rhs.size) ||
            (lhs.offsetInDeviceMemory != rhs.offsetInDeviceMemory) ||
            (lhs.typeID != rhs.typeID);
    }

    MemoryBlock::MemoryBlock(const VkDevice& device, const VkMemoryAllocateInfo& allocateInfo) :
        m_Device(device), m_AllocateInfo(allocateInfo)
    {
        VkDeviceMemory memory;
        auto result = vkAllocateMemory(m_Device, &allocateInfo, nullptr, &memory);
        m_DeviceMemory = VkDeviceMemoryUnique(memory, VkDeviceMemoryDeleter(device));

        Allocation fullBlock = {};
        fullBlock.size = allocateInfo.allocationSize;
        auto fullBlockOffset = 0U;
        m_Allocations[fullBlockOffset] = fullBlock;
    }

    // returns the offset of a allocation that can contain the requested allocation if possible
    std::optional<VkDeviceSize> MemoryBlock::CanAllocate(const VkMemoryRequirements& requirements)
    {
        for (auto& allocPair : m_Allocations)
        {
            if (allocPair.second.allocated)
                continue;
            if ((helper::roundUp(allocPair.first, requirements.alignment) + requirements.size) <= 
                (allocPair.first + allocPair.second.size))
                return std::make_optional<VkDeviceSize>(allocPair.first);
        }
        return std::make_optional<VkDeviceSize>();
    }

    // divides allocation at the given offset, returning the offset of the requested allocation
    // this does not bounds check, CanAllocate() should have been called first
    VkDeviceSize MemoryBlock::DivideAllocation(
        const VkDeviceSize firstOffset,
        const VkMemoryRequirements& requirements)
    {
        // get first allocation from map
        auto& first = m_Allocations.at(firstOffset);
        // get the next offset that matches alignment
        auto nextAlignedOffset = helper::roundUp(firstOffset, requirements.alignment);
        // check if the alignment offset matches the first offset
        if (nextAlignedOffset == firstOffset)
        {
            // divide if possible
            if (first.size > requirements.size)
            {
                auto secondOffset = firstOffset + requirements.size;
                auto& second = m_Allocations[secondOffset];
                second.size = first.size - requirements.size;
                first.size = requirements.size;
            }
            return firstOffset;
        }
        // create second allocation at that offset
        auto& second = m_Allocations[nextAlignedOffset];
        // set second allocation's size (end of original allocation minus beginning of new allocation)
        second.size = firstOffset + first.size - nextAlignedOffset;
        // shrink first allocation
        first.size = nextAlignedOffset - firstOffset;
        // return offset of second allocation
        return nextAlignedOffset;
    }

    UniqueAllocationHandle MemoryBlock::CreateHandleFromAllocation(VkDeviceSize allocationOffset)
    {
        auto& alloc = m_Allocations.at(allocationOffset);
        alloc.allocated = true;

        AllocationHandle handle;
        handle.memory = m_DeviceMemory.get();
        handle.typeID = m_AllocateInfo.memoryTypeIndex;
        handle.size = alloc.size;
        handle.offsetInDeviceMemory = allocationOffset;
        return UniqueAllocationHandle(handle, AllocationHandleDeleter(this));
    }

    void MemoryBlock::DeallocateMemory(AllocationHandle allocation)
    {
        m_Allocations.at(allocation.offsetInDeviceMemory).allocated = false;
    }
    Allocator::Allocator(
        VkPhysicalDevice physicalDevice, 
        VkDevice device, 
        VkDeviceSize defaultBlockSize)
        :
        m_PhysicalDevice(physicalDevice), 
        m_Device(device), 
        m_DefaultBlockSize(defaultBlockSize)
    {
        vkGetPhysicalDeviceMemoryProperties(
            physicalDevice,
            &m_MemoryProperties);
    }

    std::optional<uint32_t> Allocator::ChooseMemoryType(VkMemoryPropertyFlags memoryFlags, 
        const VkMemoryRequirements& requirements)
    {
		for(auto i = 0U; i < m_MemoryProperties.memoryTypeCount; i++)
		{
			if (((1 << i) & requirements.memoryTypeBits) &&
				((m_MemoryProperties.memoryTypes[i].propertyFlags & memoryFlags) == memoryFlags))
			{
                return std::make_optional<uint32_t>(i);
			}
		}
		return std::make_optional<uint32_t>();
    }

    MemoryBlock& Allocator::AllocateNewBlock(const VkMemoryAllocateInfo& allocateInfo)
    {
        
        m_MemoryBlocks.push_back(MemoryBlock(m_Device, allocateInfo));
            
        return m_MemoryBlocks.back();
    }

    UniqueAllocationHandle Allocator::AllocateMemory(
        const bool DedicatedAllocation, 
        const VkMemoryRequirements& requirements, 
        const VkMemoryPropertyFlags memoryFlags)
    {
        auto typeID = ChooseMemoryType(memoryFlags, requirements);
        if (!typeID)
        {
            std::runtime_error("Cannot find matching memory type!");
        }
        
        // if a dedicated allocation is not required, attempt to allocate from existing blocks
        if (!DedicatedAllocation)
        {
            // -iterate over existing memory blocks
            // -check if they match required type
            // -check if they can accomodate the requirements for a new allocation
            for (auto& block : m_MemoryBlocks)
            {
                if (block.m_AllocateInfo.memoryTypeIndex != typeID.value())
                    continue;
                auto optionalOffset = block.CanAllocate(requirements);
                if (!optionalOffset)
                    continue;
                // divide the allocation if possible
                auto newOffset = block.DivideAllocation(*optionalOffset, requirements);

                // create an external AllocationHandle object
                return block.CreateHandleFromAllocation(newOffset);
            }
            // no existing blocks are appropriate
        }
        // allocate a new block
        VkMemoryAllocateInfo allocateInfo = {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.pNext = nullptr;
        allocateInfo.allocationSize = requirements.size;
        allocateInfo.memoryTypeIndex = typeID.value();

        // if no dedicated block is needed, allocate the default block size 
        // or the requested size, whichever is larger
        if (!DedicatedAllocation)
        {
            allocateInfo.allocationSize = std::max<VkDeviceSize>(allocateInfo.allocationSize, m_DefaultBlockSize);
        }
        auto& newBlock = AllocateNewBlock(allocateInfo);
        auto newOffset = newBlock.DivideAllocation(0U, requirements);
        return newBlock.CreateHandleFromAllocation(newOffset);
    }

    UniqueAllocationHandle Allocator::AllocateForImage(
        const bool DedicatedAllocation, 
        const VkImage image, 
        VkMemoryPropertyFlags memoryFlags)
    {
        VkMemoryRequirements requirements = {};
        vkGetImageMemoryRequirements(m_Device, image, &requirements);
        return AllocateMemory(DedicatedAllocation, requirements, memoryFlags);
    }

    UniqueAllocationHandle Allocator::AllocateForBuffer(
        const bool DedicatedAllocation, 
        const VkBuffer buffer, 
        VkMemoryPropertyFlags memoryFlags)
    {
        VkMemoryRequirements requirements = {};
        vkGetBufferMemoryRequirements(m_Device, buffer, &requirements);
        return AllocateMemory(DedicatedAllocation, requirements, memoryFlags);
    }
}