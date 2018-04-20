#pragma once
#include "UniqueVulkan.hpp"
#include "vulkan/vulkan.h"
#include "VulkanFunctions.hpp"
#include "TimeHelper.hpp"
#include "Sprite.hpp"
#include "DeviceState.hpp"
#include <string>
#include <vector>
#include <functional>

namespace vka
{
    struct VulkanApp;
    using ImageLoadFuncPtr = std::function<void()>;
	using UpdateFuncPtr = std::function<void(TimePoint_ms)>;
	using RenderFuncPtr = std::function<void()>;

    struct InitState
	{
		std::string windowTitle;
		int width; 
        int height;
		VkInstanceCreateInfo instanceCreateInfo;
		std::vector<const char*> deviceExtensions;
		std::string vertexShaderPath;
		std::string fragmentShaderPath;
		ImageLoadFuncPtr imageLoadCallback;
		UpdateFuncPtr updateCallback;
		RenderFuncPtr renderCallback;
	};
}