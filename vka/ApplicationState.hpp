#pragma once

#include "GLFW/glfw3.h"
#include "TimeHelper.hpp"
#include <iostream>

namespace vka
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    using LibraryHandle = HMODULE;
#elif defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_XLIB_KHR)
    using LibraryHandle = void*;
#endif

#if defined(VK_USE_PLATFORM_WIN32_KHR)
    #define LoadProcAddress GetProcAddress
#elif defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_XLIB_KHR)
    #define LoadProcAddress dlsym
#endif

    struct ApplicationState
    {
        GLFWwindow* window;
        TimePoint_ms startupTimePoint;
        TimePoint_ms currentSimulationTime;
        bool gameLoop = false;
        LibraryHandle VulkanLibrary;
    };

    static bool LoadVulkanLibrary(ApplicationState& appState) 
    {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        appState.VulkanLibrary = LoadLibrary( "vulkan-1.dll" );
#elif defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_XLIB_KHR)
        appState.VulkanLibrary = dlopen( "libvulkan.so.1", RTLD_NOW );
#endif
        if( appState.VulkanLibrary == nullptr ) 
        {
            std::cout << "Could not load Vulkan library!" << std::endl;
            return false;
        }
        return true;
    }

    static void LoadExportedEntryPoints(const ApplicationState& appState) 
	{
#define VK_EXPORTED_FUNCTION( fun )                                                             \
        if( !(fun = (PFN_##fun)LoadProcAddress( appState.VulkanLibrary, #fun )) )               \
        {                                                                                       \
            std::cout << "Could not load exported function: " << #fun << "!" << std::endl;      \
        }
        #include "VulkanFunctions.inl"
    }

    static void LoadGlobalLevelEntryPoints() 
	{
#define VK_GLOBAL_LEVEL_FUNCTION( fun )                                                     	\
		if( !(fun = (PFN_##fun)vkGetInstanceProcAddr( nullptr, #fun )) ) 						\
		{                      																	\
			std::cout << "Could not load global level function: " << #fun << "!" << std::endl;  \
		}

#include "VulkanFunctions.inl"
  	}
}