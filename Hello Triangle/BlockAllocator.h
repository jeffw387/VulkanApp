#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <map>
#include "math.h"

namespace vke
{
	class Chunk
	{
		friend class Allocator;
		friend class Block;
	public:
		Chunk() : m_Mapped(false), m_Offset(0), m_Size(0), m_MappedMemory(nullptr), m_Allocated(false)
		{
		}

		Chunk(Chunk& chunkToCopy)
		{
			m_Mapped = chunkToCopy.m_Mapped;
			m_Offset = chunkToCopy.m_Offset;
			m_Size = chunkToCopy.m_Size;
			m_MappedMemory = chunkToCopy.m_MappedMemory;
			m_Allocated = chunkToCopy.m_Allocated;
		}
		
		~Chunk()
		{
		}

		bool isMapped() { return m_Mapped; }
		VkDeviceSize offset() { return m_Offset; }
		VkDeviceSize size() { return m_Size; }
		void* mappedMemory() { return m_MappedMemory; }

		operator VkDeviceMemory();

		void map(VkDevice device);

		void unmap(VkDevice device);

		bool isAllocated() { return m_Allocated; }

	private:
		void* m_MappedMemory;
		VkDeviceSize m_Size;
		VkDeviceSize m_Offset;
		bool m_Allocated;
		bool m_Mapped;
	};

	class Block
	{
		friend class Allocator;
	public:
		Block() : m_Device(VK_NULL_HANDLE), m_Memory(VK_NULL_HANDLE) {}

		Block(VkDevice device, VkDeviceSize size, uint32_t memoryType, VkDeviceSize minimumAlignment);

		~Block()
		{
			if (m_Device && m_Memory)
				vkFreeMemory(m_Device, m_Memory, nullptr);
		}

		VkDeviceMemory getMemory()
		{
			return m_Memory;
		}

		auto allocateChunk(VkDeviceSize size);

		void deallocateChunk(Chunk& chunk);

	private:
		VkDevice m_Device;
		VkDeviceMemory m_Memory;
		VkDeviceSize m_Size;
		VkDeviceSize m_Alignment;
		std::map<VkDeviceSize, Chunk> m_ChunkMap;

		void createChunk(VkDeviceSize offset, VkDeviceSize size);
	};

	class Allocator
	{
	public:
		void init(VkDevice device)
		{
			m_device = device;
		}

		auto allocateMemory(uint32_t memType, VkDeviceSize size, VkDeviceSize alignment);

	private:
		VkDevice m_device;
		std::multimap<uint32_t, std::unique_ptr<Block>> m_blocksByMemoryType;

		const uint32_t BLOCK_SIZE = 256000000U;
	};
}