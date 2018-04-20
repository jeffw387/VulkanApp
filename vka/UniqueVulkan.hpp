#pragma once
#include <memory>
#include "vulkan/vulkan.h"
#include "VulkanFunctions.hpp"

namespace vka
{
    struct VkInstanceDeleter
	{
		using pointer = VkInstance;
		void operator()(VkInstance instance)
		{
			vkDestroyInstance(instance, nullptr);
		}
	};
	using VkInstanceUnique = std::unique_ptr<VkInstance, VkInstanceDeleter>;

	struct VkDebugReportCallbackEXTDeleter
	{
		using pointer = VkDebugReportCallbackEXT;
		VkInstance instance;
		void operator()(VkDebugReportCallbackEXT callback)
		{
			vkDestroyDebugReportCallbackEXT(instance, callback, nullptr);
		}
	};
	using VkDebugCallbackUnique = std::unique_ptr<VkDebugReportCallbackEXT, VkDebugReportCallbackEXTDeleter>;

    struct VkDeviceDeleter
	{
		using pointer = VkDevice;
		void operator()(VkDevice device)
		{
			vkDestroyDevice(device, nullptr);
		}
	};
	using VkDeviceUnique = std::unique_ptr<VkDevice, VkDeviceDeleter>;

    struct VkDeviceMemoryDeleter
    {
        using pointer = VkDeviceMemory;
        VkDevice device;
        void operator()(VkDeviceMemory memory)
        {
            vkFreeMemory(device, memory, nullptr);
        }
    };
    using VkDeviceMemoryUnique = std::unique_ptr<VkDeviceMemory, VkDeviceMemoryDeleter>;

    struct VkBufferDeleter
    {
        using pointer = VkBuffer;
        VkDevice device;

        void operator()(VkBuffer buffer)
        {
            vkDestroyBuffer(device, buffer, nullptr);
        }
    };
    using VkBufferUnique = std::unique_ptr<VkBuffer, VkBufferDeleter>;

    struct VkImageDeleter
	{
		using pointer = VkImage;
		VkDevice device;

		void operator()(VkImage image)
		{
			vkDestroyImage(device, image, nullptr);
		}
	};
	using VkImageUnique = std::unique_ptr<VkImage, VkImageDeleter>;

	struct VkImageViewDeleter
	{
		using pointer = VkImageView;
		VkDevice device;

		void operator()(VkImageView view)
		{
			vkDestroyImageView(device, view, nullptr);
		}
	};
	using VkImageViewUnique = std::unique_ptr<VkImageView, VkImageViewDeleter>;

    struct VkFenceDeleter
    {
        using pointer = VkFence;
        VkDevice device;

        void operator()(VkFence fence)
        {
            vkDestroyFence(device, fence, nullptr);
        }
    };
    using VkFenceUnique = std::unique_ptr<VkFence, VkFenceDeleter>;

    struct VkSemaphoreDeleter
    {
        using pointer = VkSemaphore;
        VkDevice device;

        void operator()(VkSemaphore semaphore)
        {
            vkDestroySemaphore(device, semaphore, nullptr);
        }
    };
    using VkSemaphoreUnique = std::unique_ptr<VkSemaphore, VkSemaphoreDeleter>;

    struct VkCommandPoolDeleter
    {
        using pointer = VkCommandPool;
        VkDevice device;

        void operator()(VkCommandPool pool)
        {
            vkDestroyCommandPool(device, pool, nullptr);
        }
    };
    using VkCommandPoolUnique = std::unique_ptr<VkCommandPool, VkCommandPoolDeleter>;
    
    struct VkSurfaceKHRDeleter
    {
        using pointer = VkSurfaceKHR;
        VkInstance instance;

        void operator()(VkSurfaceKHR surface)
        {
            vkDestroySurfaceKHR(instance, surface, nullptr);
        }
    };
    using VkSurfaceKHRUnique = std::unique_ptr<VkSurfaceKHR, VkSurfaceKHRDeleter>;

    struct VkFramebufferDeleter
    {
        using pointer = VkFramebuffer;
        VkDevice device;

        void operator()(VkFramebuffer framebuffer)
        {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    };
    using VkFramebufferUnique = std::unique_ptr<VkFramebuffer, VkFramebufferDeleter>;

    struct VkRenderPassDeleter
    {
        using pointer = VkRenderPass;
        VkDevice device;

        void operator()(VkRenderPass renderpass)
        {
            vkDestroyRenderPass(device, renderpass, nullptr);
        }
    };
    using VkRenderPassUnique = std::unique_ptr<VkRenderPass, VkRenderPassDeleter>;

    struct VkSamplerDeleter
    {
        using pointer = VkSampler;
        VkDevice device;

        void operator()(VkSampler sampler)
        {
            vkDestroySampler(device, sampler, nullptr);
        }
    };
    using VkSamplerUnique = std::unique_ptr<VkSampler, VkSamplerDeleter>;

    struct VkShaderModuleDeleter
    {
        using pointer = VkShaderModule;
        VkDevice device;

        void operator()(VkShaderModule shader)
        {
            vkDestroyShaderModule(device, shader, nullptr);
        }
    };
    using VkShaderModuleUnique = std::unique_ptr<VkShaderModule, VkShaderModuleDeleter>;

    struct VkDescriptorSetLayoutDeleter
    {
        using pointer = VkDescriptorSetLayout;
        VkDevice device;

        void operator()(VkDescriptorSetLayout layout)
        {
            vkDestroyDescriptorSetLayout(device, layout, nullptr);
        }
    };
    using VkDescriptorSetLayoutUnique = std::unique_ptr<VkDescriptorSetLayout, VkDescriptorSetLayoutDeleter>;

    struct VkDescriptorPoolDeleter
    {
        using pointer = VkDescriptorPool;
        VkDevice device;

        void operator()(VkDescriptorPool pool)
        {
            vkDestroyDescriptorPool(device, pool, nullptr);
        }
    };
    using VkDescriptorPoolUnique = std::unique_ptr<VkDescriptorPool, VkDescriptorPoolDeleter>;

    struct VkSwapchainKHRDeleter
    {
        using pointer = VkSwapchainKHR;
        VkDevice device;

        void operator()(VkSwapchainKHR swapchain)
        {
            vkDestroySwapchainKHR(device, swapchain, nullptr);
        }
    };
    using VkSwapchainKHRUnique = std::unique_ptr<VkSwapchainKHR, VkSwapchainKHRDeleter>;

    struct VkPipelineLayoutDeleter
    {
        using pointer = VkPipelineLayout;
        VkDevice device;

        void operator()(VkPipelineLayout layout)
        {
            vkDestroyPipelineLayout(device, layout, nullptr);
        }
    };
    using VkPipelineLayoutUnique = std::unique_ptr<VkPipelineLayout, VkPipelineLayoutDeleter>;

    struct VkPipelineDeleter
    {
        using pointer = VkPipeline;
        VkDevice device;

        void operator()(VkPipeline pipeline)
        {
            vkDestroyPipeline(device, pipeline, nullptr);
        }
    };
    using VkPipelineUnique = std::unique_ptr<VkPipeline, VkPipelineDeleter>;
}