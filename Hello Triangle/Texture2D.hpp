#pragma once
#include <vulkan/vulkan.h>
struct Texture2D
{
	uint32_t index;
	VkImage imageHandle;
	uint32_t width;
	uint32_t height;
	VkDeviceSize size;
};