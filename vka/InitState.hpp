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
    struct InitState
	{
		std::string 					windowTitle;
		int 							width; 
        int 							height;
		VkApplicationInfo 				appInfo;
		VkInstanceCreateInfo 			instanceCreateInfo;
		std::vector<const char*> 		deviceExtensions;
		std::string 					vertexShaderPath;
		std::string 					fragmentShaderPath;
	};
}