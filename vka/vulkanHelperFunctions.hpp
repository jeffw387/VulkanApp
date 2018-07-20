#pragma once

#include "vulkan/vulkan.h"
#include "UniqueVulkan.hpp"
#include "GLFW/glfw3.h"
#include "gsl.hpp"
#include "vka/Image2D.hpp"

#include <vector>

namespace vka
{
	inline auto CreateInstanceUnique(const VkInstanceCreateInfo& createInfo)
	{
		VkInstance instance;
		vkCreateInstance(&createInfo, nullptr, &instance);
		return VkInstanceUnique(instance);
	}

	inline auto GetPhysicalDevices(VkInstance instance)
	{
		std::vector<VkPhysicalDevice> physicalDevices;
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		physicalDevices.resize(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());
		return physicalDevices;
	}

	inline auto GetQueueFamilyProperties(VkPhysicalDevice physicalDevice)
	{
		std::vector<VkQueueFamilyProperties> queueFamilyProperties;
		uint32_t count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
		queueFamilyProperties.resize(count);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, queueFamilyProperties.data());
		return queueFamilyProperties;
	}

	inline auto GetMemoryProperties(VkPhysicalDevice physicalDevice)
	{
		VkPhysicalDeviceMemoryProperties properties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &properties);
		return properties;
	}

	inline auto GetProperties(VkPhysicalDevice physicalDevice)
	{
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);
		return properties;
	}

	inline auto GetSurfaceCapabilities(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
	{
		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);
		return capabilities;
	}

	inline auto GetSurfaceFormats(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
	{
		std::vector<VkSurfaceFormatKHR> surfaceFormats;
		uint32_t count = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, nullptr);
		surfaceFormats.resize(count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, surfaceFormats.data());
		return surfaceFormats;
	}

	inline auto GetPresentModes(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
	{
		std::vector<VkPresentModeKHR> supported;
		uint32_t count = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, nullptr);
		supported.resize(count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, supported.data());
		return supported;
	}

	inline auto CreateDeviceUnique(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo& createInfo)
	{
		VkDevice device;
		vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
		return VkDeviceUnique(device);
	}

	inline auto CreateImage2DUnique(
		VkDevice device,
		VkImageUsageFlags usage,
		VkFormat format,
		uint32_t width,
		uint32_t height,
		uint32_t graphicsQueueFamilyIndex,
		VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		VkImageCreateFlags flags = 0)
	{
		auto createInfo = vka::imageCreateInfo();
		createInfo.imageType = VK_IMAGE_TYPE_2D;
		createInfo.format = format;
		createInfo.extent = { width, height, 1 };
		createInfo.mipLevels = 1;
		createInfo.arrayLayers = 1;
		createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		createInfo.usage = usage;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 1;
		createInfo.pQueueFamilyIndices = &graphicsQueueFamilyIndex;
		createInfo.initialLayout = initialLayout;

		VkImage image;
		vkCreateImage(device, &createInfo, nullptr, &image);
		return VkImageUnique(image, VkImageDeleter(device));
	}

	inline auto CreateImageView2DUnique(
		VkDevice device,
		VkImage image,
		VkFormat format,
		VkImageAspectFlags aspects)
	{
		auto createInfo = vka::imageViewCreateInfo();
		createInfo.image = image;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = format;
		createInfo.components = {
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY
		};
		createInfo.subresourceRange = {
			aspects,
			0,
			1,
			0,
			1
		};

		VkImageView view;
		vkCreateImageView(
			device,
			&createInfo,
			nullptr,
			&view);
		return VkImageViewUnique(view, VkImageViewDeleter(device));
	}

	inline auto AllocateImage2D(
		VkDevice device,
		Allocator& allocator,
		VkImage image)
	{
		auto handle = allocator.AllocateForImage(
			true, 
			image, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		vkBindImageMemory(
			device,
			image,
			handle.get().memory,
			handle.get().offsetInDeviceMemory);
		return std::move(handle);
	}

	inline auto CreateImage2DExtended(
		VkDevice device,
		Allocator& allocator,
		VkImageUsageFlags usage,
		VkFormat format,
		uint32_t width,
		uint32_t height,
		uint32_t graphicsQueueFamilyIndex,
		VkImageAspectFlags aspects,
		VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		VkImageCreateFlags flags = 0)
	{
		vka::Image2D image{};
		image.imageUnique = CreateImage2DUnique(
			device, usage,
			format,
			width,
			height,
			graphicsQueueFamilyIndex,
			initialLayout,
			flags);
		image.image = image.imageUnique.get();

		image.viewUnique = CreateImageView2DUnique(
			device,
			image.image,
			format,
			aspects);
		image.view = image.viewUnique.get();
		image.allocation = AllocateImage2D(
			device,
			allocator,
			image.image);
		image.currentLayout = initialLayout;
		image.format = format;
		image.height = height;
		image.width = width;
		image.usage = usage;
		return std::move(image);
	}

	inline auto CreateSurfaceUnique(VkInstance instance, GLFWwindow* window)
	{
		VkSurfaceKHR surface;
		glfwCreateWindowSurface(instance, window, nullptr, &surface);
		return VkSurfaceKHRUnique(surface, VkSurfaceKHRDeleter(instance));
	}

	inline auto CreateRenderPassUnique(VkDevice device, const VkRenderPassCreateInfo& createInfo)
	{
		VkRenderPass renderPass;
		vkCreateRenderPass(device, &createInfo, nullptr, &renderPass);
		return VkRenderPassUnique(renderPass, VkRenderPassDeleter(device));
	}

	inline auto CreateSwapchainUnique(VkDevice device, const VkSwapchainCreateInfoKHR& createInfo)
	{
		VkSwapchainKHR swapchain;
		vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain);
		return VkSwapchainKHRUnique(swapchain, VkSwapchainKHRDeleter(device));
	}

	inline auto GetSwapImages(VkDevice device, VkSwapchainKHR swapchain)
	{
		std::vector<VkImage> swapImages;
		uint32_t count = 0;
		vkGetSwapchainImagesKHR(device, swapchain, &count, nullptr);
		swapImages.resize(count);
		vkGetSwapchainImagesKHR(device, swapchain, &count, swapImages.data());
		return swapImages;
	}

	inline auto CreateDescriptorSetLayoutUnique(VkDevice device, const VkDescriptorSetLayoutCreateInfo& createInfo)
	{
		VkDescriptorSetLayout layout;
		vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &layout);
		return VkDescriptorSetLayoutUnique(layout, VkDescriptorSetLayoutDeleter(device));
	}

	inline auto CreateDescriptorPoolUnique(VkDevice device, const VkDescriptorPoolCreateInfo& createInfo)
	{
		VkDescriptorPool pool;
		vkCreateDescriptorPool(device, &createInfo, nullptr, &pool);
		return VkDescriptorPoolUnique(pool, VkDescriptorPoolDeleter(device));
	}

	inline auto AllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo& allocateInfo)
	{
		std::vector<VkDescriptorSet> sets;
		sets.resize(allocateInfo.descriptorSetCount);
		vkAllocateDescriptorSets(device, &allocateInfo, sets.data());
		return sets;
	}

	inline auto CreateCommandPoolUnique(VkDevice device, const VkCommandPoolCreateInfo& createInfo)
	{
		VkCommandPool pool;
		vkCreateCommandPool(device, &createInfo, nullptr, &pool);
		return VkCommandPoolUnique(pool, VkCommandPoolDeleter(device));
	}

	inline auto AllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo& allocateInfo)
	{
		std::vector<VkCommandBuffer> commandBuffers;
		commandBuffers.resize(allocateInfo.commandBufferCount);
		vkAllocateCommandBuffers(device, &allocateInfo, commandBuffers.data());
		return commandBuffers;
	}

	inline auto CreatePipelineLayoutUnique(VkDevice device, const VkPipelineLayoutCreateInfo& createInfo)
	{
		VkPipelineLayout layout;
		vkCreatePipelineLayout(device, &createInfo, nullptr, &layout);
		return VkPipelineLayoutUnique(layout, VkPipelineLayoutDeleter(device));
	}

	inline auto CreateShaderModuleUnique(VkDevice device, const VkShaderModuleCreateInfo& createInfo)
	{
		VkShaderModule shader;
		vkCreateShaderModule(device, &createInfo, nullptr, &shader);
		return VkShaderModuleUnique(shader, VkShaderModuleDeleter(device));
	}

	inline auto CreateGraphicsPipelinesUnique(VkDevice device, gsl::span<VkGraphicsPipelineCreateInfo> createInfos)
	{
		std::vector<VkPipeline> pipelines;
		pipelines.resize(createInfos.size());
		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, gsl::narrow_cast<uint32_t>(createInfos.size()), createInfos.data(), nullptr, pipelines.data());
		std::vector<VkPipelineUnique> pipelinesUnique;
		for (const auto& pipeline : pipelines)
		{
			pipelinesUnique.push_back(VkPipelineUnique(pipeline, VkPipelineDeleter(device)));
		}
		return pipelinesUnique;
	}

	inline auto CreateSamplerUnique(VkDevice device, const VkSamplerCreateInfo& createInfo)
	{
		VkSampler sampler;
		vkCreateSampler(device, &createInfo, nullptr, &sampler);
		return VkSamplerUnique(sampler, VkSamplerDeleter(device));
	}

	inline auto CreateBufferUnique(VkDevice device, const VkBufferCreateInfo& createInfo)
	{
		VkBuffer buffer;
		vkCreateBuffer(device, &createInfo, nullptr, &buffer);
		return VkBufferUnique(buffer, VkBufferDeleter(device));
	}

	inline auto CreateBufferViewUnique(VkDevice device, const VkBufferViewCreateInfo& createInfo)
	{
		VkBufferView view;
		vkCreateBufferView(device, &createInfo, nullptr, &view);
	}

	inline auto CreateImageUnique(VkDevice device, const VkImageCreateInfo& createInfo)
	{
		VkImage image;
		vkCreateImage(device, &createInfo, nullptr, &image);
		return VkImageUnique(image, VkImageDeleter(device));
	}

	inline auto CreateImageView(VkDevice device, const VkImageViewCreateInfo& createInfo)
	{
		VkImageView view;
		vkCreateImageView(device, &createInfo, nullptr, &view);
		return VkImageViewUnique(view, VkImageViewDeleter(device));
	}

	inline auto CreateFramebufferUnique(VkDevice device, const VkFramebufferCreateInfo& createInfo)
	{
		VkFramebuffer framebuffer;
		vkCreateFramebuffer(device, &createInfo, nullptr, &framebuffer);
		return VkFramebufferUnique(framebuffer, VkFramebufferDeleter(device));
	}

	inline auto CreateSemaphoreUnique(VkDevice device, const VkSemaphoreCreateInfo& createInfo)
	{
		VkSemaphore semaphore;
		vkCreateSemaphore(device, &createInfo, nullptr, &semaphore);
		return VkSemaphoreUnique(semaphore, VkSemaphoreDeleter(device));
	}

	inline auto CreateFenceUnique(VkDevice device, const VkFenceCreateInfo& createInfo)
	{
		VkFence fence;
		vkCreateFence(device, &createInfo, nullptr, &fence);
		return VkFenceUnique(fence, VkFenceDeleter(device));
	}
}