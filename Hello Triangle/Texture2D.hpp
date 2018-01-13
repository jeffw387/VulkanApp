#pragma once
#include <vulkan/vulkan.h>
using TextureIndex = uint32_t;
struct Texture2D
{
	TextureIndex index;
	VkImage imageHandle;
	uint32_t width;
	uint32_t height;
	VkDeviceSize size;
};