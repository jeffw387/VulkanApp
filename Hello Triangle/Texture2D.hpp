#pragma once
#include <vulkan/vulkan.h>
using TextureIndex = uint32_t;
struct Texture2D
{
	TextureIndex index;
	uint32_t width;
	uint32_t height;
	VkDeviceSize size;
};