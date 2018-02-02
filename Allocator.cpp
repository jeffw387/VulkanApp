#include <stdexcept>
#include <algorithm>

#include "VulkanFunctions.hpp"
#include "Allocator.hpp"
#include "mymath.hpp"
namespace vka
{
    namespace internal
    {
        bool CanSuballocate(
            const vk::DeviceSize offset,
            const vk::DeviceSize size,
            const vk::MemoryRequirements& requirements)
        {
            if (subAllocation.allocated)
                return false;
            auto nextAlignedOffset = helper::roundUp(offset, requirements.alignment);
            // check if aligned offset + required size is within original allocation
            return ((nextAlignedOffset + requirements.size) <= (offset + size))
        }

        MemoryBlock::MemoryBlock(vk::UniqueDeviceMemory&& memory, const vk::MemoryAllocateInfo& allocateInfo) :
            m_DeviceMemory(std::move(memory)), m_Type(allocateInfo.memoryTypeIndex)
        {
            SubAllocation fullBlock = {};
            fullBlock.size = allocateInfo.allocationSize;
            auto fullBlockOffset = 0U;
            m_Suballocations[fullBlockOffset] = fullBlock;
        }

        // divides suballocation at the given offset, returning the offset of the second suballocation
        // this does not bounds check, CanSuballocate() should have been called first
        vk::DeviceSize MemoryBlock::DivideSubAllocation(
            const vk::DeviceSize firstOffset,
            const vk::MemoryRequirements& requirements)
        {
            // get first suballocation from map
            auto& first = m_Suballocations.at(firstOffset);
            // get the next offset that matches alignment
            auto nextAlignedOffset = helper::roundUp(firstOffset, requirements.alignment);
            // create second suballocation at that offset
            auto& second = m_Suballocations[nextAlignedOffset];
            // set second sub's size
            auto newAllocSize = firstOffset + first.size - nextAlignedOffset;
            second.size = newAllocSize;
            // shrink first sub
            auto offsetDistance = nextAlignedOffset - firstOffset;
            first.size = offsetDistance;
            // return offset of second sub
            return nextAlignedOffset;
        }

        Allocation MemoryBlock::CreateExternalAllocationFromSuballocation(vk::DeviceSize allocationOffset)
        {
            auto& subAlloc = m_Suballocations.at(allocationOffset);
            Allocation newAllocation;
            newAllocation.memory = m_DeviceMemory;
            newAllocation.typeID = m_AllocateInfo.memoryTypeIndex;
            newAllocation.size = subAlloc.size;
            newAllocation.offsetInDeviceMemory = allocationOffset;
            subAlloc.allocated = true;
            return newAllocation;
        }

        void MemoryBlock::DeallocateMemory(Allocation allocation)
        {
            m_Suballocations.at(allocation.offsetInDeviceMemory).allocated = false;
        }
    }// namespace internal
    Allocator::Allocator(vk::PhysicalDevice physicalDevice, 
        vk::Device device, 
        vk::DeviceSize defaultBlockSize) :
        m_PhysicalDevice(physicalDevice), 
        m_Device(device), 
        m_DefaultBlockSize(defaultBlockSize),
        m_MemoryProperties(m_PhysicalDevice.getMemoryProperties()))
    {}

    std::optional<uint32_t> Allocator::ChooseMemoryType(vk::MemoryPropertyFlags memoryFlags)
    {
		for(auto i = 0U; i < m_MemoryProperties.memoryTypeCount; i++)
		{
			if (((1 << i) & vertexBufferMemoryRequirements.memoryTypeBits) &&
				((m_MemoryProperties.memoryTypes[i].memoryFlags & memoryFlags) == memoryFlags))
			{
                return std::make_optional<uint32_t>(i);
			}
		}
		return std::make_optional<uint32_t>();
    }

    internal::MemoryBlock& Allocator::AllocateNewBlock(const vk::MemoryAllocateInfo& allocateInfo)
    {
        // hopefully avoiding std::move by not naming the block here
        m_MemoryBlocks.push_back(internal::MemoryBlock(
            m_Device.allocateMemoryUnique(allocateInfo), 
            allocateInfo.memoryTypeIndex, 
            allocateInfo.allocationSize));
        return m_MemoryBlocks.back();
    }

    UniqueAllocation Allocator::AllocateMemory(
        bool DedicatedAllocation, 
        vk::MemoryRequirements requirements, 
        vk::MemoryPropertyFlags memoryFlags)
    {
        auto typeID = ChooseMemoryType(memoryFlags);
        if (!typeID)
        {
            std::runtime_error("Cannot find matching memory type!");
        }
        vk::MemoryAllocateInfo allocateInfo = vk::MemoryAllocateInfo(requirements.size, typeID.value());
        // if a dedicated allocation is not required, attempt to allocate from existing blocks
        if (!DedicatedAllocation)
        {
            // search for a block that matches the type and has space
            auto matchingMemoryBlock = std::find_if(
                std::begin(m_MemoryBlocks), 
                std::end(m_MemoryBlocks), 
                [typeID, requirements](const auto& block)
                {
                    if (block.m_AllocateInfo.memoryTypeIndex != typeID.value())
                        return false;
                    std::for_each(
                        std::begin(block.m_Suballocations), 
                        std::end(block.m_Suballocations), 
                        [requirements](auto subAlloc)
                        {
                            return internal::CanSuballocate(subAlloc, requirements);
                        }
                    );
                }
            );
            // if an appropriate block is found, allocate from it
            if (matchingMemoryBlock != m_MemoryBlocks.end())
            {
                // search for a suballocation that will fit the new allocation
                auto matchingSubAlloc = std::find_if(
                    std::begin(matchingMemoryBlock->m_Suballocations), 
                    std::end(matchingMemoryBlock->m_Suballocations), 
                    [requirements](auto subAlloc)
                    {
                        return internal::CanSuballocate(subAlloc, requirements);
                    }
                );
                if (matchingSubAlloc != std::end(matchingMemoryBlock->m_Suballocations))
                {
                    // subdivide the allocation
                    auto newOffset = matchingMemoryBlock->DivideSubAllocation(*matchingSubAlloc, requirements);

                    // create an external Allocation object
                    auto newAllocation = matchingMemoryBlock->CreateAllocationFromSuballocation(newOffset);
                    AllocationDeleter deleter;
                    deleter.block = &(*matchingMemoryBlock);
                    return UniqueAllocation(newAllocation, deleter);
                }
            }
        }
        // if a dedicated block is needed or no appropriate block is found, allocate a new block
        if (!DedicatedAllocation)
        {
            // if no dedicated block is needed, allocate the default block size 
            // or the requested size, whichever is larger
            allocateInfo.allocationSize = std::max(allocateInfo.allocationSize, m_DefaultBlockSize);
        }
        auto& newBlock = AllocateNewBlock(allocateInfo);
        auto newAllocation = newBlock.CreateAllocationFromSuballocation(0);
        AllocationDeleter deleter;
        deleter.block = &newBlock;
        return UniqueAllocation(newAllocation, deleter);
    }

    UniqueAllocation Allocator::AllocateForImage(
        bool DedicatedAllocation, 
        vk::Image image, 
        vk::MemoryPropertyFlags memoryFlags)
    {
        auto requirements = m_Device.getImageMemoryRequirements(image);
        return AllocateMemory(DedicatedAllocation, requirements, memoryFlags);
    }

    UniqueAllocation Allocator::AllocateForBuffer(
        bool DedicatedAllocation, 
        vk::Buffer buffer, 
        vk::MemoryPropertyFlags memoryFlags)
    {
        auto requirements = m_Device.getBufferMemoryRequirements(buffer);
        return AllocateMemory(DedicatedAllocation, requirements, memoryFlags);
    }
}