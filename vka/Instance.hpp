#pragma once

#include "UniqueVulkan.hpp"
#include "VulkanFunctionLoader.hpp"
#include "vulkan/vulkan.h"

namespace vka {
class Instance {
 public:
  Instance(const VkInstanceCreateInfo& createInfo) : createInfo(createInfo) {
    CreateInstance();
  }

  VkInstance GetInstance() { return instanceUnique.get(); }

 private:
  VkInstanceCreateInfo createInfo;
  VkInstanceUnique instanceUnique;

  void CreateInstance() {
    VkInstance instance;
    auto result = vkCreateInstance(&createInfo, nullptr, &instance);
    instanceUnique = VkInstanceUnique(instance, VkInstanceDeleter());
    LoadInstanceLevelEntryPoints(instance);
  }
};
}  // namespace vka
