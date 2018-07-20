#pragma once

#include "vulkan/vulkan.h"
#include "vka/VulkanFunctionLoader.hpp"
#include "vka/UniqueVulkan.hpp"
#include "vka/Buffer.hpp"
#include "vka/Image2D.hpp"
#include "Pool.hpp"
#include "vka/Allocator.hpp"

#include <array>
#include <vector>

constexpr uint32_t BufferCount = 3;
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
		bool unifiedMemory = false;
		std::vector<VkQueueFamilyProperties> queueFamilyProperties;
		float graphicsQueuePriority = 0.f;
		float presentQueuePriority = 0.f;
		uint32_t graphicsQueueFamilyIndex;
		uint32_t presentQueueFamilyIndex;
		VkDevice device;
		VkQueue graphicsQueue;
		VkQueue presentQueue;
		vka::Allocator allocator;
		VkSurfaceKHR surface;
		VkExtent2D swapExtent;
		std::vector<VkSurfaceFormatKHR> supportedSurfaceFormats;
		VkSurfaceFormatKHR surfaceFormat;
		VkFormat depthFormat;
		std::vector<VkPresentModeKHR> supportedPresentModes;
		VkPresentModeKHR presentMode;
		VkSwapchainKHR swapchain;
		VkRenderPass renderPass;
		struct {
			struct {
				VkCommandPoolUnique pool;
				VkFenceUnique fence;
			} unique;
			VkCommandBuffer buffer;
			VkFence fence;
		} utility;
		vka::Image2D depthImage;

		struct {
			struct {
				struct {
					VkSamplerUnique sampler;
					VkDescriptorSetLayoutUnique staticLayout;
					VkDescriptorSetLayoutUnique frameLayout;
					VkDescriptorSetLayoutUnique materialLayout;
					VkDescriptorSetLayoutUnique drawLayout;
					VkShaderModuleUnique vertex;
					VkShaderModuleUnique fragment;
					VkPipelineLayoutUnique pipelineLayout;
					VkPipelineUnique pipeline;
			} unique;
				VkSampler sampler;
				std::vector<VkDescriptorSetLayoutBinding> staticBindings;
				std::vector<VkDescriptorSetLayoutBinding> frameBindings;
				std::vector<VkDescriptorSetLayoutBinding> materialBindings;
				std::vector<VkDescriptorSetLayoutBinding> drawBindings;
				VkDescriptorSetLayout staticLayout;
				VkDescriptorSetLayout frameLayout;
				VkDescriptorSetLayout materialLayout;
				VkDescriptorSetLayout drawLayout;
				VkShaderModule vertex;
				VkShaderModule fragment;
				VkPipelineLayout pipelineLayout;
				VkPipeline pipeline;
			} pipeline2D;
			struct {
				struct {
					VkDescriptorSetLayoutUnique materialLayout;
					VkDescriptorSetLayoutUnique frameLayout;
					VkShaderModuleUnique vertex;
					VkShaderModuleUnique fragment;
					VkPipelineLayoutUnique pipelineLayout;
					VkPipelineUnique pipeline;
				} unique;
				std::vector<VkDescriptorSetLayoutBinding> materialBindings;
				std::vector<VkDescriptorSetLayoutBinding> frameBindings;
				VkDescriptorSetLayout materialLayout;
				VkDescriptorSetLayout frameLayout;
				VkShaderModule vertex;
				VkShaderModule fragment;
				VkPipelineLayout pipelineLayout;
				VkPipeline pipeline;
			} pipeline3D;
		} pipelines;

		VkViewport viewport;
		VkRect2D scissor;
		uint32_t nextImage;
		std::array<VkFenceUnique, BufferCount> imagePresentedFences;
		Pool<VkFence> imagePresentedFencePool;
		VkFence imagePresentedFence;
		struct {
			struct {
				vka::UniqueAllocatedBuffer matrixBuffer;
				VkDescriptorPoolUnique staticDescriptorPool;
				VkDescriptorPoolUnique dynamicDescriptorPool;
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