#pragma once
#include <vulkan/vulkan.h>
#include "BlockAllocator.h"

namespace vke
{
	class Device;

	struct Buffer
	{
		Buffer() : device(), buffer(VK_NULL_HANDLE), memoryType(0), size(0), alignment(0), memory(nullptr)
		{
		}

		~Buffer()
		{
			memory->deallocate();
			vkDestroyBuffer(device, *this, nullptr);
		}

		operator VkBuffer()
		{
			return buffer;
		}

		void map()
		{
			memory->map(device);
		}

		void unmap()
		{
			memory->unmap(device);
		}

		Device device;
		VkBuffer buffer;
		uint32_t memoryType;
		VkDeviceSize size;
		VkDeviceSize alignment;
		std::shared_ptr<Chunk> memory;
	};
}