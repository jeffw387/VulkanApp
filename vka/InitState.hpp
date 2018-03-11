#pragma once
#include "vulkan/vulkan.hpp"
#include "TimeHelper.hpp"
#include "Sprite.hpp"
#include "DeviceState.hpp"
#include <string>
#include <vector>
#include <functional>

namespace vka
{
    struct VulkanApp;
    using SpriteCount = SpriteIndex;
    using ImageLoadFuncPtr = std::function<void(VulkanApp*)>;
	using UpdateFuncPtr = std::function<SpriteCount(TimePoint_ms)>;
	using RenderFuncPtr = std::function<void()>;

    struct InitState
	{
		std::string windowTitle;
		int width; 
        int height;
		vk::InstanceCreateInfo instanceCreateInfo;
		std::vector<const char*> deviceExtensions;
		std::string vertexShaderPath;
		std::string fragmentShaderPath;
		ImageLoadFuncPtr imageLoadCallback;
		UpdateFuncPtr updateCallback;
		RenderFuncPtr renderCallback;
		vk::UniqueCommandPool copyCommandPool;
		vk::UniqueCommandBuffer copyCommandBuffer;
		vk::UniqueFence copyCommandFence;
	};

    void InitCopyStructures(InitState& initState, const DeviceState& deviceState)
    {
        // create copy command objects
			auto copyPoolCreateInfo = vk::CommandPoolCreateInfo(
				vk::CommandPoolCreateFlagBits::eResetCommandBuffer |
				vk::CommandPoolCreateFlagBits::eTransient,
				deviceState.graphicsQueueFamilyID);
			initState.copyCommandPool = deviceState.logicalDevice->createCommandPoolUnique(copyPoolCreateInfo);
			auto copyCmdBufferInfo = vk::CommandBufferAllocateInfo(
				initState.copyCommandPool.get(),
				vk::CommandBufferLevel::ePrimary,
				1U);
			
			initState.copyCommandBuffer = std::move(
				deviceState.logicalDevice->allocateCommandBuffersUnique(copyCmdBufferInfo)[0]);
			initState.copyCommandFence = deviceState.logicalDevice->createFenceUnique(
				vk::FenceCreateInfo(/*vk::FenceCreateFlagBits::eSignaled*/));
    }
}