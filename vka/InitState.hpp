#pragma once

namespace vka
{
    struct InitState
	{
		std::string WindowTitle;
		int width; int height;
		vk::InstanceCreateInfo instanceCreateInfo;
		std::vector<const char*> deviceExtensions;
		std::string vertexShaderPath;
		std::string fragmentShaderPath;
		std::function<void(VulkanApp*)> imageLoadCallback;
		UpdateFuncPtr updateCallback;
		RenderFuncPtr renderCallback;
		vk::UniqueCommandPool copyCommandPool;
		vk::UniqueCommandBuffer copyCommandBuffer;
		vk::UniqueFence copyCommandFence;
	};
}