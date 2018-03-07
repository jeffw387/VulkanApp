#pragma once

#include "vulkan/vulkan.hpp"
#include "InitState.hpp"
#include "Buffer.hpp"
#include "Camera.hpp"
#include "glm/glm.hpp"
#include "Sprite.hpp"
#include "Vertex.hpp"
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
        const DeviceState& deviceState, 
        const InitState& initState,
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
    }
}