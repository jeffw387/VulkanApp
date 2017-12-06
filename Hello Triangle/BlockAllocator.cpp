#include "BlockAllocator.h"

inline vke::Chunk::operator VkDeviceMemory()
{
	return m_Memory;
}

inline void vke::Chunk::map(VkDevice device)
{
	if (isMapped)
	{
		return;
	}
	vkMapMemory(device, m_Memory, offset, size, 0, &mappedMemory);
	isMapped = true;
}

inline void vke::Chunk::unmap(VkDevice device)
{
	if (isMapped)
	{
		vkUnmapMemory(device, m_Memory);
		isMapped = false;
	}
}

inline vke::Block::Block(VkDevice device, VkDeviceSize size, uint32_t memoryType, VkDeviceSize minimumAlignment)
{
	m_Device = device;
	VkMemoryAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	info.allocationSize = size;
	info.memoryTypeIndex = memoryType;
	if (vkAllocateMemory(device, &info, nullptr, &m_Memory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate memory block");
	}
	m_Size = size;
	m_Alignment = minimumAlignment;
	auto initialOffset = 0U;
	createChunk(initialOffset, size);
}

inline auto vke::Block::allocateChunk(VkDeviceSize size)
{
	// round allocation size up to nearest multiple of minimum alignment
	auto allocSize = helper::roundUp(size, m_Alignment);
	for (auto& chunk : m_ChunkMap)
	{
		if (chunk.second.size > allocSize)
		{
			// return entire chunk if the size matches exactly
			if (chunk.second.size == allocSize)
			{
				chunk.second.m_Allocated = true;
				return chunk.second;
			}
			// otherwise subdivide the chunk before returning it
			auto remainder = chunk.second.size - allocSize;
			chunk.second.size = allocSize;
			createChunk(chunk.first + allocSize, remainder);
			return chunk.second;
		}
	}
	return Chunk();
}

inline void vke::Block::deallocateChunk(Chunk& chunk)
{
	// check if chunk exists at the given offset
	auto foundChunk = m_ChunkMap.find(chunk.offset);
	if (foundChunk != m_ChunkMap.end())
	{
		// mark the chunk as not allocated
		foundChunk->second.m_Allocated = false;
		if (foundChunk != m_ChunkMap.begin())
		{
			auto prevChunk = foundChunk;
			// If the previous chunk is not allocated, merge with it
			if ((--prevChunk)->second->m_Allocated == false)
			{
				auto newSize = prevChunk->second->size + foundChunk->second->size;
				prevChunk->second->size = newSize;
				auto next = m_ChunkMap.erase(foundChunk);
				if (next != m_ChunkMap.end())
				{
					// if the following chunk is not allocated, merge with it
					if (next->second->m_Allocated == false)
					{
						newSize = prevChunk->second->size + next->second->size;
						prevChunk->second->size = newSize;
						m_ChunkMap.erase(next);
					}
				}
			}
		}
	}
}

inline void vke::Block::createChunk(VkDeviceSize offset, VkDeviceSize size)
{
	m_ChunkMap.emplace(std::make_pair(offset, std::make_unique<Chunk>()));
	auto iter = m_ChunkMap.find(offset);
	m_ChunkMap[offset]->size = size;
	m_ChunkMap[offset]->offset = offset;
	m_ChunkMap[offset]->m_Memory = m_Memory;
	m_ChunkMap[offset]->m_Block = std::make_unique<Block>(*this);
}

inline auto vke::Allocator::allocateMemory(uint32_t memType, VkDeviceSize size, VkDeviceSize alignment)
{
	// create invalid chunk to return if none can be allocated
	auto chunk = std::make_unique<Chunk>();

	// if the requested allocation size is larger than the block allocation size, return invalid chunk
	if (size > BLOCK_SIZE)
		return std::move(chunk);

	// find any blocks matching the requested memory type
	auto blockRange = m_blocksByMemoryType.equal_range(memType);

	// if any are found iterate through them
	if (std::distance(blockRange.first, blockRange.second) > 0)
	{
		while (blockRange.first != blockRange.second)
		{
			// attempt to allocate a chunk
			chunk = blockRange.first->second->allocateChunk(size);
			if (chunk->size)
			{
				// return the chunk if it is successfully allocated
				return std::move(chunk);
			}
			++blockRange.first;
		}
	}
	// if no blocks are found that fit the memory type and have sufficient space to allocate the chunk
	// create a new block, then allocate from that
	auto blockPair = m_blocksByMemoryType.emplace(std::make_pair(memType, std::make_unique<Block>(m_device, BLOCK_SIZE, memType, alignment)));
	chunk = std::move(blockPair->second->allocateChunk(size));
	return std::move(chunk);
}
