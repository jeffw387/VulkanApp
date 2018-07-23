#pragma once

#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"
#include "gsl.hpp"
#include "vka/Image2D.hpp"
#include "vka/Buffer.hpp"

#include <vector>

namespace vka
{
	inline auto GetPhysicalDevices(VkInstance instance)
	{
		std::vector<VkPhysicalDevice> physicalDevices;
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		physicalDevices.resize(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());
		return physicalDevices;
	}

	inline auto GetQueueFamilyProperties(VkPhysicalDevice physicalDevice)
	{
		std::vector<VkQueueFamilyProperties> queueFamilyProperties;
		uint32_t count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
		queueFamilyProperties.resize(count);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, queueFamilyProperties.data());
		return queueFamilyProperties;
	}

	inline auto GetMemoryProperties(VkPhysicalDevice physicalDevice)
	{
		VkPhysicalDeviceMemoryProperties properties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &properties);
		return properties;
	}

	inline auto GetProperties(VkPhysicalDevice physicalDevice)
	{
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);
		return properties;
	}

	inline auto GetSurfaceCapabilities(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
	{
		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);
		return capabilities;
	}

	inline auto GetSurfaceFormats(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
	{
		std::vector<VkSurfaceFormatKHR> surfaceFormats;
		uint32_t count = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, nullptr);
		surfaceFormats.resize(count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, surfaceFormats.data());
		return surfaceFormats;
	}

	inline auto GetPresentModes(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
	{
		std::vector<VkPresentModeKHR> supported;
		uint32_t count = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, nullptr);
		supported.resize(count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, supported.data());
		return supported;
	}

	inline auto CreateAllocatedBuffer(
		VkDevice device,
		Allocator &allocator,
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		uint32_t queueFamilyIndex,
		VkMemoryPropertyFlags memoryFlags,
		bool DedicatedAllocation)
	{
		auto bufferCreateInfo = vka::bufferCreateInfo(usage, size);
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufferCreateInfo.queueFamilyIndexCount = 1;
		bufferCreateInfo.pQueueFamilyIndices = &queueFamilyIndex;

		Buffer allocatedBuffer{};
		vkCreateBuffer(device,
			&bufferCreateInfo,
			nullptr,
			&allocatedBuffer.buffer);

		allocatedBuffer.usage = usage;
		allocatedBuffer.size = size;
		allocatedBuffer.allocation = allocator.AllocateForBuffer(DedicatedAllocation, allocatedBuffer.buffer, memoryFlags);

		vkBindBufferMemory(device, allocatedBuffer.buffer,
			allocatedBuffer.allocation.memory,
			allocatedBuffer.allocation.offsetInDeviceMemory);

		return allocatedBuffer;
	}

	inline auto DestroyAllocatedBuffer(
		VkDevice device,
		Buffer buffer)
	{
		buffer.allocation.deallocate();
		if (buffer.buffer != VK_NULL_HANDLE)
			vkDestroyBuffer(device, buffer.buffer, nullptr);
	}

	inline auto MapBuffer(
		VkDevice device,
		Buffer& buffer)
	{
		auto& alloc = buffer.allocation;
		vkMapMemory(device,
			alloc.memory,
			alloc.offsetInDeviceMemory,
			alloc.size,
			0,
			&buffer.mapPtr);
	}

	inline auto CopyBufferToBuffer(
		const VkCommandBuffer commandBuffer,
		const VkQueue queue,
		const VkBuffer source,
		const VkBuffer destination,
		const VkBufferCopy &bufferCopy,
		const VkFence fence,
		const gsl::span<VkSemaphore> signalSemaphores)
	{
		auto cmdBufferBeginInfo = VkCommandBufferBeginInfo();
		cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufferBeginInfo.pNext = nullptr;
		cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		cmdBufferBeginInfo.pInheritanceInfo = nullptr;

		vkBeginCommandBuffer(commandBuffer, &cmdBufferBeginInfo);
		vkCmdCopyBuffer(commandBuffer, source, destination, 1, &bufferCopy);
		vkEndCommandBuffer(commandBuffer);

		auto submitInfo = vka::submitInfo();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		submitInfo.signalSemaphoreCount = gsl::narrow<uint32_t>(signalSemaphores.size());
		submitInfo.pSignalSemaphores = signalSemaphores.data();

		vkQueueSubmit(queue, 1U, &submitInfo, fence);
	}

	inline auto CreateImage2D(
		VkDevice device,
		VkImageUsageFlags usage,
		VkFormat format,
		uint32_t width,
		uint32_t height,
		uint32_t queueFamilyIndex,
		VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		VkImageCreateFlags flags = 0)
	{
		auto createInfo = vka::imageCreateInfo();
		createInfo.imageType = VK_IMAGE_TYPE_2D;
		createInfo.format = format;
		createInfo.extent = { width, height, 1 };
		createInfo.mipLevels = 1;
		createInfo.arrayLayers = 1;
		createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		createInfo.usage = usage;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 1;
		createInfo.pQueueFamilyIndices = &queueFamilyIndex;
		createInfo.initialLayout = initialLayout;

		VkImage image;
		vkCreateImage(device, &createInfo, nullptr, &image);
		return image;
	}

	inline auto CreateImageView2D(
		VkDevice device,
		VkImage image,
		VkFormat format,
		VkImageAspectFlags aspects)
	{
		auto createInfo = vka::imageViewCreateInfo();
		createInfo.image = image;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = format;
		createInfo.components = {
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY
		};
		createInfo.subresourceRange = {
			aspects,
			0,
			1,
			0,
			1
		};

		VkImageView view;
		vkCreateImageView(
			device,
			&createInfo,
			nullptr,
			&view);
		return view;
	}

	inline auto AllocateImage2D(
		VkDevice device,
		Allocator& allocator,
		VkImage image)
	{
		auto handle = allocator.AllocateForImage(
			true,
			image,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		vkBindImageMemory(
			device,
			image,
			handle.memory,
			handle.offsetInDeviceMemory);
		return handle;
	}

	inline auto CreateAllocatedImage2D(
		VkDevice device,
		Allocator& allocator,
		VkImageUsageFlags usage,
		VkFormat format,
		uint32_t width,
		uint32_t height,
		uint32_t graphicsQueueFamilyIndex,
		VkImageAspectFlags aspects,
		VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		VkImageCreateFlags flags = 0)
	{
		vka::Image2D image2D{};
		image2D.image = CreateImage2D(
			device, 
			usage,
			format,
			width,
			height,
			graphicsQueueFamilyIndex,
			initialLayout,
			flags);

		image2D.allocation = AllocateImage2D(
			device,
			allocator,
			image2D.image);

		image2D.view = CreateImageView2D(
			device,
			image2D.image,
			format,
			aspects);
		image2D.currentLayout = initialLayout;
		image2D.format = format;
		image2D.height = height;
		image2D.width = width;
		image2D.usage = usage;
		image2D.aspects = aspects;
		return image2D;
	}

	inline auto DestroyAllocatedImage2D(
		VkDevice device,
		Image2D image2D)
	{
		image2D.allocation.deallocate();
		vkDestroyImageView(device, image2D.view, nullptr);
		vkDestroyImage(device, image2D.image, nullptr);
	}

	inline auto RecordImageTransition(
		VkCommandBuffer cmdBuffer,
		Image2D& imageStruct,
		VkImageLayout newLayout)
	{
		auto barrier = vka::imageMemoryBarrier();
		barrier.image = imageStruct.image;
		barrier.oldLayout = imageStruct.currentLayout;
		barrier.newLayout = newLayout;

		// Source layouts (old)
			// Source access mask controls actions that have to be finished on the old layout
			// before it will be transitioned to the new layout
		switch (barrier.oldLayout)
		{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			// Image layout is undefined (or does not matter)
			// Only valid as initial layout
			// No flags required, listed only for completeness
			barrier.srcAccessMask = 0;
			break;

		case VK_IMAGE_LAYOUT_PREINITIALIZED:
			// Image is preinitialized
			// Only valid as initial layout for linear images, preserves memory contents
			// Make sure host writes have been finished
			barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image is a color attachment
			// Make sure any writes to the color buffer have been finished
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image is a depth/stencil attachment
			// Make sure any writes to the depth/stencil buffer have been finished
			barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image is a transfer source 
			// Make sure any reads from the image have been finished
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image is a transfer destination
			// Make sure any writes to the image have been finished
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image is read by a shader
			// Make sure any shader reads from the image have been finished
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			// Other source layouts aren't handled (yet)
			break;
		}

		// Target layouts (new)
		// Destination access mask controls the dependency for the new image layout
		switch (newLayout)
		{
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image will be used as a transfer destination
			// Make sure any writes to the image have been finished
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image will be used as a transfer source
			// Make sure any reads from the image have been finished
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image will be used as a color attachment
			// Make sure any writes to the color buffer have been finished
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image layout will be used as a depth/stencil attachment
			// Make sure any writes to depth/stencil buffer have been finished
			barrier.dstAccessMask = barrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image will be read in a shader (sampler, input attachment)
			// Make sure any writes to the image have been finished
			if (barrier.srcAccessMask == 0)
			{
				barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
			}
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			// Other source layouts aren't handled (yet)
			break;
		}

		barrier.subresourceRange = {
			imageStruct.aspects,
			0,
			1,
			0,
			1
		};

		vkCmdPipelineBarrier(
			cmdBuffer,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier);
	}

	inline auto CopyBitmapToImage2D(
		Image2D& imageStruct,
		const Bitmap& bitmap,
		VkDevice device,
		Allocator& allocator,
		VkCommandBuffer cmdBuffer,
		VkQueue queue,
		uint32_t queueFamilyIndex,
		VkFence fence,
		VkImageLayout finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		auto data = gsl::span(bitmap.data);

		auto stagingBuffer = CreateAllocatedBuffer(
			device,
			allocator,
			data.size_bytes(),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			queueFamilyIndex,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			true);

		MapBuffer(device, stagingBuffer);

		memcpy(stagingBuffer.mapPtr, data.data(), data.size_bytes());

		auto beginInfo = vka::commandBufferBeginInfo();
		vkBeginCommandBuffer(cmdBuffer, &beginInfo);

		RecordImageTransition(
			cmdBuffer,
			imageStruct,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		auto copyRegion = VkBufferImageCopy{};
		copyRegion.bufferRowLength = imageStruct.width;
		copyRegion.bufferImageHeight = imageStruct.height;
		copyRegion.imageExtent = { imageStruct.width, imageStruct.height, 1 };
		copyRegion.imageOffset = { 0, 0, 0 };
		copyRegion.imageSubresource = {
			imageStruct.aspects,
			0,
			0,
			1
		};

		vkCmdCopyBufferToImage(
			cmdBuffer,
			stagingBuffer.buffer,
			imageStruct.image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&copyRegion);

		RecordImageTransition(cmdBuffer, imageStruct, finalLayout);

		vkEndCommandBuffer(cmdBuffer);

		auto copySubmit = vka::submitInfo();
		copySubmit.commandBufferCount = 1;
		copySubmit.pCommandBuffers = &cmdBuffer;
		vkQueueSubmit(queue, 1, &copySubmit, fence);

		constexpr auto maxU64 = ~0Ui64;
		vkWaitForFences(device, 1, &fence, 1, maxU64);
		vkResetFences(device, 1, &fence);

		DestroyAllocatedBuffer(device, stagingBuffer);
	}

	inline auto GetSwapImages(VkDevice device, VkSwapchainKHR swapchain)
	{
		std::vector<VkImage> swapImages;
		uint32_t count = 0;
		vkGetSwapchainImagesKHR(device, swapchain, &count, nullptr);
		swapImages.resize(count);
		vkGetSwapchainImagesKHR(device, swapchain, &count, swapImages.data());
		return swapImages;
	}

	inline auto AllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo& allocateInfo)
	{
		std::vector<VkDescriptorSet> sets;
		sets.resize(allocateInfo.descriptorSetCount);
		vkAllocateDescriptorSets(device, &allocateInfo, sets.data());
		return sets;
	}

	inline auto AllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo& allocateInfo)
	{
		std::vector<VkCommandBuffer> commandBuffers;
		commandBuffers.resize(allocateInfo.commandBufferCount);
		vkAllocateCommandBuffers(device, &allocateInfo, commandBuffers.data());
		return commandBuffers;
	}

	inline auto CreateGraphicsPipelines(VkDevice device, gsl::span<VkGraphicsPipelineCreateInfo> createInfos)
	{
		std::vector<VkPipeline> pipelines;
		pipelines.resize(createInfos.size());
		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, gsl::narrow_cast<uint32_t>(createInfos.size()), createInfos.data(), nullptr, pipelines.data());
		return pipelines;
	}
}