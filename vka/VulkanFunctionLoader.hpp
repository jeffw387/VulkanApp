#pragma once

#include <stdexcept>
#include "VulkanFunctions.hpp"
#include "vulkan/vulkan.h"

namespace vka {

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

static LibraryHandle LoadVulkanLibrary() {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
  auto library = LoadLibrary("vulkan-1.dll");
#elif defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_XLIB_KHR)
  auto library = dlopen("libvulkan.so.1", RTLD_NOW);
#endif
  if (library == nullptr) {
    std::runtime_error("Could not load Vulkan library!\n");
  }
  return library;
}

static void LoadExportedEntryPoints(LibraryHandle const library) {
#define VK_EXPORTED_FUNCTION(fun)                                    \
  if (!(fun = (PFN_##fun)LoadProcAddress(library, #fun))) {          \
    std::cout << "Could not load exported function: " << #fun << "!" \
              << std::endl;                                          \
  }
#include "VulkanFunctions.inl"
}

static void LoadGlobalLevelEntryPoints() {
#define VK_GLOBAL_LEVEL_FUNCTION(fun)                                    \
  if (!(fun = (PFN_##fun)vkGetInstanceProcAddr(nullptr, #fun))) {        \
    std::cout << "Could not load global level function: " << #fun << "!" \
              << std::endl;                                              \
  }
#include "VulkanFunctions.inl"
}

static void LoadInstanceLevelEntryPoints(const VkInstance& instance) {
#define VK_INSTANCE_LEVEL_FUNCTION(fun)                                    \
  if (!(fun = (PFN_##fun)vkGetInstanceProcAddr(instance, #fun))) {         \
    std::cout << "Could not load instance level function: " << #fun << "!" \
              << std::endl;                                                \
  }
#include "VulkanFunctions.inl"
}

static void LoadDeviceLevelEntryPoints(const VkDevice& device) {
#define VK_DEVICE_LEVEL_FUNCTION(fun)                                    \
  if (!(fun = (PFN_##fun)vkGetDeviceProcAddr(device, #fun))) {           \
    std::cout << "Could not load device level function: " << #fun << "!" \
              << std::endl;                                              \
  }
#include "VulkanFunctions.inl"
}
}  // namespace vka