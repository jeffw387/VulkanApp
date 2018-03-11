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
#include <array>
#include <vector>

namespace vka
{
    static constexpr size_t BufferCount = 3U;

    struct Supports
    {
        vk::UniqueCommandPool commandPool;
        vk::UniqueCommandBuffer copyCommandBuffer;
        vk::UniqueCommandBuffer renderCommandBuffer;
        AllocatedBuffer matrixStagingBuffer;
        AllocatedBuffer matrixBuffer;
        size_t bufferCapacity = 0U;
        vk::UniqueDescriptorPool vertexLayoutDescriptorPool;
        vk::UniqueDescriptorSet vertexDescriptorSet;
        vk::UniqueFence renderBufferExecuted;
        vk::UniqueFence imagePresentCompleteFence;
        vk::UniqueSemaphore matrixBufferStagingCompleteSemaphore;
        vk::UniqueSemaphore imageRenderCompleteSemaphore;
        entt::ResourceCache<Image2D> images;
		entt::ResourceCache<Sprite> sprites;
		std::vector<Sprite> spriteVector;
    };

    struct RenderState
    {
        AllocatedBuffer vertexBuffer;
		AllocatedBuffer indexBuffer;
        vk::UniqueRenderPass renderPass;
		vk::UniqueSampler sampler;
        std::array<Supports, BufferCount> supports;
		vk::UniqueSwapchainKHR swapchain;
		std::vector<vk::Image> swapImages;
		std::array<vk::UniqueImageView, BufferCount> swapViews;
		std::array<vk::UniqueFramebuffer, BufferCount> framebuffers;
        Camera2D camera;
        uint32_t nextImage;
        glm::mat4 vp;
        vk::DeviceSize copyOffset;
        void* mapped;
        SpriteIndex spriteIndex;
    };

    static void CreateVertexAndIndexBuffers(
        RenderState& renderState, 
        const InitState& initState,
        const DeviceState& deviceState, 
        const std::vector<Sprite>& sprites)
    {
        // create vertex buffers
        constexpr auto quadSize = sizeof(Quad);
        size_t vertexBufferSize = quadSize * sprites.size();
        auto vertexStagingBuffer = CreateBuffer(
            deviceState.logicalDevice,
            deviceState.allocator,
            vertexBufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent,
            true);
        
        renderState.vertexBuffer = CreateBuffer(
            deviceState.logicalDevice,
            deviceState.allocator,
            vertexBufferSize, 
            vk::BufferUsageFlagBits::eTransferDst |
                vk::BufferUsageFlagBits::eVertexBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            true);

        // copy data to vertex buffer
        void* vertexStagingData = deviceState.logicalDevice->mapMemory(
            vertexStagingBuffer.allocation->memory,
            vertexStagingBuffer.allocation->offsetInDeviceMemory,
            vertexStagingBuffer.allocation->size);
        
        memcpy(vertexStagingData, sprites.data(), vertexBufferSize);
        deviceState.logicalDevice->unmapMemory(vertexStagingBuffer.allocation->memory);
        CopyToBuffer(
            initState.copyCommandBuffer.get(),
            vertexStagingBuffer.buffer.get(),
            vertexBuffer.buffer.get(),
            vertexBufferSize,
            0U,
            0U,
            std::make_optional(initState.copyCommandFence.get()));
        
        // wait for vertex buffer copy to finish
        deviceState.logicalDevice->waitForFences({ initState.copyCommandFence.get() }, 
            vk::Bool32(true), 
            std::numeric_limits<uint64_t>::max());
        deviceState.logicalDevice->resetFences({ initState.copyCommandFence.get() });

        constexpr auto quadIndexSize = sizeof(std::array<VertexIndex, IndicesPerQuad>);
        constexpr auto indexBufferSize = quadIndexSize * sprites.size();

        auto indexStagingBuffer = CreateBuffer(
            deviceState.logicalDevice,
            deviceState.allocator,
            indexBufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent,
            true);

        renderState.indexBuffer = CreateBuffer(
            deviceState.logicalDevice,
            deviceState.allocator,
            indexBufferSize,
            vk::BufferUsageFlagBits::eTransferDst |
                vk::BufferUsageFlagBits::eIndexBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            true);

        // copy data to index buffer
        void* indexStagingData = deviceState.logicalDevice->mapMemory(
            indexStagingBuffer.allocation->memory,
            indexStagingBuffer.allocation->offsetInDeviceMemory,
            indexStagingBuffer.allocation->size);
        
        auto indexOffset = 0U;
        SpriteIndex spriteIndex = 0U;
        for (const Sprite& sprite : sprites)
        {
            auto indices = Quad::getIndices(spriteIndex);
            memcpy(indexStagingData + indexOffset, indices.data(), quadIndexSize);
            ++spriteIndex;
            indexOffset = quadIndexSize * spriteIndex;
        }
        deviceState.logicalDevice->unmapMemory(indexStagingBuffer.allocation->memory);
        
        CopyToBuffer(
            initState.copyCommandBuffer.get(),
            indexStagingBuffer.buffer.get(),
            renderState.indexBuffer.buffer.get(),
            indexBufferSize,
            0U,
            0U,
            std::make_optional(initState.copyCommandFence.get()));

        deviceState.logicalDevice->waitForFences({ initState.copyCommandFence.get() }, 
            vk::Bool32(true), 
            std::numeric_limits<uint64_t>::max());
        deviceState.logicalDevice->resetFences({ initState.copyCommandFence.get() });
    }

    void CreateRenderPass(RenderState& renderState, const DeviceState& deviceState, const SurfaceState& surfaceState)
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

    void CreateSampler(RenderState& renderState, const DeviceState& deviceState)
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

    void CheckForPresentationSupport(const DeviceState& deviceState, const SurfaceState& surfaceState)
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

    void ResetSwapChain(RenderState& renderState, const DeviceState& deviceState)
    {
        deviceState.logicalDevice->waitIdle();
        renderState.swapChain->reset();
    }

    void CreateSwapchainWithDependencies(RenderState& renderState, 
        const DeviceState& deviceState, 
        const SurfaceState& surfaceState)
    {
        renderState.swapChain = deviceState.logicalDevice->createSwapchainKHRUnique(
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
                
        renderState.swapImages = deviceState.logicalDevice->getSwapchainImagesKHR(renderState.swapChain.get());

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

    InitializeSupportStructs(RenderState& renderState, const DeviceState& deviceState, const ShaderState& shaderState)
    {
        for (auto& support : renderState.supports)
        {
            support.commandPool = deviceState.logicalDevice->createCommandPoolUnique(
				vk::CommandPoolCreateInfo(
					vk::CommandPoolCreateFlagBits::eResetCommandBuffer |
					vk::CommandPoolCreateFlagBits::eTransient,
					deviceState.graphicsQueueFamilyID));
			auto commandBuffers = deviceState.logicalDevice->allocateCommandBuffersUnique(
				vk::CommandBufferAllocateInfo(
					support.commandPool.get(),
					vk::CommandBufferLevel::ePrimary,
					2U));
			support.copyCommandBuffer = std::move(commandBuffers[0]);
			support.renderCommandBuffer = std::move(commandBuffers[1]);
			support.vertexLayoutDescriptorPool = createVertexDescriptorPool();
			support.bufferCapacity = 0U;
			support.renderBufferExecuted = deviceState.logicalDevice->createFenceUnique(
				vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
			support.imageRenderCompleteSemaphore = deviceState.logicalDevice->createSemaphoreUnique(
				vk::SemaphoreCreateInfo(
					vk::SemaphoreCreateFlags()));
			support.matrixBufferStagingCompleteSemaphore = deviceState.logicalDevice->createSemaphoreUnique(
				vk::SemaphoreCreateInfo(
					vk::SemaphoreCreateFlags()));
			support.vertexLayoutDescriptorPool = createVertexDescriptorPool();
			support.vertexDescriptorSet = vk::UniqueDescriptorSet(deviceState.logicalDevice->allocateDescriptorSets(
				vk::DescriptorSetAllocateInfo(
					support.vertexLayoutDescriptorPool.get(),
					1U,
					&shaderState.vertexDescriptorSetLayout.get()))[0],
				vk::DescriptorSetDeleter(deviceState.logicalDevice.get(),
					support.vertexLayoutDescriptorPool.get()));
        }
    }
}