#pragma once

#include "VulkanFunctions.hpp"
#include "vulkan/vulkan.hpp"
#include <iostream>

namespace vka
{
    struct InstanceState
    {
        HMODULE vulkanLibrary;
        vk::UniqueInstance instance;
        vk::UniqueDebugReportCallbackEXT debugBreakpointCallbackData;
    };

    static void CreateInstance(InstanceState& instanceState, const vk::InstanceCreateInfo& instanceCreateInfo)
    {
        instanceState.instance = vk::createInstanceUnique(instanceCreateInfo);
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
		if (flags & VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_DEBUG_BIT_EXT)
			std::cerr << "(Debug Callback)";
		if (flags & VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_ERROR_BIT_EXT)
			std::cerr << "(Error Callback)";
		if (flags & VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
			return false;
			//std::cerr << "(Information Callback)";
		if (flags & VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
			std::cerr << "(Performance Warning Callback)";
		if (flags & VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_WARNING_BIT_EXT)
			std::cerr << "(Warning Callback)";
		std::cerr << pLayerPrefix << pMessage <<"\n";

		return false;
	}

    static void InitDebugCallback(InstanceState& instanceState)
	{
		if (vkCreateDebugReportCallbackEXT)
		{
			instanceState.debugBreakpointCallbackData = instanceState.instance->createDebugReportCallbackEXTUnique(
				vk::DebugReportCallbackCreateInfoEXT(
					vk::DebugReportFlagBitsEXT::ePerformanceWarning |
					vk::DebugReportFlagBitsEXT::eError |
					vk::DebugReportFlagBitsEXT::eDebug |
					vk::DebugReportFlagBitsEXT::eInformation |
					vk::DebugReportFlagBitsEXT::eWarning,
					reinterpret_cast<PFN_vkDebugReportCallbackEXT>(&debugBreakCallback)
				));
		}
	}
}

static bool LoadVulkanDLL(vka::InstanceState& instanceState)
{
	// TODO: remove platform dependence
	instanceState.vulkanLibrary = LoadLibrary("vulkan-1.dll");

	if (instanceState.vulkanLibrary == nullptr)
		return false;
	return true;
}

static bool LoadVulkanEntryPoint(vka::InstanceState& instanceState)
{
#define VK_EXPORTED_FUNCTION( fun )                                                   \
    if( !(fun = (PFN_##fun)GetProcAddress( instanceState.vulkanLibrary, #fun )) ) {   \
    std::cout << "Could not load exported function: " << #fun << "!" << std::endl;    \
    }

#include "VulkanFunctions.inl"

    return true;
}

static bool LoadVulkanGlobalFunctions()
{
#define VK_GLOBAL_LEVEL_FUNCTION( fun )                                                   \
    if( !(fun = (PFN_##fun)vkGetInstanceProcAddr( nullptr, #fun )) ) {                    \
    std::cout << "Could not load global level function: " << #fun << "!" << std::endl;    \
    }

#include "VulkanFunctions.inl"

    return true;
}

static bool LoadVulkanInstanceFunctions(vka::InstanceState& instanceState)
{
#define VK_INSTANCE_LEVEL_FUNCTION( fun )                                                   \
    if( !(fun = (PFN_##fun)vkGetInstanceProcAddr( instanceState.instance.get(), #fun )) ) { \
    std::cout << "Could not load instance level function: " << #fun << "!" << std::endl;    \
    }

#include "VulkanFunctions.inl"

    return true;
}