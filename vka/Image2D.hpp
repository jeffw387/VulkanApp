#pragma once

#undef max
#include "vulkan/vulkan.hpp"
#include "Allocator.hpp"
#include "Buffer.hpp"
#include "Bitmap.hpp"
#include "entt.hpp"

namespace vka
{
	struct Image2D
	{
		vk::UniqueImage image;
		vk::UniqueImageView view;
		UniqueAllocation allocation;
		vk::ImageCreateInfo imageCreateInfo;
		uint64_t imageOffset;
	};

	Image2D&& CreateImage2D(vk::Device device,
			vk::CommandBuffer commandBuffer,
			Allocator& allocator,
			const Bitmap& bitmap,
			uint32_t queueFamilyIndex,
			vk::Queue graphicsQueue)
	{
		auto result = Image2D();
		result.imageCreateInfo = vk::ImageCreateInfo(
			vk::ImageCreateFlags(),
			vk::ImageType::e2D,
			vk::Format::eR8G8B8A8Srgb,
			vk::Extent3D(bitmap.m_Width, bitmap.m_Height, 1U),
			1U,
			1U,
			vk::SampleCountFlagBits::e1,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
			vk::SharingMode::eExclusive,
			1U,
			&queueFamilyIndex,
			vk::ImageLayout::eUndefined);

		result.image = device.createImageUnique(result.imageCreateInfo);
		result.allocation = allocator.AllocateForImage(true, result.image.get(), 
			vk::MemoryPropertyFlagBits::eDeviceLocal);
		device.bindImageMemory(
			result.image.get(),
			result.allocation->memory,
			result.allocation->offsetInDeviceMemory);
		
		auto stagingBufferResult = CreateBuffer(device, allocator, result.allocation->size,
			vk::BufferUsageFlagBits::eTransferSrc,
			queueFamilyIndex,
			vk::MemoryPropertyFlagBits::eHostCoherent |
			vk::MemoryPropertyFlagBits::eHostVisible,
			true);
			
		// copy from host to staging buffer
		void* stagingBufferData = device.mapMemory(
			stagingBufferResult.allocation->memory,
			stagingBufferResult.allocation->offsetInDeviceMemory,
			stagingBufferResult.allocation->size);
		memcpy(stagingBufferData, bitmap.m_Data.data(), bitmap.m_Size);
		device.unmapMemory(stagingBufferResult.allocation->memory);

		// begin recording command buffer
		commandBuffer.begin(vk::CommandBufferBeginInfo(
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

		// transition image layout
		auto imageMemoryBarrier = vk::ImageMemoryBarrier(
			vk::AccessFlags(),
			vk::AccessFlagBits::eTransferWrite,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eTransferDstOptimal,
			queueFamilyIndex,
			queueFamilyIndex,
			result.image.get(),
			vk::ImageSubresourceRange(
				vk::ImageAspectFlagBits::eColor,
				0U,
				1U,
				0U,
				1U));
		commandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTopOfPipe,
			vk::PipelineStageFlagBits::eTransfer,
			vk::DependencyFlags(),
			{ },
			{ },
			{ imageMemoryBarrier });

		// copy from host to device (to image)
		commandBuffer.copyBufferToImage(
			stagingBufferResult.buffer.get(),
			result.image.get(),
			vk::ImageLayout::eTransferDstOptimal,
			{
				vk::BufferImageCopy(0U, 0U, 0U,
					vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0U, 0U, 1U),
					vk::Offset3D(),
					vk::Extent3D(bitmap.m_Width, bitmap.m_Height, 1U))
			});

		// transition image for shader access
		auto imageMemoryBarrier2 = vk::ImageMemoryBarrier(
			vk::AccessFlagBits::eTransferWrite,
			vk::AccessFlagBits::eShaderRead,
			vk::ImageLayout::eTransferDstOptimal,
			vk::ImageLayout::eShaderReadOnlyOptimal,
			queueFamilyIndex,
			queueFamilyIndex,
			result.image.get(),
			vk::ImageSubresourceRange(
				vk::ImageAspectFlagBits::eColor,
				0U,
				1U,
				0U,
				1U));
		commandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eFragmentShader,
			vk::DependencyFlags(),
			{ },
			{ },
			{ imageMemoryBarrier2 });

		commandBuffer.end();

		// create fence, submit command buffer
		auto imageLoadFence = device.createFenceUnique(vk::FenceCreateInfo());
		graphicsQueue.submit(
			vk::SubmitInfo(0U, nullptr, nullptr,
				1U, &commandBuffer,
				0U, nullptr),
			imageLoadFence.get());

		// create image view
		result.view = device.createImageViewUnique(
			vk::ImageViewCreateInfo(
				vk::ImageViewCreateFlags(),
				result.image.get(),
				vk::ImageViewType::e2D,
				vk::Format::eR8G8B8A8Srgb,
				vk::ComponentMapping(),
				vk::ImageSubresourceRange(
					vk::ImageAspectFlagBits::eColor,
					0U,
					1U,
					0U,
					1U)));
				
		// wait for command buffer to be executed
		device.waitForFences({ imageLoadFence.get() }, true, std::numeric_limits<uint64_t>::max());
	}
}