#include <stdexcept>
#include <algorithm>

#include "VulkanFunctions.hpp"
#include "Allocator.hpp"
#include "mymath.hpp"
namespace vka
{
    void AllocationDeleter::operator()(Allocation* allocation)
    {
        block->DeallocateMemory(*allocation);
        delete allocation;
    }
    MemoryBlock::MemoryBlock(vk::UniqueDeviceMemory&& memory, const vk::MemoryAllocateInfo& allocateInfo) :
        m_DeviceMemory(std::move(memory)), m_AllocateInfo(allocateInfo)
    {
        SubAllocation fullBlock = {};
        fullBlock.size = allocateInfo.allocationSize;
        auto fullBlockOffset = 0U;
        m_Suballocations[fullBlockOffset] = fullBlock;
    }

    // returns the offset of a suballocation that can contain the requested allocation if possible
    std::optional<vk::DeviceSize> MemoryBlock::CanSuballocate(const vk::MemoryRequirements& requirements)
    {
        for (auto& allocPair : m_Suballocations)
        {
            if (allocPair.second.allocated)
                continue;
            if ((helper::roundUp(allocPair.first, requirements.alignment) + requirements.size) <= 
                (allocPair.first + allocPair.second.size))
                return std::make_optional<vk::DeviceSize>(allocPair.first);
        }
        return std::make_optional<vk::DeviceSize>();
    }

    // divides suballocation at the given offset, returning the offset of the requested suballocation
    // this does not bounds check, CanSuballocate() should have been called first
    vk::DeviceSize MemoryBlock::DivideSubAllocation(
        const vk::DeviceSize firstOffset,
        const vk::MemoryRequirements& requirements)
    {
        // get first suballocation from map
        auto& first = m_Suballocations.at(firstOffset);
        // get the next offset that matches alignment
        auto nextAlignedOffset = helper::roundUp(firstOffset, requirements.alignment);
        // check if the alignment offset matches the first offset
        if (nextAlignedOffset == firstOffset)
        {
            // subdivide if possible
            if (first.size > requirements.size)
            {
                auto secondOffset = firstOffset + requirements.size;
                auto& second = m_Suballocations[secondOffset];
                second.size = first.size - requirements.size;
                first.size = requirements.size;
            }
            return firstOffset;
        }
        // create second suballocation at that offset
        auto& second = m_Suballocations[nextAlignedOffset];
        // set second sub's size (end of original sub minus beginning of new sub)
        second.size = firstOffset + first.size - nextAlignedOffset;
        // shrink first sub
        first.size = nextAlignedOffset - firstOffset;
        // return offset of second sub
        return nextAlignedOffset;
    }

    UniqueAllocation MemoryBlock::CreateExternalAllocationFromSuballocation(vk::DeviceSize allocationOffset)
    {
        auto& subAlloc = m_Suballocations.at(allocationOffset);
        AllocationDeleter deleter;
        deleter.block = this;
        UniqueAllocation newAllocation = UniqueAllocation(new Allocation(), deleter);
        newAllocation->memory = m_DeviceMemory.get();
        newAllocation->typeID = m_AllocateInfo.memoryTypeIndex;
        newAllocation->size = subAlloc.size;
        newAllocation->offsetInDeviceMemory = allocationOffset;
        subAlloc.allocated = true;
        return std::move(newAllocation);
    }

    void MemoryBlock::DeallocateMemory(Allocation allocation)
    {
        m_Suballocations.at(allocation.offsetInDeviceMemory).allocated = false;
    }
    Allocator::Allocator(vk::PhysicalDevice physicalDevice, 
        vk::Device device, 
        vk::DeviceSize defaultBlockSize) :
        m_PhysicalDevice(physicalDevice), 
        m_Device(device), 
        m_DefaultBlockSize(defaultBlockSize),
        m_MemoryProperties(m_PhysicalDevice.getMemoryProperties())
    {}

    std::optional<uint32_t> Allocator::ChooseMemoryType(vk::MemoryPropertyFlags memoryFlags, 
        const vk::MemoryRequirements& requirements)
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

    MemoryBlock& Allocator::AllocateNewBlock(const vk::MemoryAllocateInfo& allocateInfo)
    {
        // hopefully avoiding std::move by not naming the block here
        m_MemoryBlocks.push_back(MemoryBlock(
            m_Device.allocateMemoryUnique(allocateInfo), allocateInfo));
        return m_MemoryBlocks.back();
    }

    UniqueAllocation Allocator::AllocateMemory(
        const bool DedicatedAllocation, 
        const vk::MemoryRequirements& requirements, 
        const vk::MemoryPropertyFlags memoryFlags)
    {
        auto typeID = ChooseMemoryType(memoryFlags, requirements);
        if (!typeID)
        {
            std::runtime_error("Cannot find matching memory type!");
        }
        vk::MemoryAllocateInfo allocateInfo = vk::MemoryAllocateInfo(requirements.size, typeID.value());
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
                auto optionalOffset = block.CanSuballocate(requirements);
                if (!optionalOffset)
                    continue;
                // subdivide the allocation if possible
                auto newOffset = block.DivideSubAllocation(*optionalOffset, requirements);

                // create an external Allocation object
                return block.CreateExternalAllocationFromSuballocation(newOffset);
            }
            // no existing blocks are appropriate
        }
        // allocate a new block
        if (!DedicatedAllocation)
        {
            // if no dedicated block is needed, allocate the default block size 
            // or the requested size, whichever is larger
            allocateInfo.allocationSize = std::max<vk::DeviceSize>(allocateInfo.allocationSize, m_DefaultBlockSize);
        }
        auto& newBlock = AllocateNewBlock(allocateInfo);
        auto newOffset = newBlock.DivideSubAllocation(0U, requirements);
        return newBlock.CreateExternalAllocationFromSuballocation(newOffset);
    }

    UniqueAllocation Allocator::AllocateForImage(
        const bool DedicatedAllocation, 
        const vk::Image image, 
        vk::MemoryPropertyFlags memoryFlags)
    {
        auto requirements = m_Device.getImageMemoryRequirements(image);
        return AllocateMemory(DedicatedAllocation, requirements, memoryFlags);
    }

    UniqueAllocation Allocator::AllocateForBuffer(
        const bool DedicatedAllocation, 
        const vk::Buffer buffer, 
        vk::MemoryPropertyFlags memoryFlags)
    {
        auto requirements = m_Device.getBufferMemoryRequirements(buffer);
        return AllocateMemory(DedicatedAllocation, requirements, memoryFlags);
    }
}