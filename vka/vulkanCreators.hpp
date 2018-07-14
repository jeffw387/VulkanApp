#pragma once

#include "vulkan/vulkan.h"
#include "UniqueVulkan.hpp"

#include <vector>

namespace vka
{
	auto CreateInstanceUnique(VkInstanceCreateInfo createInfo)
	{
		VkInstance instance;
		vkCreateInstance(&createInfo, nullptr, &instance);
		return VkInstanceUnique(instance);
	}

	auto CreateDeviceUnique(VkPhysicalDevice physicalDevice, VkDeviceCreateInfo createInfo)
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

	auto CreateRenderPassUnique(VkDevice device, VkRenderPassCreateInfo createInfo)
	{
		VkRenderPass renderPass;
		vkCreateRenderPass(device, &createInfo, nullptr, &renderPass);
		return VkRenderPassUnique(renderPass, VkRenderPassDeleter(device));
	}

	auto CreateSwapchainUnique(VkDevice device, VkSwapchainCreateInfoKHR createInfo)
	{
		VkSwapchainKHR swapchain;
		vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain);
		return VkSwapchainKHRUnique(swapchain, VkSwapchainKHRDeleter(device));
	}

	auto CreateDescriptorSetLayoutUnique(VkDevice device, VkDescriptorSetLayoutCreateInfo createInfo)
	{
		VkDescriptorSetLayout layout;
		vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &layout);
		return VkDescriptorSetLayoutUnique(layout, VkDescriptorSetLayoutDeleter(device));
	}

	auto CreateDescriptorPoolUnique(VkDevice device, VkDescriptorPoolCreateInfo createInfo)
	{
		VkDescriptorPool pool;
		vkCreateDescriptorPool(device, &createInfo, nullptr, &pool);
		return VkDescriptorPoolUnique(pool, VkDescriptorPoolDeleter(device));
	}

	auto AllocateDescriptorSets(VkDevice device, VkDescriptorSetAllocateInfo allocateInfo)
	{
		std::vector<VkDescriptorSet> sets;
		sets.resize(allocateInfo.descriptorSetCount);
		vkAllocateDescriptorSets(device, &allocateInfo, sets.data());
		return sets;
	}

	auto CreateCommandPoolUnique(VkDevice device, VkCommandPoolCreateInfo createInfo)
	{
		VkCommandPool pool;
		vkCreateCommandPool(device, &createInfo, nullptr, &pool);
		return VkCommandPoolUnique(pool, VkCommandPoolDeleter(device));
	}

	auto AllocateCommandBuffers(VkDevice device, VkCommandBufferAllocateInfo allocateInfo)
	{
		std::vector<VkCommandBuffer> commandBuffers;
		commandBuffers.resize(allocateInfo.commandBufferCount);
		vkAllocateCommandBuffers(device, &allocateInfo, commandBuffers.data());
		return commandBuffers;
	}

	auto CreatePipelineLayoutUnique(VkDevice device, VkPipelineLayoutCreateInfo createInfo)
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

	auto CreateSamplerUnique(VkDevice device, VkSamplerCreateInfo createInfo)
	{
		VkSampler sampler;
		vkCreateSampler(device, &createInfo, nullptr, &sampler);
		return VkSamplerUnique(sampler, VkSamplerDeleter(device));
	}

	auto CreateBufferUnique(VkDevice device, VkBufferCreateInfo createInfo)
	{
		VkBuffer buffer;
		vkCreateBuffer(device, &createInfo, nullptr, &buffer);
		return VkBufferUnique(buffer, VkBufferDeleter(device));
	}

	auto CreateBufferViewUnique(VkDevice device, VkBufferViewCreateInfo createInfo)
	{
		VkBufferView view;
		vkCreateBufferView(device, &createInfo, nullptr, &view);
	}

	auto CreateImageUnique(VkDevice device, VkImageCreateInfo createInfo)
	{
		VkImage image;
		vkCreateImage(device, &createInfo, nullptr, &image);
		return VkImageUnique(image, VkImageDeleter(device));
	}

	auto CreateImageView(VkDevice device, VkImageViewCreateInfo createInfo)
	{
		VkImageView view;
		vkCreateImageView(device, &createInfo, nullptr, &view);
		return VkImageViewUnique(view, VkImageViewDeleter(device));
	}

	auto CreateFramebufferUnique(VkDevice device, VkFramebufferCreateInfo createInfo)
	{
		VkFramebuffer framebuffer;
		vkCreateFramebuffer(device, &createInfo, nullptr, &framebuffer);
		return VkFramebufferUnique(framebuffer, VkFramebufferDeleter(device));
	}

	auto CreateSemaphoreUnique(VkDevice device, VkSemaphoreCreateInfo createInfo)
	{
		VkSemaphore semaphore;
		vkCreateSemaphore(device, &createInfo, nullptr, &semaphore);
		return VkSemaphoreUnique(semaphore, VkSemaphoreDeleter(device));
	}

	auto CreateFenceUnique(VkDevice device, VkFenceCreateInfo createInfo)
	{
		VkFence fence;
		vkCreateFence(device, &createInfo, nullptr, &fence);
		return VkFenceUnique(fence, VkFenceDeleter(device));
	}
}