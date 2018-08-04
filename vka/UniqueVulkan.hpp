#pragma once
#include <memory>
#include "VulkanFunctions.hpp"
#include "vulkan/vulkan.h"

namespace vka {
struct VkInstanceDeleter {
  using pointer = VkInstance;
  void operator()(VkInstance instance) { vkDestroyInstance(instance, nullptr); }
};
using VkInstanceUnique = std::unique_ptr<VkInstance, VkInstanceDeleter>;

struct VkDebugReportCallbackEXTDeleter {
  using pointer = VkDebugReportCallbackEXT;
  VkInstance instance = VK_NULL_HANDLE;
  void operator()(VkDebugReportCallbackEXT callback) {
    vkDestroyDebugReportCallbackEXT(instance, callback, nullptr);
  }

  VkDebugReportCallbackEXTDeleter(VkInstance instance) : instance(instance) {}

  VkDebugReportCallbackEXTDeleter() noexcept = default;
};
using VkDebugReportCallbackEXTUnique =
    std::unique_ptr<VkDebugReportCallbackEXT, VkDebugReportCallbackEXTDeleter>;

struct VkDeviceDeleter {
  using pointer = VkDevice;
  void operator()(VkDevice device) { vkDestroyDevice(device, nullptr); }
};
using VkDeviceUnique = std::unique_ptr<VkDevice, VkDeviceDeleter>;

struct VkDeviceMemoryDeleter {
  using pointer = VkDeviceMemory;
  VkDevice device = VK_NULL_HANDLE;
  void operator()(VkDeviceMemory memory) {
    vkFreeMemory(device, memory, nullptr);
  }

  VkDeviceMemoryDeleter(VkDevice device) : device(device) {}

  VkDeviceMemoryDeleter() noexcept = default;
};
using VkDeviceMemoryUnique =
    std::unique_ptr<VkDeviceMemory, VkDeviceMemoryDeleter>;

struct VkBufferDeleter {
  using pointer = VkBuffer;
  VkDevice device = VK_NULL_HANDLE;

  void operator()(VkBuffer buffer) { vkDestroyBuffer(device, buffer, nullptr); }

  VkBufferDeleter(VkDevice device) : device(device) {}

  VkBufferDeleter() noexcept = default;
};
using VkBufferUnique = std::unique_ptr<VkBuffer, VkBufferDeleter>;

struct VkBufferViewDeleter {
  using pointer = VkBufferView;
  VkDevice device = VK_NULL_HANDLE;

  void operator()(VkBufferView view) {
    vkDestroyBufferView(device, view, nullptr);
  }

  VkBufferViewDeleter(VkDevice device) : device(device) {}
};
using VkBufferViewUnique = std::unique_ptr<VkBufferView, VkBufferViewDeleter>;

struct VkImageDeleter {
  using pointer = VkImage;
  VkDevice device = VK_NULL_HANDLE;

  void operator()(VkImage image) { vkDestroyImage(device, image, nullptr); }

  VkImageDeleter(VkDevice device) : device(device) {}

  VkImageDeleter() noexcept = default;
};
using VkImageUnique = std::unique_ptr<VkImage, VkImageDeleter>;

struct VkImageViewDeleter {
  using pointer = VkImageView;
  VkDevice device = VK_NULL_HANDLE;

  void operator()(VkImageView view) {
    vkDestroyImageView(device, view, nullptr);
  }

  VkImageViewDeleter(VkDevice device) : device(device) {}

  VkImageViewDeleter() noexcept = default;
};
using VkImageViewUnique = std::unique_ptr<VkImageView, VkImageViewDeleter>;

struct VkFenceDeleter {
  using pointer = VkFence;
  VkDevice device = VK_NULL_HANDLE;

  void operator()(VkFence fence) { vkDestroyFence(device, fence, nullptr); }

  VkFenceDeleter(VkDevice device) : device(device) {}

  VkFenceDeleter() noexcept = default;
};
using VkFenceUnique = std::unique_ptr<VkFence, VkFenceDeleter>;

struct VkSemaphoreDeleter {
  using pointer = VkSemaphore;
  VkDevice device = VK_NULL_HANDLE;

  void operator()(VkSemaphore semaphore) {
    vkDestroySemaphore(device, semaphore, nullptr);
  }

  VkSemaphoreDeleter(VkDevice device) : device(device) {}

  VkSemaphoreDeleter() noexcept = default;
};
using VkSemaphoreUnique = std::unique_ptr<VkSemaphore, VkSemaphoreDeleter>;

struct VkCommandPoolDeleter {
  using pointer = VkCommandPool;
  VkDevice device = VK_NULL_HANDLE;

  void operator()(VkCommandPool pool) {
    vkDestroyCommandPool(device, pool, nullptr);
  }

  VkCommandPoolDeleter(VkDevice device) : device(device) {}

  VkCommandPoolDeleter() noexcept = default;
};
using VkCommandPoolUnique =
    std::unique_ptr<VkCommandPool, VkCommandPoolDeleter>;

struct VkSurfaceKHRDeleter {
  using pointer = VkSurfaceKHR;
  VkInstance instance = VK_NULL_HANDLE;

  void operator()(VkSurfaceKHR surface) {
    vkDestroySurfaceKHR(instance, surface, nullptr);
  }

  VkSurfaceKHRDeleter(VkInstance instance) : instance(instance) {}

  VkSurfaceKHRDeleter() noexcept = default;
};
using VkSurfaceKHRUnique = std::unique_ptr<VkSurfaceKHR, VkSurfaceKHRDeleter>;

struct VkFramebufferDeleter {
  using pointer = VkFramebuffer;
  VkDevice device = VK_NULL_HANDLE;

  void operator()(VkFramebuffer framebuffer) {
    vkDestroyFramebuffer(device, framebuffer, nullptr);
  }

  VkFramebufferDeleter(VkDevice device) : device(device) {}

  VkFramebufferDeleter() noexcept = default;
};
using VkFramebufferUnique =
    std::unique_ptr<VkFramebuffer, VkFramebufferDeleter>;

struct VkRenderPassDeleter {
  using pointer = VkRenderPass;
  VkDevice device = VK_NULL_HANDLE;

  void operator()(VkRenderPass renderpass) {
    vkDestroyRenderPass(device, renderpass, nullptr);
  }

  VkRenderPassDeleter(VkDevice device) : device(device) {}

  VkRenderPassDeleter() noexcept = default;
};
using VkRenderPassUnique = std::unique_ptr<VkRenderPass, VkRenderPassDeleter>;

struct VkSamplerDeleter {
  using pointer = VkSampler;
  VkDevice device = VK_NULL_HANDLE;

  void operator()(VkSampler sampler) {
    vkDestroySampler(device, sampler, nullptr);
  }

  VkSamplerDeleter(VkDevice device) : device(device) {}

  VkSamplerDeleter() noexcept = default;
};
using VkSamplerUnique = std::unique_ptr<VkSampler, VkSamplerDeleter>;

struct VkShaderModuleDeleter {
  using pointer = VkShaderModule;
  VkDevice device = VK_NULL_HANDLE;

  void operator()(VkShaderModule shader) {
    vkDestroyShaderModule(device, shader, nullptr);
  }

  VkShaderModuleDeleter(VkDevice device) : device(device) {}

  VkShaderModuleDeleter() noexcept = default;
};
using VkShaderModuleUnique =
    std::unique_ptr<VkShaderModule, VkShaderModuleDeleter>;

struct VkDescriptorSetLayoutDeleter {
  using pointer = VkDescriptorSetLayout;
  VkDevice device = VK_NULL_HANDLE;

  void operator()(VkDescriptorSetLayout layout) {
    vkDestroyDescriptorSetLayout(device, layout, nullptr);
  }

  VkDescriptorSetLayoutDeleter(VkDevice device) : device(device) {}

  VkDescriptorSetLayoutDeleter() noexcept = default;
};
using VkDescriptorSetLayoutUnique =
    std::unique_ptr<VkDescriptorSetLayout, VkDescriptorSetLayoutDeleter>;

struct VkDescriptorPoolDeleter {
  using pointer = VkDescriptorPool;
  VkDevice device = VK_NULL_HANDLE;

  void operator()(VkDescriptorPool pool) {
    vkDestroyDescriptorPool(device, pool, nullptr);
  }

  VkDescriptorPoolDeleter(VkDevice device) : device(device) {}

  VkDescriptorPoolDeleter() noexcept = default;
};
using VkDescriptorPoolUnique =
    std::unique_ptr<VkDescriptorPool, VkDescriptorPoolDeleter>;

struct VkSwapchainKHRDeleter {
  using pointer = VkSwapchainKHR;
  VkDevice device = VK_NULL_HANDLE;

  void operator()(VkSwapchainKHR swapchain) {
    vkDestroySwapchainKHR(device, swapchain, nullptr);
  }

  VkSwapchainKHRDeleter(VkDevice device) : device(device) {}

  VkSwapchainKHRDeleter() noexcept = default;
};
using VkSwapchainKHRUnique =
    std::unique_ptr<VkSwapchainKHR, VkSwapchainKHRDeleter>;

struct VkPipelineLayoutDeleter {
  using pointer = VkPipelineLayout;
  VkDevice device = VK_NULL_HANDLE;

  void operator()(VkPipelineLayout layout) {
    vkDestroyPipelineLayout(device, layout, nullptr);
  }

  VkPipelineLayoutDeleter(VkDevice device) : device(device) {}

  VkPipelineLayoutDeleter() noexcept = default;
};
using VkPipelineLayoutUnique =
    std::unique_ptr<VkPipelineLayout, VkPipelineLayoutDeleter>;

struct VkPipelineDeleter {
  using pointer = VkPipeline;
  VkDevice device = VK_NULL_HANDLE;

  void operator()(VkPipeline pipeline) {
    vkDestroyPipeline(device, pipeline, nullptr);
  }

  VkPipelineDeleter(VkDevice device) : device(device) {}

  VkPipelineDeleter() noexcept = default;
};
using VkPipelineUnique = std::unique_ptr<VkPipeline, VkPipelineDeleter>;
}  // namespace vka