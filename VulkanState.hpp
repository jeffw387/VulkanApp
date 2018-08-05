#pragma once

#include "GLTF.hpp"
#include "Pool.hpp"
#include "vka/Allocator.hpp"
#include "vka/Buffer.hpp"
#include "vka/Image2D.hpp"
#include "vka/Quad.hpp"
#include "vka/UniqueVulkan.hpp"
#include "vka/VulkanFunctionLoader.hpp"
#include "vulkan/vulkan.h"

#include <array>
#include <vector>

constexpr uint32_t BufferCount = 3;
constexpr uint32_t QuadCount = 1;
constexpr VkExtent2D DefaultWindowSize = {900, 900};
constexpr uint32_t LightCount = 3;

template <typename E>
constexpr auto get(E e) -> typename std::underlying_type<E>::type {
  return static_cast<typename std::underlying_type<E>::type>(e);
}

enum class Materials : uint32_t { White, Red, Green, Blue, COUNT };

enum class Models : uint32_t {
  Cylinder,
  Cube,
  Triangle,
  IcosphereSub2,
  Pentagon,
  COUNT
};

enum class Images : uint32_t { Star, COUNT };
std::array<const char*, get(Images::COUNT)> imagePaths = {
    "content/images/star.png"};

enum class StarQuads { Full, COUNT };

enum class Queues : uint32_t { Graphics, Present, COUNT };

enum class SpecConsts : uint32_t {
  MaterialCount,
  ImageCount,
  LightCount,
  COUNT
};

enum class SetLayouts : uint32_t { Static, Dynamic, COUNT };

enum class Shaders : uint32_t {
  Vertex2D,
  Fragment2D,
  Vertex3D,
  Fragment3D,
  COUNT
};

enum class Samplers : uint32_t { Texture2D, COUNT };

enum class PushRanges : uint32_t { Primary, COUNT };

enum class UniformBuffers : uint32_t {
  Camera,
  Lights,
  InstanceStaging,
  Instance,
  COUNT
};

enum class PipelineLayouts : uint32_t { Primary, COUNT };

enum class ClearValues : uint32_t { Color, Depth, COUNT };

enum class Pipelines : uint32_t { P3D, P2D, COUNT };

struct MaterialUniform {
  glm::vec4 color;
};

struct CameraUniform {
  glm::mat4 view;
  glm::mat4 projection;
};

struct LightUniform {
  glm::vec4 position;
  glm::vec4 color;
};

struct InstanceUniform {
  glm::mat4 model;
  glm::mat4 MVP;
};

struct StagedBuffer {
  std::optional<vka::BufferData> stagingBufferData;
  vka::BufferData bufferData;
};

struct PushConstantData {
  glm::uint32 materialIndex;
  glm::uint32 textureIndex;
};

struct SpecializationData {
  using MaterialCount = glm::uint32;
  using ImageCount = glm::uint32;
  using LightCount = glm::uint32;
  MaterialCount materialCount;
  ImageCount imageCount;
  LightCount lightCount;
};

struct PipelineState {
  std::vector<VkDynamicState> dynamicStates;
  VkPipelineDynamicStateCreateInfo dynamicState;
  VkPipelineMultisampleStateCreateInfo multisampleState;
  VkPipelineViewportStateCreateInfo viewportState;
  VkSpecializationInfo specializationInfo;
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
  VkPipelineColorBlendAttachmentState colorBlendAttachmentState;
  VkPipelineColorBlendStateCreateInfo colorBlendState;
  VkPipelineDepthStencilStateCreateInfo depthStencilState;
  VkPipelineRasterizationStateCreateInfo rasterizationState;
  std::vector<VkVertexInputAttributeDescription> vertexAttributes;
  std::vector<VkVertexInputBindingDescription> vertexBindings;
  VkPipelineVertexInputStateCreateInfo vertexInputState;
};

struct VS {
  vka::LibraryHandle VulkanLibrary;

  VkInstance instance;
  VkPhysicalDevice physicalDevice;
  VkPhysicalDeviceProperties physicalDeviceProperties;
  bool unifiedMemory = false;
  VkDeviceSize uniformBufferOffsetAlignment;

  std::vector<VkQueueFamilyProperties> familyProperties;
  std::array<float, get(Queues::COUNT)> queuePriorities;
  std::array<uint32_t, get(Queues::COUNT)> queueFamilyIndices;
  std::array<VkQueue, get(Queues::COUNT)> queues;

  VkDevice device;
  vka::Allocator allocator;

  std::map<uint32_t, std::vector<vka::Quad>> imageQuads;
  std::array<vka::Image2D, get(Images::COUNT)> images;
  std::array<vka::glTF, get(Models::COUNT)> models;
  std::array<LightUniform, LightCount> lightUniforms;

  std::array<MaterialUniform, get(Materials::COUNT)> materialUniforms = {
      MaterialUniform{glm::vec4(1.f)},
      MaterialUniform{glm::vec4(1.f, 0.f, 0.f, 1.f)},
      MaterialUniform{glm::vec4(0.f, 1.f, 0.f, 1.f)},
      MaterialUniform{glm::vec4(0.f, 0.f, 1.f, 1.f)}};

  struct Utility {
    VkCommandPool pool;
    VkCommandBuffer buffer;
    VkFence fence;
  } utility;

  VkSurfaceKHR surface;
  struct SurfaceData {
    VkExtent2D extent;
    std::vector<VkSurfaceFormatKHR> supportedFormats;
    VkSurfaceFormatKHR swapFormat;
    VkFormat depthFormat;
    std::vector<VkPresentModeKHR> supportedPresentModes;
    VkPresentModeKHR presentMode;
  } surfaceData;

  VkViewport viewport;
  VkRect2D scissor;
  VkRenderPass renderPass;
  VkSwapchainKHR swapchain;
  vka::Image2D depthImage;
  SpecializationData specData;

  StagedBuffer Vertex2D;
  StagedBuffer Position3D;
  StagedBuffer Normal3D;
  StagedBuffer Index3D;

  std::vector<VkDescriptorSetLayoutBinding> staticLayoutBindings;
  std::vector<VkDescriptorSetLayoutBinding> instanceLayoutBindings;
  std::array<VkSampler, get(Samplers::COUNT)> samplers;
  std::array<VkDescriptorSetLayout, get(SetLayouts::COUNT)> setLayouts;
  std::array<VkPushConstantRange, get(PushRanges::COUNT)> pushRanges;
  std::array<VkShaderModule, get(Shaders::COUNT)> shaderModules;
  std::array<VkPipelineLayout, get(PipelineLayouts::COUNT)> pipelineLayouts;
  std::array<VkSpecializationMapEntry, get(SpecConsts::COUNT)>
      specializationMapEntries;
  std::array<PipelineState, get(Pipelines::COUNT)> pipelineStates;
  std::array<VkGraphicsPipelineCreateInfo, get(Pipelines::COUNT)>
      pipelineCreateInfos;
  std::array<VkPipeline, get(Pipelines::COUNT)> pipelines;

  uint32_t nextImage;

  std::array<VkClearValue, get(ClearValues::COUNT)> clearValues = {
      VkClearValue{0.f, 0.f, 0.f, 0.f}, VkClearValue{1.f, 0U}};

  VkDescriptorPool staticLayoutPool;
  VkDescriptorPool dynamicLayoutPool;

  StagedBuffer materialsUniformBuffer;

  VkDeviceSize materialsBufferOffsetAlignment;
  VkDeviceSize instanceBuffersOffsetAlignment;
  VkDeviceSize lightsBuffersOffsetAlignment;

  std::array<StagedBuffer, BufferCount> cameraUniformBuffers;
  std::array<StagedBuffer, BufferCount> lightsUniformBuffers;
  std::array<StagedBuffer, BufferCount> instanceUniformBuffers;
  std::array<size_t, BufferCount> instanceBufferCapacities;
  std::array<VkDescriptorSetLayout, BufferCount> staticSetLayouts;
  std::array<VkDescriptorSet, BufferCount> staticSets;
  std::array<VkDescriptorSetLayout, BufferCount> dynamicSetLayouts;
  std::array<VkDescriptorSet, BufferCount> dynamicSets;
  std::array<VkImage, BufferCount> swapImages;
  std::array<VkImageView, BufferCount> swapViews;
  std::array<VkFramebuffer, BufferCount> framebuffers;
  std::array<VkCommandPool, BufferCount> renderCommandPools;
  std::array<VkFence, BufferCount> imageReadyFences;
  Pool<VkFence> imageReadyFencePool;
  std::array<VkSemaphore, BufferCount> frameDataCopySemaphores;
  std::array<VkSemaphore, BufferCount> imageRenderSemaphores;
};