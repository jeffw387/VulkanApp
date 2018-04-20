#pragma once

#undef max
#include "vulkan/vulkan.h"
#include "VulkanFunctions.hpp"
#include "Allocator.hpp"
#include "Buffer.hpp"
#include "Bitmap.hpp"
#include "entt.hpp"
#include "UniqueVulkan.hpp"
#include <limits>

namespace vka
{
	struct UniqueImage2D
	{
		VkImageUnique image;
		VkImageViewUnique view;
		UniqueAllocationHandle allocation;
		VkImageCreateInfo imageCreateInfo;
		uint64_t imageOffset;
	};

	UniqueImage2D CreateImage2D(VkDevice device,
			VkCommandBuffer commandBuffer,
			Allocator& allocator,
			const Bitmap& bitmap,
			uint32_t queueFamilyIndex,
			VkQueue graphicsQueue)
	{
		auto imageResult = UniqueImage2D();
		imageResult.imageCreateInfo = {};
		imageResult.imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageResult.imageCreateInfo.pNext = nullptr;
		imageResult.imageCreateInfo.flags = VkImageCreateFlags(0);
		imageResult.imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageResult.imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		imageResult.imageCreateInfo.extent.width = bitmap.m_Width;
		imageResult.imageCreateInfo.extent.height = bitmap.m_Height;
		imageResult.imageCreateInfo.extent.depth = 1;
		imageResult.imageCreateInfo.mipLevels = 1;
		imageResult.imageCreateInfo.arrayLayers = 1;
		imageResult.imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageResult.imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageResult.imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		imageResult.imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageResult.imageCreateInfo.queueFamilyIndexCount = 1;
		imageResult.imageCreateInfo.pQueueFamilyIndices = &queueFamilyIndex;
		imageResult.imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VkImage image;
		auto result = vkCreateImage(device, &imageResult.imageCreateInfo, nullptr, &image);

		VkImageDeleter imageDeleter;
		imageDeleter.device = device;
		imageResult.image = VkImageUnique(image, imageDeleter);
		imageResult.allocation = allocator.AllocateForImage(true, image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		vkBindImageMemory(device, image,
			imageResult.allocation.get().memory,
			imageResult.allocation.get().offsetInDeviceMemory);
		
		auto stagingBufferResult = CreateBuffer(device, allocator, imageResult.allocation.get().size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			queueFamilyIndex,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			true);

		// copy from host to staging buffer
		void* stagingBufferData = nullptr;
		vkMapMemory(device, 
			stagingBufferResult.allocation.get().memory,
			stagingBufferResult.allocation.get().offsetInDeviceMemory,
			stagingBufferResult.allocation.get().size,
			VkMemoryMapFlags(0),
			&stagingBufferData);

		memcpy(stagingBufferData, bitmap.m_Data.data(), bitmap.m_Size);
		vkUnmapMemory(device, 
			stagingBufferResult.allocation.get().memory);

		auto cmdBufferBeginInfo = VkCommandBufferBeginInfo();
		cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufferBeginInfo.pNext = nullptr;
		cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		cmdBufferBeginInfo.pInheritanceInfo = nullptr;

		vkBeginCommandBuffer(commandBuffer, &cmdBufferBeginInfo);

		// transition image layout for transfer
		VkImageMemoryBarrier imageMemoryBarrier = {};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.pNext = nullptr;
		imageMemoryBarrier.srcAccessMask = VkAccessFlags(0);
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.srcQueueFamilyIndex = queueFamilyIndex;
		imageMemoryBarrier.dstQueueFamilyIndex = queueFamilyIndex;
		imageMemoryBarrier.image = image;
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
		imageMemoryBarrier.subresourceRange.levelCount = 1;
		imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
		imageMemoryBarrier.subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(commandBuffer, 
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VkDependencyFlags(0),
			0,
			nullptr,
			0,
			nullptr,
			1,
			&imageMemoryBarrier);

		// copy from host to device (to image)
		VkBufferImageCopy bufferImageCopy = {};
		bufferImageCopy.bufferOffset = 0;
		bufferImageCopy.bufferRowLength = 0;
		bufferImageCopy.bufferImageHeight = 0;
		bufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferImageCopy.imageSubresource.mipLevel = 0;
		bufferImageCopy.imageSubresource.baseArrayLayer = 0;
		bufferImageCopy.imageSubresource.layerCount = 1;
		bufferImageCopy.imageOffset = {};
		bufferImageCopy.imageExtent.width = bitmap.m_Width;
		bufferImageCopy.imageExtent.height = bitmap.m_Height;
		bufferImageCopy.imageExtent.depth = 1;

		vkCmdCopyBufferToImage(commandBuffer, 
			stagingBufferResult.buffer.get(),
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&bufferImageCopy);

		// transition image for shader access
		VkImageMemoryBarrier imageMemoryBarrier2 = {};
		imageMemoryBarrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier2.pNext = nullptr;
		imageMemoryBarrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		imageMemoryBarrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier2.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageMemoryBarrier2.srcQueueFamilyIndex = queueFamilyIndex;
		imageMemoryBarrier2.dstQueueFamilyIndex = queueFamilyIndex;
		imageMemoryBarrier2.image = image;
		imageMemoryBarrier2.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageMemoryBarrier2.subresourceRange.baseMipLevel = 0;
		imageMemoryBarrier2.subresourceRange.levelCount = 1;
		imageMemoryBarrier2.subresourceRange.baseArrayLayer = 0;
		imageMemoryBarrier2.subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(commandBuffer, 
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VkDependencyFlags(0),
			0,
			nullptr,
			0,
			nullptr,
			1,
			&imageMemoryBarrier2);

		vkEndCommandBuffer(commandBuffer);

		// create fence, submit command buffer
		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.pNext = nullptr;
		fenceCreateInfo.flags = VkFenceCreateFlags(0);

		VkFence imageLoadFence;
		vkCreateFence(device, &fenceCreateInfo, nullptr, &imageLoadFence);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, imageLoadFence);

		// create image view
		VkImageView imageView;

		VkImageViewCreateInfo viewCreateInfo = {};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.pNext = nullptr;
		viewCreateInfo.flags = (VkImageViewCreateFlags)0;
		viewCreateInfo.image = image;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewCreateInfo.subresourceRange.baseMipLevel = 0;
		viewCreateInfo.subresourceRange.levelCount = 1;
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;
		viewCreateInfo.subresourceRange.layerCount = 1;

		vkCreateImageView(device, &viewCreateInfo, nullptr, &imageView);
		VkImageViewDeleter viewDeleter;
		viewDeleter.device = device;
		imageResult.view = VkImageViewUnique(imageView, viewDeleter);

		vkWaitForFences(device, 1, &imageLoadFence, (VkBool32)true, 
			std::numeric_limits<uint64_t>::max());

		return std::move(imageResult);
	}
}