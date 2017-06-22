#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <map>
#include "math.h"

namespace vke
{
	class Block;
	class Allocator;

	// TODO: maybe distribute this as a shared_ptr
	class Chunk
	{
		friend class Allocator;
		friend class Block;
	public:
		Chunk() : isMapped(false), offset(0), size(0), mappedMemory(nullptr), m_memory(0), m_allocated(false)
		{
		}

		bool isMapped;
		VkDeviceSize offset;
		VkDeviceSize size;
		void* mappedMemory;

		void map(VkDevice device)
		{
			if (isMapped)
			{
				return;
			}
			vkMapMemory(device, m_memory, offset, size, 0, &mappedMemory);
			isMapped = true;
		}

		void unmap(VkDevice device)
		{
			if (isMapped)
			{
				vkUnmapMemory(device, m_memory);
				isMapped = false;
			}
		}

		void deallocate()
		{
			auto shared = m_block.lock();
			if (m_allocated && shared)
			{
				shared->deallocateChunk(offset);
			}
			else
			{
				m_allocated = false;
			}
		}

	private:
		std::weak_ptr<Block> m_block;
		VkDeviceMemory m_memory;
		bool m_allocated;
	};

	class Block
	{
		friend class Allocator;
	public:
		Block(VkDevice device, VkDeviceSize size, uint32_t memoryType, VkDeviceSize minimumAlignment)
		{
			VkMemoryAllocateInfo info = {};
			info.allocationSize = size;
			info.memoryTypeIndex = memoryType;
			auto block = std::make_unique<Block>();
			vkAllocateMemory(device, &info, nullptr, &m_memory);
			m_size = size;
			m_alignment = minimumAlignment;
			auto initialOffset = 0U;
			createChunk(initialOffset, size);
		}

		~Block()
		{
			if (m_device && m_memory)
				vkFreeMemory(m_device, m_memory, nullptr);
		}

		VkDeviceMemory getMemory()
		{
			return m_memory;
		}

		std::shared_ptr<Chunk> allocateChunk(VkDeviceSize size)
		{
			// round allocation size up to nearest multiple of minimum alignment
			auto allocSize = helper::roundUp(size, m_alignment);
			for (auto chunk : m_chunkMap)
			{
				if (chunk.second->size > allocSize)
				{
					// return entire chunk if the size matches exactly
					if (chunk.second->size == allocSize)
					{
						chunk.second->m_allocated = true;
						return chunk.second;
					}
					// otherwise subdivide the chunk before returning it
					auto remainder = chunk.second->size - allocSize;
					chunk.second->size = allocSize;
					createChunk(chunk.first + allocSize, remainder);
					return chunk.second;
				}
			}
		}

		void deallocateChunk(VkDeviceSize offset)
		{
			// check if chunk exists at the given offset
			auto chunk = m_chunkMap.find(offset);
			if (chunk != m_chunkMap.end())
			{
				// mark the chunk as not allocated
				chunk->second->m_allocated = false;
				if (chunk != m_chunkMap.begin())
				{
					auto prevChunk = chunk;
					// If the previous chunk is not allocated, merge with it
					if ((--prevChunk)->second->m_allocated == false)
					{
						auto newSize = prevChunk->second->size + chunk->second->size;
						prevChunk->second->size = newSize;
						auto next = m_chunkMap.erase(chunk);
						if (next != m_chunkMap.end())
						{
							// if the following chunk is not allocated, merge with it
							if (next->second->m_allocated == false)
							{
								newSize = prevChunk->second->size + next->second->size;
								prevChunk->second->size = newSize;
								m_chunkMap.erase(next);
							}
						}
					}
				}
			}
		}

	private:
		VkDevice m_device;
		VkDeviceMemory m_memory;
		VkDeviceSize m_size;
		VkDeviceSize m_alignment;
		std::map<VkDeviceSize, std::shared_ptr<Chunk>> m_chunkMap;

		void createChunk(VkDeviceSize offset, VkDeviceSize size)
		{
			m_chunkMap.emplace(std::make_pair(offset, std::make_shared<Chunk>()));
			auto iter = m_chunkMap.find(offset);
			m_chunkMap[offset]->size = size;
			m_chunkMap[offset]->offset = offset;
			m_chunkMap[offset]->m_memory = m_memory;
			m_chunkMap[offset]->m_block = std::make_shared<Block>(this);
		}
	};

	class Allocator
	{
	public:
		void init(VkDevice device)
		{
			m_device = device;
		}

		std::shared_ptr<Chunk> allocateMemory(uint32_t memType, VkDeviceSize size, VkDeviceSize alignment)
		{
			// create invalid chunk to return if none can be allocated
			auto chunk = std::make_shared<Chunk>();

			// if the requested allocation size is larger than the block allocation size, return invalid chunk
			if (size > BLOCK_SIZE)
				return chunk;

			// find any blocks matching the requested memory type
			auto blockRange = m_blocksByMemoryType.equal_range(memType);

			// if any are found iterate through them
			if (std::distance(blockRange.first, blockRange.second) > 0)
			{
				while (blockRange.first != blockRange.second)
				{
					// attempt to allocate a chunk
					chunk = blockRange.first->second.allocateChunk(size);
					if (chunk->size)
					{
						// return the chunk if it is successfully allocated
						return chunk;
					}
					++blockRange.first;
				}
			}
			// if no blocks are found that fit the memory type and have sufficient space to allocate the chunk
			// create a new block, then allocate from that
			auto block = m_blocksByMemoryType.emplace(std::make_pair(memType, std::make_shared<Block>(m_device, BLOCK_SIZE, memType, alignment)));
			chunk = block->second.allocateChunk(size);
			return chunk;
		}

	private:
		VkDevice m_device;
		std::multimap<uint32_t, Block> m_blocksByMemoryType;

		const uint32_t BLOCK_SIZE = 256000000U;
	};
}