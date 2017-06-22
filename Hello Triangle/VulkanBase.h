#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <iostream>
#include <set>
#include <algorithm>
#include "VulkanBuffer.h"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

namespace vke
{
	struct QueueFamilyIndices
	{
		int graphicsFamily = -1;
		int presentFamily = -1;

		bool isComplete()
		{
			return (graphicsFamily >= 0 && presentFamily >= 0);
		}
	};

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	class Instance
	{
		friend class Device;
	public:
		void create(const std::vector<char*> validationLayers, const std::vector<char*> requiredExtensions, const std::vector<char*> deviceExtensions)
		{
			m_ValidationLayers = validationLayers;
			m_DeviceExtensions = deviceExtensions;
			m_VulkanExtensions = requiredExtensions;
			// enable debug layer(s) if in debug mode
			if (enableValidationLayers && !checkValidationLayerSupport())
			{
				throw std::runtime_error("validation layers requested, but not available!");
			}

			if (!checkExtensionSupport(m_VulkanExtensions))
			{
				throw std::runtime_error("required extension was not available.");
			}

			// Application Info Struct
			VkApplicationInfo appInfo = {};
			appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			appInfo.pApplicationName = "Hello Triangle";
			appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
			appInfo.pEngineName = "No Engine";
			appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
			appInfo.apiVersion = VK_API_VERSION_1_0;

			// Instance Create Info Struct
			VkInstanceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			createInfo.pApplicationInfo = &appInfo;
			createInfo.enabledExtensionCount = static_cast<uint32_t>(m_VulkanExtensions.size());
			createInfo.ppEnabledExtensionNames = m_VulkanExtensions.data();
			if (enableValidationLayers)
			{
				createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
				createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
			}
			else
			{
				createInfo.enabledLayerCount = 0;
			}

			if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create vulkan instance!");
			}

			setupDebugCallback(m_callback);
		}
		VkInstance getInstance() { return m_Instance; }
	private:
		VkInstance m_Instance;
		std::vector<char*> m_VulkanExtensions;
		std::vector<char*> m_DeviceExtensions;
		std::vector<char*> m_ValidationLayers;
		VkDebugReportCallbackEXT m_callback;

		bool checkValidationLayerSupport()
		{
			uint32_t layerCount;
			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

			std::vector<VkLayerProperties> availableLayers(layerCount);
			vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

			for (const char* layerName : m_ValidationLayers)
			{
				bool layerFound = false;

				for (const auto& layerProperties : availableLayers)
				{
					if (strcmp(layerName, layerProperties.layerName) == 0)
					{
						layerFound = true;
						break;
					}
				}

				if (!layerFound)
				{
					return false;
				}
			}

			return true;
		}

		// checks that all the given extensions are supported
		bool checkExtensionSupport(std::vector<char*> requiredExtensions)
		{
			// Look up available extensions
			uint32_t extensionCount = 0;
			vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
			std::vector<VkExtensionProperties> availableExtensions(extensionCount);
			vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

			// Print available extensions to output if in Debug mode
			std::cout << "available extensions:" << std::endl;
			for (const auto& extension : availableExtensions)
			{
				std::cout << "\t" << extension.extensionName << std::endl;
			}

			// check that each required extension is available
			for (const char* requiredExt : requiredExtensions)
			{
				bool extensionFound = false;

				for (auto& availableExt : availableExtensions)
				{
					if (strcmp(requiredExt, availableExt.extensionName) == 0)
					{
						extensionFound = true;
						break;
					}
				}

				if (!extensionFound)
				{
					return false;
				}
			}

			return true;
		}


		VkResult CreateDebugReportCallbackEXT(
			VkInstance instance,
			const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
			const VkAllocationCallbacks* pAllocator,
			VkDebugReportCallbackEXT* pCallback
		)
		{
			auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
			if (func != nullptr)
			{
				return func(instance, pCreateInfo, pAllocator, pCallback);
			}
			else
			{
				return VK_ERROR_EXTENSION_NOT_PRESENT;
			}
		}

		void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator)
		{
			auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
			if (func != nullptr)
			{
				func(instance, callback, pAllocator);
			}
		}

		// a callback function that displays any errors/warnings from Vulkan
		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			VkDebugReportFlagsEXT flags,
			VkDebugReportObjectTypeEXT objType,
			uint64_t obj,
			size_t location,
			int32_t code,
			const char* layerPrefix,
			const char* message,
			void* userData)
		{
			std::cerr << "validation layer: " << message << std::endl;

			return false;
		}

		// pass a reference for a callback function for errors to Vulkan
		void setupDebugCallback(VkDebugReportCallbackEXT callback)
		{
			if (!enableValidationLayers)
				return;

			VkDebugReportCallbackCreateInfoEXT createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
			createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
			createInfo.pfnCallback = debugCallback;

			if (CreateDebugReportCallbackEXT(m_Instance, &createInfo, nullptr, &callback) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to set up debug callback!");
			}
		}
	};

	class Device
	{
	public:
		operator VkDevice()
		{
			return m_Device;
		}

		void init(Instance instance, VkSurfaceKHR surface)
		{
			m_Instance = instance;
			m_Surface = surface;
			pickPhysicalDevice();
			createLogicalDevice();
			m_Allocator.init(*this);
		}

		std::unique_ptr<Buffer> createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
		{
			auto buffer = std::make_unique<Buffer>();
			VkBuffer vkBuffer;
			
			VkBufferCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = size;
			bufferInfo.usage = usage;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			if (vkCreateBuffer(m_Device, &bufferInfo, nullptr, &vkBuffer) != VK_SUCCESS) 
			{
				throw std::runtime_error("failed to create buffer!");
			}

			VkMemoryRequirements memRequirements;
			vkGetBufferMemoryRequirements(m_Device, vkBuffer, &memRequirements);
			uint32_t memType = findMemoryType(memRequirements.memoryTypeBits, properties);
			buffer->buffer = vkBuffer;
			buffer->memory = m_Allocator.allocateMemory(memType, size, memRequirements.alignment);
			buffer->alignment = memRequirements.alignment;
			buffer->size = buffer->memory->size;
			buffer->memoryType = memType;
			return buffer;
		}

		void createImage2D(uint32_t width, uint32_t height, VkFormat format,
			VkImageTiling tiling, VkImageUsageFlags usage, VkImage& image)
		{
			VkImageCreateInfo imageInfo = {};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = width;
			imageInfo.extent.height = height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.format = format;
			imageInfo.tiling = tiling;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
			imageInfo.usage = usage;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			if (vkCreateImage(m_Device, &imageInfo, nullptr, &image) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create image!");
			}
		}

		VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) 
		{
			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = image;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = format;
			viewInfo.subresourceRange.aspectMask = aspectFlags;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			VkImageView imageView;
			if (vkCreateImageView(m_Device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create texture image view!");
			}

			return imageView;
		}

		VkResult createSampler(VkSampler& textureSampler)
		{
			VkSamplerCreateInfo samplerInfo = {};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			samplerInfo.minFilter = VK_FILTER_LINEAR;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.anisotropyEnable = VK_TRUE;
			samplerInfo.maxAnisotropy = 16;
			samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			samplerInfo.unnormalizedCoordinates = VK_FALSE;
			samplerInfo.compareEnable = VK_FALSE;
			samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.mipLodBias = 0.0f;
			samplerInfo.minLod = 0.0f;
			samplerInfo.maxLod = 0.0f;

			return vkCreateSampler(m_Device, &samplerInfo, nullptr, &textureSampler);
		}

		// Get methods
		VkPhysicalDevice getPhysicalDevice() { return m_PhysicalDevice; }
		VkDevice getDevice() { return m_Device; }
		VkPhysicalDeviceProperties getPhysicalProperties() { return m_DeviceProperties; }
		QueueFamilyIndices getQueueFamilyIndices() { return m_QueueIndices; }
		VkQueue getGraphicsQueue() { return m_GraphicsQueue; }
		VkQueue getPresentQueue() { return m_PresentQueue; }

	private:
		Instance m_Instance;
		VkPhysicalDevice m_PhysicalDevice;
		VkDevice m_Device;
		VkSurfaceKHR m_Surface;
		VkPhysicalDeviceProperties m_DeviceProperties;
		VkQueue m_GraphicsQueue;
		VkQueue m_PresentQueue;
		QueueFamilyIndices m_QueueIndices;
		Allocator m_Allocator;

		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
		{
			VkPhysicalDeviceMemoryProperties memProperties;
			vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);

			for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
			{
				if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
				{
					return i;
				}
			}

			throw std::runtime_error("failed to find suitable memory type!");
		}

		// checks if the given GPU supports the extensions listed in "deviceExtensions"
		bool checkDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<char*> deviceExtensions)
		{
			uint32_t extensionCount;
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

			std::vector<VkExtensionProperties> availableExtensions(extensionCount);
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
			std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

			for (const auto& extension : availableExtensions)
			{
				requiredExtensions.erase(extension.extensionName);
			}

			return requiredExtensions.empty();
		}

		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
		{
			SwapChainSupportDetails details;

			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
			if (formatCount != 0)
			{
				details.formats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
			}

			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

			if (presentModeCount != 0)
			{
				details.presentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
			}

			return details;
		}

		// return true if the physical device supports the graphics and present queue families, 
		// and that required device extensions are supported
		bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface)
		{
			QueueFamilyIndices indices = findQueueFamilies(device, surface);

			bool swapChainAdequate = false;
			bool extensionsSupported = checkDeviceExtensionSupport(device, m_Instance.m_DeviceExtensions);
			if (extensionsSupported)
			{
				SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
				swapChainAdequate = (!swapChainSupport.formats.empty()) && (!swapChainSupport.presentModes.empty());
			}

			VkPhysicalDeviceFeatures supportedFeatures;
			vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

			return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
		}

		// looks up desired queue families and populates the QueueFamilyIndices struct with their indices.
		// this should be called after surface creation, as it depends on the surface to query for present capability
		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
		{
			// enumerate available queue families on the given device
			QueueFamilyIndices indices;

			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

			// initialize the QueueFamilyIndices struct with the index of the graphics family (only one needed for now)
			int i = 0;
			for (const auto& queueFamily : queueFamilies)
			{
				VkBool32 presentSupport = false;
				if (queueFamily.queueCount > 0)
				{
					if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
						indices.graphicsFamily = i;
					vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
					if (presentSupport)
					{
						indices.presentFamily = i;
					}
				}

				if (indices.isComplete())
				{
					break;
				}
				i++;
			}

			return indices;
		}

		void pickPhysicalDevice()
		{
			// enumerate all physical devices (GPUs)
			uint32_t deviceCount = 0;
			vkEnumeratePhysicalDevices(m_Instance.m_Instance, &deviceCount, nullptr);

			if (deviceCount == 0)
			{
				throw std::runtime_error("failed to find GPUs with Vulkan support");
			}

			std::vector<VkPhysicalDevice> devices(deviceCount);
			vkEnumeratePhysicalDevices(m_Instance.m_Instance, &deviceCount, devices.data());

			// select the first suitable GPU as the device for use
			for (const auto& device : devices)
			{
				if (isDeviceSuitable(device, m_Surface))
				{
					m_PhysicalDevice = device;
					break;
				}
			}

			// if no suitable devices are found, throw an error
			if (m_PhysicalDevice == VK_NULL_HANDLE)
			{
				throw std::runtime_error("failed to find a suitable GPU");
			}
		}

		void lookUpPhysicalProperties()
		{
			vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_DeviceProperties);
		}

		void createLogicalDevice()
		{
			// get indices of desired queue families
			QueueFamilyIndices indices = findQueueFamilies(m_PhysicalDevice, m_Surface);

			std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
			std::set<int> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

			float queuePriority = 1.0f;
			for (int queueFamily : uniqueQueueFamilies)
			{
				VkDeviceQueueCreateInfo queueCreateInfo = {};
				queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo.queueFamilyIndex = queueFamily;
				queueCreateInfo.queueCount = 1;
				queueCreateInfo.pQueuePriorities = &queuePriority;
				queueCreateInfos.push_back(queueCreateInfo);
			}

			// logical device create info struct
			VkPhysicalDeviceFeatures deviceFeatures = {};
			deviceFeatures.samplerAnisotropy = VK_TRUE;
			VkDeviceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			createInfo.pQueueCreateInfos = queueCreateInfos.data();
			createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
			createInfo.pEnabledFeatures = &deviceFeatures;
			createInfo.enabledExtensionCount = static_cast<uint32_t>(m_Instance.m_DeviceExtensions.size());
			createInfo.ppEnabledExtensionNames = m_Instance.m_DeviceExtensions.data();

			// pass in validation layer info if in debug mode
			if (enableValidationLayers)
			{
				createInfo.enabledLayerCount = static_cast<uint32_t>(m_Instance.m_ValidationLayers.size());
				createInfo.ppEnabledLayerNames = m_Instance.m_ValidationLayers.data();
			}
			else
			{
				createInfo.enabledLayerCount = 0;
			}

			// attempt to create logical device and throw error on failure
			if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create logical device!");
			}

			// Get handles to graphics and present queues
			vkGetDeviceQueue(m_Device, indices.graphicsFamily, 0, &m_GraphicsQueue);
			vkGetDeviceQueue(m_Device, indices.presentFamily, 0, &m_PresentQueue);
		}
	};

	class SwapChain
	{
	public:
		void create(std::shared_ptr<Device> device, std::shared_ptr<VkSurfaceKHR> surface, const uint32_t width, const uint32_t height)
		{
			m_Surface = surface;
			m_Device = device;
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device->getPhysicalDevice());

			auto surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
			auto presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
			auto extent = chooseSwapExtent(swapChainSupport.capabilities, width, height);

			uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
			if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
				imageCount = swapChainSupport.capabilities.maxImageCount;

			VkSwapchainCreateInfoKHR createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			createInfo.surface = *surface;
			createInfo.minImageCount = imageCount;
			createInfo.imageFormat = surfaceFormat.format;
			createInfo.imageColorSpace = surfaceFormat.colorSpace;
			createInfo.imageExtent = extent;
			createInfo.imageArrayLayers = 1;
			createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

			auto indices = device->getQueueFamilyIndices();
			uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };

			if (indices.graphicsFamily != indices.presentFamily) {
				createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
				createInfo.queueFamilyIndexCount = 2;
				createInfo.pQueueFamilyIndices = queueFamilyIndices;
			}
			else {
				createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
				createInfo.queueFamilyIndexCount = 0; // Optional
				createInfo.pQueueFamilyIndices = nullptr; // Optional
			}

			createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
			createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			createInfo.presentMode = presentMode;
			createInfo.clipped = VK_TRUE;
			createInfo.oldSwapchain = VK_NULL_HANDLE;

			if (vkCreateSwapchainKHR(device->getDevice(), &createInfo, nullptr, &m_SwapChain) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create swap chain!");
			}

			vkGetSwapchainImagesKHR(device->getDevice(), m_SwapChain, &imageCount, nullptr);
			m_SwapChainImages.resize(imageCount);
			vkGetSwapchainImagesKHR(device->getDevice(), m_SwapChain, &imageCount, m_SwapChainImages.data());

			m_SwapChainImageFormat = surfaceFormat.format;
			m_SwapChainExtent = extent;

			m_SwapChainImageViews.resize(imageCount);

			for (size_t i = 0; i < imageCount; i++)
			{
				m_SwapChainImageViews[i] = device->createImageView(m_SwapChainImages[i], m_SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
			}
		}

		void cleanup()
		{

		}
	private:
		std::shared_ptr<Device> m_Device;
		std::shared_ptr<VkSurfaceKHR> m_Surface;
		// Owned objects
		VkSwapchainKHR m_SwapChain;
		std::vector<VkImage> m_SwapChainImages;
		std::vector<VkImageView> m_SwapChainImageViews;
		VkExtent2D m_SwapChainExtent;
		VkFormat m_SwapChainImageFormat;

		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
		{
			if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
			{
				return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
			}

			for (const auto& availableFormat : availableFormats)
			{
				if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
					return availableFormat;
			}

			return availableFormats[0];
		}

		// iterate through available present modes and return the best choice available
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
		{
			// Fallback mode
			VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

			for (const auto& availablePresentMode : availablePresentModes)
			{
				// ideal mode
				if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					bestMode = availablePresentMode;
				}
				// second best mode
				else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
				{
					bestMode = availablePresentMode;
				}
			}

			return bestMode;
		}

		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, const uint32_t width, const uint32_t height)
		{
			if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
				return capabilities.currentExtent;
			}
			else {
				VkExtent2D actualExtent = { (uint32_t)width, (uint32_t)height };

				actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
				actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

				return actualExtent;
			}
		}

		// populates SwapChainSupportDetails struct with information about the swapchain support on the given GPU
		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
		{
			SwapChainSupportDetails details;

			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, *m_Surface, &details.capabilities);

			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, *m_Surface, &formatCount, nullptr);
			if (formatCount != 0)
			{
				details.formats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, *m_Surface, &formatCount, details.formats.data());
			}

			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, *m_Surface, &presentModeCount, nullptr);

			if (presentModeCount != 0)
			{
				details.presentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, *m_Surface, &presentModeCount, details.presentModes.data());
			}

			return details;
		}

	};
}