#pragma once

#include "vulkan/vulkan.hpp"
#include "Allocator.hpp"
#include "InitState.hpp"
#include "Buffer.hpp"
#include "Camera.hpp"
#include "glm/glm.hpp"
#include "Sprite.hpp"
#include "Vertex.hpp"
#include "DeviceState.hpp"
#include "SurfaceState.hpp"
#include "Image2D.hpp"
#include "fileIO.hpp"
#include "entt.hpp"
#include <array>
#include <cstring>
#include <vector>
#include <string>
#include <map>


namespace vka
{
    static constexpr size_t BufferCount = 3U;

    struct Supports
    {
        vk::UniqueCommandPool renderCommandPool;
        vk::UniqueCommandBuffer renderCommandBuffer;
        vk::UniqueFence renderBufferExecuted;
        vk::UniqueFence imagePresentCompleteFence;
        vk::UniqueSemaphore imageRenderCompleteSemaphore;
    };

    struct RenderState
    {
        UniqueAllocatedBuffer vertexBuffer;
        vk::UniqueRenderPass renderPass;
		vk::UniqueSampler sampler;
        std::array<Supports, BufferCount> supports;
		vk::UniqueSwapchainKHR swapchain;
		std::vector<vk::Image> swapImages;
		std::array<vk::UniqueImageView, BufferCount> swapViews;
		std::array<vk::UniqueFramebuffer, BufferCount> framebuffers;
        Camera2D camera;
        uint32_t nextImage;
        std::map<uint64_t, UniqueImage2D> images;
		std::map<uint64_t, Sprite> sprites;
        std::vector<Quad> quads;
    };

    struct FragmentPushConstants
	{
		glm::uint32 imageOffset;
        glm::vec4 color;
        glm::mat4 mvp;
	};

    struct ShaderState
    {
        vk::UniqueShaderModule vertexShader;
		vk::UniqueShaderModule fragmentShader;
		std::vector<vk::PushConstantRange> pushConstantRanges;
		vk::UniqueDescriptorSetLayout fragmentDescriptorSetLayout;
		vk::UniqueDescriptorPool fragmentLayoutDescriptorPool;
		vk::UniqueDescriptorSet fragmentDescriptorSet;
    };

    static void CreateShaderModules(ShaderState& shaderState, const DeviceState& deviceState, const InitState& initState)
	{
		// read from file
		auto vertexShaderBinary = fileIO::readFile(initState.vertexShaderPath);
		// create shader module
		auto vertexShaderInfo = vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(),
			vertexShaderBinary.size(),
			reinterpret_cast<const uint32_t*>(vertexShaderBinary.data()));
		shaderState.vertexShader = deviceState.logicalDevice->createShaderModuleUnique(vertexShaderInfo);

		// read from file
		auto fragmentShaderBinary = fileIO::readFile(initState.fragmentShaderPath);
		// create shader module
		auto fragmentShaderInfo = vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(),
			fragmentShaderBinary.size(),
			reinterpret_cast<const uint32_t*>(fragmentShaderBinary.data()));
		shaderState.fragmentShader = deviceState.logicalDevice->createShaderModuleUnique(fragmentShaderInfo);
	}

	static void CreateFragmentDescriptorPool(ShaderState& shaderState, 
		const DeviceState& deviceState, 
		const RenderState& renderState)
	{
		auto textureCount = renderState.images.size();
		auto poolSizes = std::vector<vk::DescriptorPoolSize>
		{
			vk::DescriptorPoolSize(vk::DescriptorType::eSampler, 1U),
			vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, textureCount)
		};

		shaderState.fragmentLayoutDescriptorPool = deviceState.logicalDevice->createDescriptorPoolUnique(
			vk::DescriptorPoolCreateInfo(
				vk::DescriptorPoolCreateFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet),
				1U,
				static_cast<uint32_t>(poolSizes.size()),
				poolSizes.data()));
	}

	static void CreateFragmentSetLayout(ShaderState& shaderState, 
		const DeviceState& deviceState, 
		const RenderState& renderState)
	{
		auto textureCount = renderState.images.size();
		auto fragmentLayoutBindings = std::vector<vk::DescriptorSetLayoutBinding>
		{
			// Fragment: sampler uniform buffer
			vk::DescriptorSetLayoutBinding(
				0U,
				vk::DescriptorType::eSampler,
				1U,
				vk::ShaderStageFlagBits::eFragment,
				&renderState.sampler.get()),
			// Fragment: image2D uniform buffer (array)
			vk::DescriptorSetLayoutBinding(
				1U,
				vk::DescriptorType::eSampledImage,
				textureCount,
				vk::ShaderStageFlagBits::eFragment)
		};

		shaderState.fragmentDescriptorSetLayout = deviceState.logicalDevice->createDescriptorSetLayoutUnique(
			vk::DescriptorSetLayoutCreateInfo(
				vk::DescriptorSetLayoutCreateFlags(),
				static_cast<uint32_t>(fragmentLayoutBindings.size()),
				fragmentLayoutBindings.data()));
	}

	static std::vector<vk::DescriptorSetLayout> GetSetLayouts(const ShaderState& shaderState)
	{
		return std::vector<vk::DescriptorSetLayout>
		{ 
			shaderState.fragmentDescriptorSetLayout.get()
		};
	}

	static void SetupPushConstants(ShaderState& shaderState)
	{
		shaderState.pushConstantRanges.emplace_back(vk::PushConstantRange(
			vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
			0U,
			sizeof(FragmentPushConstants)));
	}

	static void CreateAndUpdateFragmentDescriptorSet(ShaderState& shaderState, 
		const DeviceState& deviceState, 
		const RenderState& renderState)
	{
		auto textureCount = renderState.images.size();
		// create fragment descriptor set
		shaderState.fragmentDescriptorSet = std::move(
			deviceState.logicalDevice->allocateDescriptorSetsUnique(
				vk::DescriptorSetAllocateInfo(
					shaderState.fragmentLayoutDescriptorPool.get(),
					1U,
					&shaderState.fragmentDescriptorSetLayout.get()))[0]);

		// update fragment descriptor set
			// binding 0 (sampler)
		auto descriptorImageInfo = vk::DescriptorImageInfo(renderState.sampler.get());
		deviceState.logicalDevice->updateDescriptorSets(
			{
				vk::WriteDescriptorSet(
					shaderState.fragmentDescriptorSet.get(),
					0U,
					0U,
					1U,
					vk::DescriptorType::eSampler,
					&descriptorImageInfo)
			},
			{}
		);
			// binding 1 (images)
		
		auto imageInfos = std::vector<vk::DescriptorImageInfo>();
		imageInfos.reserve(textureCount);
		for (auto& imagePair : renderState.images)
		{
			imageInfos.emplace_back(vk::DescriptorImageInfo(
				vk::Sampler(),
				imagePair.second.view.get(),
				vk::ImageLayout::eShaderReadOnlyOptimal));
		}

		deviceState.logicalDevice->updateDescriptorSets(
			{
				vk::WriteDescriptorSet(
					shaderState.fragmentDescriptorSet.get(),
					1U,
					0U,
					static_cast<uint32_t>(imageInfos.size()),
					vk::DescriptorType::eSampledImage,
					imageInfos.data())
			},
			{}
		);
	}

    static void CreateVertexBuffer(
        RenderState& renderState, 
        DeviceState& deviceState, 
        const InitState& initState)
    {
        auto& quads = renderState.quads;
        if (quads.size() == 0)
        {
            std::runtime_error("Error: no vertices loaded.");
        }
        // create vertex buffers
        constexpr auto quadSize = sizeof(Quad);
        size_t vertexBufferSize = quadSize * quads.size();
        auto vertexStagingBuffer = CreateBuffer(
            deviceState.logicalDevice.get(),
            deviceState.allocator,
            vertexBufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            deviceState.graphicsQueueFamilyID,
            vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent,
            true);
        
        renderState.vertexBuffer = CreateBuffer(
            deviceState.logicalDevice.get(),
            deviceState.allocator,
            vertexBufferSize, 
            vk::BufferUsageFlagBits::eTransferDst |
                vk::BufferUsageFlagBits::eVertexBuffer,
            deviceState.graphicsQueueFamilyID,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            true);

        // copy data to vertex buffer
        void* vertexStagingData = deviceState.logicalDevice->mapMemory(
            vertexStagingBuffer.allocation->memory,
            vertexStagingBuffer.allocation->offsetInDeviceMemory,
            vertexStagingBuffer.allocation->size);
        
        memcpy(vertexStagingData, quads.data(), vertexBufferSize);
        deviceState.logicalDevice->unmapMemory(vertexStagingBuffer.allocation->memory);
        CopyToBuffer(
            initState.copyCommandBuffer.get(),
            deviceState.graphicsQueue,
            vertexStagingBuffer.buffer.get(),
            renderState.vertexBuffer.buffer.get(),
            vk::BufferCopy(vertexBufferSize, 0U, 0U),
            std::make_optional(initState.copyCommandFence.get()));
        
        // wait for vertex buffer copy to finish
        deviceState.logicalDevice->waitForFences({ initState.copyCommandFence.get() }, 
            vk::Bool32(true), 
            std::numeric_limits<uint64_t>::max());
        deviceState.logicalDevice->resetFences({ initState.copyCommandFence.get() });
    }

    static void CreateRenderPass(RenderState& renderState, const DeviceState& deviceState, const SurfaceState& surfaceState)
    {
        // create render pass
        auto colorAttachmentDescription = vk::AttachmentDescription(
            vk::AttachmentDescriptionFlags(),
            surfaceState.surfaceFormat,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::ePresentSrcKHR
        );
        auto colorAttachmentRef = vk::AttachmentReference(
            0,
            vk::ImageLayout::eColorAttachmentOptimal);

        auto subpassDescription = vk::SubpassDescription(
            vk::SubpassDescriptionFlags(),
            vk::PipelineBindPoint::eGraphics,
            0,
            nullptr,
            1U,
            &colorAttachmentRef,
            nullptr,
            nullptr,
            0,
            nullptr
        );

        std::vector<vk::SubpassDependency> dependencies =
        {
            vk::SubpassDependency(
                0U,
                0U,
                vk::PipelineStageFlagBits::eBottomOfPipe,
                vk::PipelineStageFlagBits::eColorAttachmentOutput,
                vk::AccessFlagBits::eMemoryRead,
                vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
                vk::DependencyFlagBits::eByRegion
            ),
            vk::SubpassDependency(
                0U,
                0U,
                vk::PipelineStageFlagBits::eColorAttachmentOutput,
                vk::PipelineStageFlagBits::eBottomOfPipe,
                vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
                vk::AccessFlagBits::eMemoryRead,
                vk::DependencyFlagBits::eByRegion
            )
        };

        auto renderPassInfo = vk::RenderPassCreateInfo(
            vk::RenderPassCreateFlags(),
            1U,
            &colorAttachmentDescription,
            1U,
            &subpassDescription,
            static_cast<uint32_t>(dependencies.size()),
            dependencies.data());
            
        renderState.renderPass = deviceState.logicalDevice->createRenderPassUnique(renderPassInfo);
    }

    static void CreateSampler(RenderState& renderState, const DeviceState& deviceState)
    {
        renderState.sampler = deviceState.logicalDevice->createSamplerUnique(
            vk::SamplerCreateInfo(
                vk::SamplerCreateFlags(),
                vk::Filter::eLinear,
                vk::Filter::eLinear,
                vk::SamplerMipmapMode::eLinear,
                vk::SamplerAddressMode::eRepeat,
                vk::SamplerAddressMode::eRepeat,
                vk::SamplerAddressMode::eRepeat,
                0.f,
                1U,
                16.f,
                0U,
                vk::CompareOp::eNever));
    }

    static void CheckForPresentationSupport(const DeviceState& deviceState, const SurfaceState& surfaceState)
    {
        vk::Bool32 presentSupport;
        vkGetPhysicalDeviceSurfaceSupportKHR(deviceState.physicalDevice, 
            deviceState.graphicsQueueFamilyID, 
            surfaceState.surface.get(), 
            &presentSupport);
        if (!presentSupport)
        {
            std::runtime_error("Presentation to this surface isn't supported.");
            exit(1);
        }
    }

    static void ResetSwapChain(RenderState& renderState, const DeviceState& deviceState)
    {
        deviceState.logicalDevice->waitIdle();
        renderState.swapchain.reset();
    }

    static void CreateSwapchainWithDependencies(RenderState& renderState, 
        const DeviceState& deviceState, 
        const SurfaceState& surfaceState)
    {
        renderState.swapchain = deviceState.logicalDevice->createSwapchainKHRUnique(
            vk::SwapchainCreateInfoKHR(
                vk::SwapchainCreateFlagsKHR(),
                surfaceState.surface.get(),
                BufferCount,
                surfaceState.surfaceFormat,
                surfaceState.surfaceColorSpace,
                surfaceState.surfaceExtent,
                1U,
                vk::ImageUsageFlagBits::eColorAttachment,
                vk::SharingMode::eExclusive,
                1U,
                &deviceState.graphicsQueueFamilyID,
                vk::SurfaceTransformFlagBitsKHR::eIdentity,
                vk::CompositeAlphaFlagBitsKHR::eOpaque,
                surfaceState.presentMode,
                0U));
                
        renderState.swapImages = deviceState.logicalDevice->getSwapchainImagesKHR(renderState.swapchain.get());

        for (auto i = 0U; i < BufferCount; i++)
        {
            // create swap image view
            auto viewInfo = vk::ImageViewCreateInfo(
                vk::ImageViewCreateFlags(),
                renderState.swapImages[i],
                vk::ImageViewType::e2D,
                surfaceState.surfaceFormat,
                vk::ComponentMapping(),
                vk::ImageSubresourceRange(
                    vk::ImageAspectFlagBits::eColor,
                    0U,
                    1U,
                    0U,
                    1U));
            renderState.swapViews[i] = deviceState.logicalDevice->createImageViewUnique(viewInfo);

            // create framebuffer
            auto fbInfo = vk::FramebufferCreateInfo(
                vk::FramebufferCreateFlags(),
                renderState.renderPass.get(),
                1U,
                &renderState.swapViews[i].get(),
                surfaceState.surfaceExtent.width,
                surfaceState.surfaceExtent.height,
                1U);
            renderState.framebuffers[i] = deviceState.logicalDevice->createFramebufferUnique(fbInfo);
        }
    }

    static void InitializeSupportStructs(RenderState& renderState, const DeviceState& deviceState, const ShaderState& shaderState)
    {
        for (auto& support : renderState.supports)
        {
            support.renderCommandPool = deviceState.logicalDevice->createCommandPoolUnique(
				vk::CommandPoolCreateInfo(
					vk::CommandPoolCreateFlagBits::eResetCommandBuffer |
					vk::CommandPoolCreateFlagBits::eTransient,
					deviceState.graphicsQueueFamilyID));
			auto commandBuffers = deviceState.logicalDevice->allocateCommandBuffersUnique(
				vk::CommandBufferAllocateInfo(
					support.renderCommandPool.get(),
					vk::CommandBufferLevel::ePrimary,
					1U));
			support.renderCommandBuffer = std::move(commandBuffers[0]);
			support.renderBufferExecuted = deviceState.logicalDevice->createFenceUnique(
				vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
			support.imageRenderCompleteSemaphore = deviceState.logicalDevice->createSemaphoreUnique(
				vk::SemaphoreCreateInfo(
					vk::SemaphoreCreateFlags()));
        }
    }

    static void FinalizeImageOrder(RenderState& renderState)
    {
        auto imageOffset = 0;
        for (auto& image : renderState.images)
        {
            image.second.imageOffset = imageOffset;
            ++imageOffset;
        }
    }

    static void FinalizeSpriteOrder(RenderState& renderState)
    {
        auto spriteOffset = 0;
        for (auto& [spriteID, sprite] : renderState.sprites)
        {
            renderState.quads.push_back(sprite.quad);
            sprite.vertexOffset = spriteOffset;
            sprite.imageOffset = renderState.images[sprite.imageID].imageOffset;
            ++spriteOffset;
        }
    }
}