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
#include <array>
#include <cstring>
#include <vector>
#include <string>


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
        entt::ResourceCache<Image2D> images;
		entt::ResourceCache<Sprite> sprites;
		std::vector<Sprite> spriteVector;
    };

    struct FragmentPushConstants
	{
		glm::uint32 textureID;
		glm::float32 r, g, b, a;
	};

    struct ShaderState
    {
        std::string vertexShaderPath;
		std::string fragmentShaderPath;
        vk::UniqueShaderModule vertexShader;
		vk::UniqueShaderModule fragmentShader;
		vk::UniqueDescriptorSetLayout vertexDescriptorSetLayout;
		vk::UniqueDescriptorSetLayout fragmentDescriptorSetLayout;
		std::vector<vk::PushConstantRange> pushConstantRanges;
		vk::UniqueDescriptorPool fragmentLayoutDescriptorPool;
		vk::UniqueDescriptorSet fragmentDescriptorSet;
    };

    void CreateShaderModules(ShaderState& shaderState, const DeviceState& deviceState)
	{
		// read from file
		auto vertexShaderBinary = fileIO::readFile(shaderState.vertexShaderPath);
		// create shader module
		auto vertexShaderInfo = vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(),
			vertexShaderBinary.size(),
			reinterpret_cast<const uint32_t*>(vertexShaderBinary.data()));
		shaderState.vertexShader = deviceState.logicalDevice->createShaderModuleUnique(vertexShaderInfo);

		// read from file
		auto fragmentShaderBinary = fileIO::readFile(shaderState.fragmentShaderPath);
		// create shader module
		auto fragmentShaderInfo = vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(),
			fragmentShaderBinary.size(),
			reinterpret_cast<const uint32_t*>(fragmentShaderBinary.data()));
		shaderState.fragmentShader = deviceState.logicalDevice->createShaderModuleUnique(fragmentShaderInfo);
	}

	vk::UniqueDescriptorPool CreateVertexDescriptorPool(const DeviceState& deviceState)
	{
		auto poolSizes = std::vector<vk::DescriptorPoolSize>
		{
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1U)
		};

		return deviceState.logicalDevice->createDescriptorPoolUnique(
			vk::DescriptorPoolCreateInfo(
				vk::DescriptorPoolCreateFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet),
				BufferCount,
				static_cast<uint32_t>(poolSizes.size()),
				poolSizes.data()));
	}

	void CreateFragmentDescriptorPool(ShaderState& shaderState, 
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

	void CreateVertexSetLayout(ShaderState& shaderState, const DeviceState& deviceState)
	{
		auto vertexLayoutBindings = std::vector<vk::DescriptorSetLayoutBinding>
		{
			// Vertex: matrix uniform buffer (dynamic)
			vk::DescriptorSetLayoutBinding(
				0U,
				vk::DescriptorType::eUniformBufferDynamic,
				1U,
				vk::ShaderStageFlagBits::eVertex)
		};

		shaderState.vertexDescriptorSetLayout = deviceState.logicalDevice->createDescriptorSetLayoutUnique(
			vk::DescriptorSetLayoutCreateInfo(
				vk::DescriptorSetLayoutCreateFlags(),
				static_cast<uint32_t>(vertexLayoutBindings.size()),
				vertexLayoutBindings.data()));
	}

	void CreateFragmentSetLayout(ShaderState& shaderState, 
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

	std::vector<vk::DescriptorSetLayout> GetSetLayouts(const ShaderState& shaderState)
	{
		return std::vector<vk::DescriptorSetLayout>
		{ 
			shaderState.vertexDescriptorSetLayout.get(), 
			shaderState.fragmentDescriptorSetLayout.get()
		};
	}

	void SetupPushConstants(ShaderState& shaderState)
	{
		shaderState.pushConstantRanges.emplace_back(vk::PushConstantRange(
			vk::ShaderStageFlagBits::eFragment,
			0U,
			sizeof(FragmentPushConstants)));
	}

	void CreateAndUpdateFragmentDescriptorSet(ShaderState& shaderState, 
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
		for (Image2D& image : renderState.images)
		{
			imageInfos.emplace_back(vk::DescriptorImageInfo(
				nullptr,
				image.view,
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

    static void CreateVertexAndIndexBuffers(
        RenderState& renderState, 
        DeviceState& deviceState, 
        const InitState& initState,
        const std::vector<Sprite>& sprites)
    {
        // create vertex buffers
        constexpr auto quadSize = sizeof(Quad);
        size_t vertexBufferSize = quadSize * sprites.size();
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
        
        memcpy(vertexStagingData, sprites.data(), vertexBufferSize);
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

        auto indexBufferSize = QuadIndicesSize * sprites.size();

        auto indexStagingBuffer = CreateBuffer(
            deviceState.logicalDevice.get(),
            deviceState.allocator,
            indexBufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            deviceState.graphicsQueueFamilyID,
            vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent,
            true);

        renderState.indexBuffer = CreateBuffer(
            deviceState.logicalDevice.get(),
            deviceState.allocator,
            indexBufferSize,
            vk::BufferUsageFlagBits::eTransferDst |
                vk::BufferUsageFlagBits::eIndexBuffer,
            deviceState.graphicsQueueFamilyID,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            true);

        // copy data to index buffer
        void* indexStagingData = deviceState.logicalDevice->mapMemory(
            indexStagingBuffer.allocation->memory,
            indexStagingBuffer.allocation->offsetInDeviceMemory,
            indexStagingBuffer.allocation->size);
        
        SpriteIndex spriteIndex = 0U;
        auto quadIndexArray = reinterpret_cast<QuadIndices*>(indexStagingData);
        for (const Sprite& sprite : sprites)
        {
            auto indices = Quad::getIndices(spriteIndex);
            auto destPtr = reinterpret_cast<void*>(quadIndexArray + spriteIndex);
            std::memcpy(destPtr, &indices, QuadIndicesSize);
            ++spriteIndex;
        }
        deviceState.logicalDevice->unmapMemory(indexStagingBuffer.allocation->memory);
        
        CopyToBuffer(
            initState.copyCommandBuffer.get(),
            deviceState.graphicsQueue,
            indexStagingBuffer.buffer.get(),
            renderState.indexBuffer.buffer.get(),
            vk::BufferCopy(indexBufferSize, 0U, 0U),
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
        renderState.swapchain.reset();
    }

    void CreateSwapchainWithDependencies(RenderState& renderState, 
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

    void InitializeSupportStructs(RenderState& renderState, const DeviceState& deviceState, const ShaderState& shaderState)
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
			support.vertexLayoutDescriptorPool = CreateVertexDescriptorPool(deviceState);
			support.bufferCapacity = 0U;
			support.renderBufferExecuted = deviceState.logicalDevice->createFenceUnique(
				vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
			support.imageRenderCompleteSemaphore = deviceState.logicalDevice->createSemaphoreUnique(
				vk::SemaphoreCreateInfo(
					vk::SemaphoreCreateFlags()));
			support.matrixBufferStagingCompleteSemaphore = deviceState.logicalDevice->createSemaphoreUnique(
				vk::SemaphoreCreateInfo(
					vk::SemaphoreCreateFlags()));
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