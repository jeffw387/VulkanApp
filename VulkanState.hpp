#pragma once

#include "vulkan/vulkan.h"
#include "vka/VulkanFunctionLoader.hpp"
#include "vka/UniqueVulkan.hpp"

#include <array>
#include <vector>

constexpr size_t BufferCount = 3;

namespace vka
{
	struct VS
	{
		LibraryHandle VulkanLibrary;

		struct {
			VkInstanceUnique instance;
			VkDeviceUnique device;
			VkSurfaceKHRUnique surface;
			VkSwapchainKHRUnique swapchain;
			VkRenderPassUnique renderPass;
		} unique;

		VkInstance instance;
		VkPhysicalDevice physicalDevice;
		std::vector<VkQueueFamilyProperties> queueFamilyProperties;
		float graphicsQueuePriority = 0.f;
		float presentQueuePriority = 0.f;
		uint32_t graphicsQueueFamilyIndex;
		uint32_t presentQueueFamilyIndex;
		VkDevice device;
		VkQueue graphicsQueue;
		VkQueue presentQueue;
		VkSurfaceKHR surface;
		std::vector<VkSurfaceFormatKHR> supportedSurfaceFormats;
		VkSurfaceFormatKHR surfaceFormat;
		VkFormat depthFormat;
		std::vector<VkPresentModeKHR> supportedPresentModes;
		VkPresentModeKHR presentMode;
		VkSwapchainKHR swapchain;
		VkRenderPass renderPass;

		struct {
			struct {
				struct {
					VkSamplerUnique sampler;
					VkDescriptorSetLayoutUnique staticLayout;
					VkDescriptorSetLayoutUnique frameLayout;
					VkDescriptorSetLayoutUnique modelLayout;
					VkDescriptorSetLayoutUnique drawLayout;
					VkShaderModuleUnique vertex;
					VkShaderModuleUnique fragment;
					VkPipelineLayoutUnique pipelineLayout;
					VkPipelineUnique pipeline;
			} unique;
				VkSampler sampler;
				std::vector<VkDescriptorSetLayoutBinding> staticBindings;
				std::vector<VkDescriptorSetLayoutBinding> frameBindings;
				std::vector<VkDescriptorSetLayoutBinding> modelBindings;
				std::vector<VkDescriptorSetLayoutBinding> drawBindings;
				VkDescriptorSetLayout staticLayout;
				VkDescriptorSetLayout frameLayout;
				VkDescriptorSetLayout modelLayout;
				VkDescriptorSetLayout drawLayout;
				VkShaderModule vertex;
				VkShaderModule fragment;
				VkPipelineLayout pipelineLayout;
				VkPipeline pipeline;
			} pipeline2D;
			struct {
				struct {
					VkDescriptorSetLayoutUnique staticLayout;
					VkDescriptorSetLayoutUnique frameLayout;
					VkDescriptorSetLayoutUnique modelLayout;
					VkDescriptorSetLayoutUnique drawLayout;
					VkShaderModuleUnique vertex;
					VkShaderModuleUnique fragment;
					VkPipelineLayoutUnique pipelineLayout;
					VkPipelineUnique pipeline;
				} unique;
				std::vector<VkDescriptorSetLayoutBinding> staticBindings;
				std::vector<VkDescriptorSetLayoutBinding> frameBindings;
				std::vector<VkDescriptorSetLayoutBinding> modelBindings;
				std::vector<VkDescriptorSetLayoutBinding> drawBindings;
				VkDescriptorSetLayout staticLayout;
				VkDescriptorSetLayout frameLayout;
				VkDescriptorSetLayout modelLayout;
				VkDescriptorSetLayout drawLayout;
				VkShaderModule vertex;
				VkShaderModule fragment;
				VkPipelineLayout pipelineLayout;
				VkPipeline pipeline;
			} pipeline3D;
		} pipelines;

		uint32_t nextImage;
		std::array<VkFenceUnique, BufferCount> imagePresentedFences;
		Pool<VkFence> imagePresentedFencePool;
		VkFence imagePresentedFence;
		struct {
			struct {
				vka::UniqueAllocatedBuffer matrixBuffer;
				VkDescriptorPoolUnique staticDescriptorPool;
				VkDescriptorPoolUnique dynamicDescriptorPool;
				VkImage swapImage;
				VkImageViewUnique swapView;
				VkFramebufferUnique framebuffer;
				VkCommandBuffer renderCommandBuffer;
				VkSemaphoreUnique imageRenderedSemaphore;
		} unique;
			VkBuffer matrixBuffer;
			VkDescriptorPool staticDescriptorPool;
			VkDescriptorPool dynamicDescriptorPool;
			VkImage swapImage;
			VkImageView swapView;
			VkFramebuffer framebuffer;
			VkCommandBuffer renderCommandBuffer;
			VkSemaphore imageRenderedSemaphore;
			VkFence renderCommandBufferExecutedFence;
		} frameData[BufferCount];
	};
}