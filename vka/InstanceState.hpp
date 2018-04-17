#pragma once

#include "vulkan/vulkan.h"
#include "UniqueVulkan.hpp"
#include <iostream>

namespace vka
{
    struct InstanceState
    {
        VkInstanceUnique instance;
		VkDebugCallbackUnique debugCallback;
    };

    static void CreateInstance(InstanceState& instanceState, const vk::InstanceCreateInfo& instanceCreateInfo)
    {
		VkInstance instance;
		auto result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
		instanceState.instance = VkInstanceUnique(instance, VkInstanceDeleter());
    }

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugBreakCallback(
		VkDebugReportFlagsEXT flags, 
		VkDebugReportObjectTypeEXT objType, 
		uint64_t obj,
		size_t location,
		int32_t messageCode,
		const char* pLayerPrefix,
		const char* pMessage, 
		void* userData)
	{
		if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
			std::cerr << "(Debug Callback)";
		if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
			std::cerr << "(Error Callback)";
		if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
			return false;
			//std::cerr << "(Information Callback)";
		if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
			std::cerr << "(Performance Warning Callback)";
		if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
			std::cerr << "(Warning Callback)";
		std::cerr << pLayerPrefix << pMessage <<"\n";

		return false;
	}

    static void InitDebugCallback(InstanceState& instanceState)
	{
		auto debugCallbackCreateInfo = VkDebugReportCallbackCreateInfoEXT();
		debugCallbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		debugCallbackCreateInfo.pNext = nullptr;
		debugCallbackCreateInfo.flags = VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
			VK_DEBUG_REPORT_WARNING_BIT_EXT |
			VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
			VK_DEBUG_REPORT_ERROR_BIT_EXT |
			VK_DEBUG_REPORT_DEBUG_BIT_EXT;
		debugCallbackCreateInfo.pfnCallback = &debugBreakCallback;
		debugCallbackCreateInfo.pUserData = nullptr;

		VkDebugReportCallbackEXT callback;
		auto result = vkCreateDebugReportCallbackEXT(instanceState.instance.get(), &debugCallbackCreateInfo, nullptr, &callback);
	}
}
