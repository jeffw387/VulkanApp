#pragma once

#include <vulkan/vulkan.h>
#include <map>
#include <vector>
#include <memory>
#include <iostream>
#include "math.h"

using MemoryType = uint32_t;

const auto DefaultAllocationSize = 64000000U; // 64,000,000

namespace vke
{
	enum class AllocationStyle
	{
		NewAllocation,
		NeverAllocate,
		NoPreference
	};

	struct MemoryRange
	{
		MemoryType memType = 0;
		VkDeviceSize startOffset = 0;
		VkDeviceSize usableStartOffset = 0;
		VkDeviceSize alignment = 0;
		VkDeviceSize totalSize = 0;
		VkDeviceSize usableSize = 0;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		bool allocated = false;

		operator VkDeviceMemory() { return memory; }

		bool operator ==(MemoryRange other)
		{
			return
				memType == other.memType &&
				startOffset == other.startOffset &&
				alignment == other.alignment &&
				totalSize == other.totalSize &&
				memory == other.memory &&
				allocated == other.allocated;
		}
	};

	class Block
	{
	public:
		Block() : m_SubAllocations{} {}
		Block(MemoryRange memoryRange) : m_SubAllocations{std::make_pair(0, memoryRange) } {}

		operator VkDeviceMemory() { return m_SubAllocations[0].memory; }

		auto subdivideRange(const MemoryRange& range, const VkDeviceSize& totalSize, const VkDeviceSize& usableSize)
		{
			MemoryRange first = { 
				range.memType,
				range.startOffset,
				range.usableStartOffset,
				range.alignment,
				totalSize,
				usableSize,
				range.memory,
				range.allocated
			};
			MemoryRange second = { 
				range.memType,
				range.startOffset + totalSize,
				range.startOffset + totalSize,
				range.alignment,
				range.totalSize - totalSize,
				range.totalSize - totalSize,
				range.memory,
				range.allocated
			};

			m_SubAllocations[first.startOffset] = first;
			m_SubAllocations[second.startOffset] = second;

			return first;
		}

		// finds a range with a total size that accomodates the given usable size and alignment
		auto findSpace(VkDeviceSize usableSize, VkDeviceSize alignment, MemoryRange& resultRange)
		{
			auto totalSize = helper::roundUp(usableSize, alignment);
			bool foundResult = false;
			for (auto& rangePair : m_SubAllocations)
			{
				if (rangePair.second.totalSize >= totalSize && rangePair.second.allocated == false)
				{
					resultRange = rangePair.second;
					foundResult = true;
					break;
				}
			}
			if (foundResult && resultRange.totalSize > totalSize)
			{
				resultRange = subdivideRange(resultRange, totalSize, usableSize);
				resultRange.alignment = alignment;
			}
			return foundResult;
		}

		bool allocateRange(MemoryRange& resultRange)
		{
			auto& foundRange = m_SubAllocations[resultRange.startOffset];
			if (foundRange.allocated == false && foundRange == resultRange)
			{
				foundRange.allocated = true;
				foundRange.usableStartOffset = helper::roundUp(foundRange.startOffset, foundRange.alignment);
				auto startToUsable = foundRange.usableStartOffset - foundRange.startOffset;
				foundRange.usableSize = foundRange.totalSize - startToUsable;
				resultRange = foundRange;
				return true;
			}
			return false;
		}

		bool deallocateRange(MemoryRange& range)
		{
			auto& foundRange = m_SubAllocations[range.startOffset];
			if (foundRange.allocated == true && foundRange == range)
			{
				foundRange.allocated = false;
				range = foundRange;
				defragment();
				return true;
			}
			return false;
		}

		auto allocatedCount()
		{
			uint32_t count = 0;
			for (auto rangePair : m_SubAllocations)
			{
				if (rangePair.second.allocated == true)
				{
					count++;
				}
			}
			return count;
		}

		std::map<VkDeviceSize, MemoryRange> m_SubAllocations;
		bool m_Mapped = false;

		void defragment()
		{
			auto current = m_SubAllocations.begin();
			while (current != m_SubAllocations.end())
			{
				auto pair1 = current;
				auto pair2 = ++current;
				if (pair2 == m_SubAllocations.end())
					break;
				// if both ranges are not allocated, merge them
				if (pair1->second.allocated == false && pair2->second.allocated == false)
				{
					// add the second range's size to the first
					pair1->second.totalSize += pair2->second.totalSize;
					// erase the second range
					m_SubAllocations.erase(pair2);
					// keep the current iterator pointing to the first range
					current = pair1;
					continue;
				}
			}
		}
	};

	using BlockMapIterator = std::multimap<MemoryType, Block>::iterator;

	class Allocator
	{
	public:
		Allocator() : m_PhysicalDevice(VK_NULL_HANDLE), m_Device(VK_NULL_HANDLE) {}

		void init(VkPhysicalDevice physicalDevice, VkDevice device)
		{
			m_PhysicalDevice = physicalDevice;
			m_Device = device;
			m_blocksByMemoryType = std::multimap<MemoryType, vke::Block>();
		}

		void cleanup()
		{
			for (auto blockPair : m_blocksByMemoryType)
			{
				vkFreeMemory(m_Device, blockPair.second, nullptr);
			}
		}

		bool allocateBuffer(MemoryRange& resultRange, VkBuffer buffer, VkMemoryPropertyFlags memoryProperties, AllocationStyle style = AllocationStyle::NoPreference)
		{
			VkMemoryRequirements requirements;
			vkGetBufferMemoryRequirements(m_Device, buffer, &requirements);

			MemoryType memoryType = findMemoryType(requirements.memoryTypeBits, memoryProperties);

			return allocateRange(resultRange, memoryType, requirements.size, requirements.alignment, style);
		}

		bool allocateImage(MemoryRange& resultRange, VkImage image, VkMemoryPropertyFlags memoryProperties, AllocationStyle style = AllocationStyle::NoPreference)
		{
			VkMemoryRequirements requirements;
			vkGetImageMemoryRequirements(m_Device, image, &requirements);

			MemoryType memoryType = findMemoryType(requirements.memoryTypeBits, memoryProperties);
			return allocateRange(resultRange, memoryType, requirements.size, requirements.alignment, style);
		}

		bool findAndAllocateRange(BlockMapIterator& block, MemoryRange& range, MemoryType memType, VkDeviceSize usableSize, VkDeviceSize alignment)
		{
			BlockMapIterator foundBlock;
			if (findBlockSpace(memType, usableSize, alignment, foundBlock, range))
			{
				if (foundBlock->second.findSpace(usableSize, alignment, range))
				{
					range.alignment = alignment;
					return (foundBlock->second.allocateRange(range));
				}
			}
			return false;
		}

		bool allocateRange(MemoryRange& resultRange, MemoryType memType, VkDeviceSize usableSize, VkDeviceSize alignment, AllocationStyle style = AllocationStyle::NoPreference)
		{
			switch (style)
			{
				case AllocationStyle::NoPreference:
				{
					BlockMapIterator block;
					if (findAndAllocateRange(block, resultRange, memType, usableSize, alignment))
					{
						return true;
					}
					auto allocationSize = std::max(usableSize, static_cast<VkDeviceSize>(DefaultAllocationSize));
					block = allocateMemory(memType, alignment, allocationSize);
					if (block->second.findSpace(usableSize, alignment, resultRange))
					{
						return block->second.allocateRange(resultRange);
					}
					return false;
				}
				case AllocationStyle::NewAllocation:
				{
					auto totalSize = helper::roundUp(usableSize, alignment);
					auto newBlock = allocateMemory(memType, alignment, totalSize);
					if (newBlock->second.findSpace(usableSize, alignment, resultRange))
					{
						return newBlock->second.allocateRange(resultRange);
					}
					return false;
				}
				case AllocationStyle::NeverAllocate:
				{
					if (usableSize > DefaultAllocationSize)
					{
						throw std::runtime_error("Error: attempted to allocate a memory range larger than the default allocation size.\n");
					}
					BlockMapIterator block;
					return findAndAllocateRange(block, resultRange, memType, usableSize, alignment);
				}
			}
			return false;
		}

		bool deallocateRange(MemoryRange& range)
		{
			bool result = false;
			// look through all Blocks with the same memory type
			auto matchingBlockPairs = m_blocksByMemoryType.equal_range(range.memType);
			for (auto it = matchingBlockPairs.first; it != matchingBlockPairs.second; )
			{
				// find the matching VkDeviceMemory handle
				if (it->second == range.memory)
				{
					// attempt to deallocate the given range
					result = it->second.deallocateRange(range);
					
					// if the block has no allocations, free the memory and delete the block
					if (it->second.allocatedCount() == 0)
					{
						vkFreeMemory(m_Device, it->second, nullptr);

						// this points "it" to the next block after the erased one
						it = m_blocksByMemoryType.erase(it);
						break;
					}
					return result;
				}
				++it;
			}
			return result;
		}

		bool mapMemoryRange(MemoryRange& range, void*& result)
		{
			BlockMapIterator containingBlock;
			if (findContainingBlock(range, containingBlock))
			{
				if (containingBlock->second.m_Mapped == false)
				{
					void* data;
					if (vkMapMemory(m_Device, range, range.usableStartOffset, range.usableSize, 0, &data) == VK_SUCCESS)
					{
						result = data;
						containingBlock->second.m_Mapped = true;
						return true;
					}
				}
			}
			return false;
		}

		void unmapRange(MemoryRange& range)
		{
			BlockMapIterator containingBlock;
			if (findContainingBlock(range, containingBlock))
			{
				if (containingBlock->second.m_Mapped == true)
				{
					vkUnmapMemory(m_Device, containingBlock->second);
					containingBlock->second.m_Mapped = false;
				}
			}
		}

	private:
		VkPhysicalDevice m_PhysicalDevice;
		VkDevice m_Device;
		std::multimap<MemoryType, Block> m_blocksByMemoryType;

		BlockMapIterator allocateMemory(MemoryType memType, VkDeviceSize alignment, VkDeviceSize totalSize = DefaultAllocationSize)
		{
			VkMemoryAllocateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			info.allocationSize = totalSize;
			info.memoryTypeIndex = memType;
			VkDeviceMemory memory;
			if (vkAllocateMemory(m_Device, &info, nullptr, &memory) != VK_SUCCESS)
			{
				throw std::runtime_error("Error: unable to allocate memory.\n");
			}
			auto block = Block({ memType, 0, 0, alignment, totalSize, totalSize, memory, false});
			auto result = m_blocksByMemoryType.insert(std::make_pair(memType, block));
			return result;
		}

		bool findContainingBlock(const MemoryRange& range, BlockMapIterator& result)
		{
			auto matchingBlockPairs = m_blocksByMemoryType.equal_range(range.memType);
			for (auto it = matchingBlockPairs.first; it != matchingBlockPairs.second; ++it)
			{
				if (it->second == range.memory)
				{
					result = it;
					return true;
				}
			}
			return false;
		}

		bool findBlockSpace(MemoryType memType, VkDeviceSize usableSize, VkDeviceSize alignment, BlockMapIterator& result, MemoryRange& range)
		{
			auto matchingBlockPairs = m_blocksByMemoryType.equal_range(memType);
			for (auto it = matchingBlockPairs.first; it != matchingBlockPairs.second; ++it)
			{
				if (it->second.findSpace(usableSize, alignment, range))
				{
					result = it;
					return true;
				}
			}
			return false;
		}

		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) 
		{
			VkPhysicalDeviceMemoryProperties memProperties;
			vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);

			for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) 
			{
				if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) 
				{
					return i;
				}
			}

			throw std::runtime_error("failed to find suitable memory type!");
		}
	};
}