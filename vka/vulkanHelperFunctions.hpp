#pragma once

#include "vulkan/vulkan.h"
#include "UniqueVulkan.hpp"
#include "GLFW/glfw3.h"
#include "gsl.hpp"

#include <vector>

namespace vka
{
	auto CreateInstanceUnique(const VkInstanceCreateInfo& createInfo)
	{
		VkInstance instance;
		vkCreateInstance(&createInfo, nullptr, &instance);
		return VkInstanceUnique(instance);
	}

	auto GetPhysicalDevices(VkInstance instance)
	{
		std::vector<VkPhysicalDevice> physicalDevices;
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		physicalDevices.resize(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());
		return physicalDevices;
	}

	auto GetQueueFamilyProperties(VkPhysicalDevice physicalDevice)
	{
		std::vector<VkQueueFamilyProperties> queueFamilyProperties;
		uint32_t count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
		queueFamilyProperties.resize(count);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, queueFamilyProperties.data());
		return queueFamilyProperties;
	}

	auto GetMemoryProperties(VkPhysicalDevice physicalDevice)
	{
		VkPhysicalDeviceMemoryProperties properties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &properties);
		return properties;
	}

	auto GetProperties(VkPhysicalDevice physicalDevice)
	{
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);
		return properties;
	}

	auto GetSurfaceFormats(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
	{
		std::vector<VkSurfaceFormatKHR> surfaceFormats;
		uint32_t count = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, nullptr);
		surfaceFormats.resize(count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, surfaceFormats.data());
		return surfaceFormats;
	}

	auto GetPresentModes(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
	{
		std::vector<VkPresentModeKHR> supported;
		uint32_t count = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, nullptr);
		supported.resize(count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, supported.data());
		return supported;
	}

	auto CreateDeviceUnique(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo& createInfo)
	{
		VkDevice device;
		vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
		return VkDeviceUnique(device);
	}

	auto CreateSurfaceUnique(VkInstance instance, GLFWwindow* window)
	{
		VkSurfaceKHR surface;
		glfwCreateWindowSurface(instance, window, nullptr, &surface);
		return VkSurfaceKHRUnique(surface, VkSurfaceKHRDeleter(instance));
	}

	auto CreateRenderPassUnique(VkDevice device, const VkRenderPassCreateInfo& createInfo)
	{
		VkRenderPass renderPass;
		vkCreateRenderPass(device, &createInfo, nullptr, &renderPass);
		return VkRenderPassUnique(renderPass, VkRenderPassDeleter(device));
	}

	auto CreateSwapchainUnique(VkDevice device, const VkSwapchainCreateInfoKHR& createInfo)
	{
		VkSwapchainKHR swapchain;
		vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain);
		return VkSwapchainKHRUnique(swapchain, VkSwapchainKHRDeleter(device));
	}

	auto CreateDescriptorSetLayoutUnique(VkDevice device, const VkDescriptorSetLayoutCreateInfo& createInfo)
	{
		VkDescriptorSetLayout layout;
		vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &layout);
		return VkDescriptorSetLayoutUnique(layout, VkDescriptorSetLayoutDeleter(device));
	}

	auto CreateDescriptorPoolUnique(VkDevice device, const VkDescriptorPoolCreateInfo& createInfo)
	{
		VkDescriptorPool pool;
		vkCreateDescriptorPool(device, &createInfo, nullptr, &pool);
		return VkDescriptorPoolUnique(pool, VkDescriptorPoolDeleter(device));
	}

	auto AllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo& allocateInfo)
	{
		std::vector<VkDescriptorSet> sets;
		sets.resize(allocateInfo.descriptorSetCount);
		vkAllocateDescriptorSets(device, &allocateInfo, sets.data());
		return sets;
	}

	auto CreateCommandPoolUnique(VkDevice device, const VkCommandPoolCreateInfo& createInfo)
	{
		VkCommandPool pool;
		vkCreateCommandPool(device, &createInfo, nullptr, &pool);
		return VkCommandPoolUnique(pool, VkCommandPoolDeleter(device));
	}

	auto AllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo& allocateInfo)
	{
		std::vector<VkCommandBuffer> commandBuffers;
		commandBuffers.resize(allocateInfo.commandBufferCount);
		vkAllocateCommandBuffers(device, &allocateInfo, commandBuffers.data());
		return commandBuffers;
	}

	auto CreatePipelineLayoutUnique(VkDevice device, const VkPipelineLayoutCreateInfo& createInfo)
	{
		VkPipelineLayout layout;
		vkCreatePipelineLayout(device, &createInfo, nullptr, &layout);
		return VkPipelineLayoutUnique(layout, VkPipelineLayoutDeleter(device));
	}

	auto CreateGraphicsPipelinesUnique(VkDevice device, gsl::span<VkGraphicsPipelineCreateInfo> createInfos)
	{
		std::vector<VkPipeline> pipelines;
		pipelines.resize(createInfos.size());
		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, gsl::narrow_cast<uint32_t>(createInfos.size()), createInfos.data(), nullptr, pipelines.data());
		std::vector<VkPipelineUnique> pipelinesUnique;
		for (const auto& pipeline : pipelines)
		{
			pipelinesUnique.push_back(VkPipelineUnique(pipeline, VkPipelineDeleter(device)));
		}
		return std::move(pipelines);
	}

	auto CreateSamplerUnique(VkDevice device, const VkSamplerCreateInfo& createInfo)
	{
		VkSampler sampler;
		vkCreateSampler(device, &createInfo, nullptr, &sampler);
		return VkSamplerUnique(sampler, VkSamplerDeleter(device));
	}

	auto CreateBufferUnique(VkDevice device, const VkBufferCreateInfo& createInfo)
	{
		VkBuffer buffer;
		vkCreateBuffer(device, &createInfo, nullptr, &buffer);
		return VkBufferUnique(buffer, VkBufferDeleter(device));
	}

	auto CreateBufferViewUnique(VkDevice device, const VkBufferViewCreateInfo& createInfo)
	{
		VkBufferView view;
		vkCreateBufferView(device, &createInfo, nullptr, &view);
	}

	auto CreateImageUnique(VkDevice device, const VkImageCreateInfo& createInfo)
	{
		VkImage image;
		vkCreateImage(device, &createInfo, nullptr, &image);
		return VkImageUnique(image, VkImageDeleter(device));
	}

	auto CreateImageView(VkDevice device, const VkImageViewCreateInfo& createInfo)
	{
		VkImageView view;
		vkCreateImageView(device, &createInfo, nullptr, &view);
		return VkImageViewUnique(view, VkImageViewDeleter(device));
	}

	auto CreateFramebufferUnique(VkDevice device, const VkFramebufferCreateInfo& createInfo)
	{
		VkFramebuffer framebuffer;
		vkCreateFramebuffer(device, &createInfo, nullptr, &framebuffer);
		return VkFramebufferUnique(framebuffer, VkFramebufferDeleter(device));
	}

	auto CreateSemaphoreUnique(VkDevice device, const VkSemaphoreCreateInfo& createInfo)
	{
		VkSemaphore semaphore;
		vkCreateSemaphore(device, &createInfo, nullptr, &semaphore);
		return VkSemaphoreUnique(semaphore, VkSemaphoreDeleter(device));
	}

	auto CreateFenceUnique(VkDevice device, const VkFenceCreateInfo& createInfo)
	{
		VkFence fence;
		vkCreateFence(device, &createInfo, nullptr, &fence);
		return VkFenceUnique(fence, VkFenceDeleter(device));
	}
}