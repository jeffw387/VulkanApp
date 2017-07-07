#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <iostream>
#include <set>
#include <array>
#include <algorithm>
#include <memory>
#include "BlockAllocator.h"
#include "fileIO.h"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

namespace vke
{
	class Resource
	{
	public:
		~Resource()
		{
			cleanup();
		}

		virtual void cleanup() {}
	};

	class PhysicalDevice;

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

	class Instance : public Resource
	{
		friend class PhysicalDevice;
		friend class LogicalDevice;
	public:
		Instance() : m_Instance(VK_NULL_HANDLE)
		{
		}

		void init(std::vector<char*> validationLayers, std::vector<char*> requiredExtensions, std::vector<char*> deviceExtensions)
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

		void cleanup()
		{
			if (m_Instance != VK_NULL_HANDLE)
			{
				vkDestroyInstance(m_Instance, nullptr);
				m_Instance = VK_NULL_HANDLE;
			}
		}

		operator VkInstance() { return m_Instance; }

		operator VkInstance*() { return &m_Instance; }

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

	class Surface : public Resource
	{
	public:
		void init(VkSurfaceKHR surface, VkInstance instance)
		{
			m_Surface = surface;
			m_Instance = instance;
		}

		void cleanup()
		{
			vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
			m_Surface = VK_NULL_HANDLE;
		}

		operator VkSurfaceKHR() { return m_Surface; }

	private:
		VkSurfaceKHR m_Surface;
		VkInstance m_Instance;
	};

	class Queue
	{
	public:
		uint32_t index;

		void init(VkDevice device, uint32_t queueFamilyIndex)
		{
			index = queueFamilyIndex;
			vkGetDeviceQueue(device, queueFamilyIndex, 0, &m_Queue);
		}

		operator VkQueue() { return m_Queue; }

	private:
		VkQueue m_Queue;
	};

	class CommandPool : public Resource
	{
	public:
		CommandPool() : m_Device(VK_NULL_HANDLE), m_Pool(VK_NULL_HANDLE) {}

		void init(VkDevice device, Queue queue, VkCommandPoolCreateFlags flags)
		{
			m_Device = device;
			m_Queue = queue;
			VkCommandPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.queueFamilyIndex = queue.index;
			poolInfo.flags = flags;

			if (vkCreateCommandPool(device, &poolInfo, nullptr, &m_Pool) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create command pool!");
			}
		}

		void cleanup()
		{
			vkDestroyCommandPool(m_Device, m_Pool, nullptr);
		}

		Queue getQueue() { return m_Queue; }
		operator VkCommandPool() { return m_Pool; }

	private:
		VkDevice m_Device;
		VkCommandPool m_Pool;
		Queue m_Queue;
	};

	class CommandBufferSingle : public Resource
	{
	public:
		CommandBufferSingle() : m_Device(VK_NULL_HANDLE), m_Pool(VK_NULL_HANDLE) {}

		void init(VkDevice device, VkCommandPool pool)
		{
			m_Device = device;
			m_Pool = pool;

			VkCommandBufferAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandPool = pool;
			allocInfo.commandBufferCount = 1;

			vkAllocateCommandBuffers(device, &allocInfo, &*m_Buffer);

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			vkBeginCommandBuffer(*m_Buffer, &beginInfo);
		}

		void execute(VkQueue queue)
		{
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &*m_Buffer;

			vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
			vkQueueWaitIdle(queue);
		}

		void cleanup()
		{
			if (m_Pool != VK_NULL_HANDLE)
				vkFreeCommandBuffers(m_Device, m_Pool, 1, &*m_Buffer);
		}

		operator VkCommandBuffer() { return *m_Buffer; }

	private:
		VkDevice m_Device;
		VkCommandPool m_Pool;
		std::unique_ptr<VkCommandBuffer> m_Buffer;
	};

	class PhysicalDevice
	{
		friend class Image2D;
		friend class Buffer;
	public:
		PhysicalDevice() : m_PhysicalDevice(VK_NULL_HANDLE) {}

		void init(Instance& instance, VkSurfaceKHR surface)
		{
			pickPhysicalDevice(instance, surface);
		}

		operator VkPhysicalDevice()
		{
			return m_PhysicalDevice;
		}

		// Get methods
		auto getPhysicalProperties() { return m_DeviceProperties; }
		auto getGraphicsQueueIndex() { return m_GraphicsQueueIndex; }
		auto getPresentQueueIndex() { return m_PresentQueueIndex; }

	private:
		VkPhysicalDevice m_PhysicalDevice;
		VkPhysicalDeviceProperties m_DeviceProperties;
		uint32_t m_GraphicsQueueIndex;
		uint32_t m_PresentQueueIndex;

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

		int32_t findGraphicsQueue(std::vector<VkQueueFamilyProperties>& queueFamilyProperties)
		{
			// find graphics queue index
			int32_t graphicsIndex = 0;
			for (const auto& family : queueFamilyProperties)
			{
				if (family.queueFlags | VK_QUEUE_GRAPHICS_BIT)
				{
					return graphicsIndex;
				}
				graphicsIndex++;
			}
			return -1;
		}

		int32_t findPresentQueue(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, std::vector<VkQueueFamilyProperties>& queueFamilyProperties)
		{
			// find present queue index
			VkBool32 surfaceSupport = false;
			int32_t presentIndex = 0;
			for (const auto& family : queueFamilyProperties)
			{
				vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, presentIndex, surface, &surfaceSupport);
				if (surfaceSupport)
				{
					return presentIndex;
				}
				presentIndex++;
			}
			return -1;
		}

		std::vector<VkQueueFamilyProperties> getQueueFamilyProperties(VkPhysicalDevice physicalDevice)
		{
			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

			auto queueFamilyProperties = std::vector<VkQueueFamilyProperties>(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());
			
			return queueFamilyProperties;
		}

		bool isDeviceSuitable(Instance& instance, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
		{
			auto queueFamilyProperties = getQueueFamilyProperties(physicalDevice);

			auto graphicsQueueIndex = findGraphicsQueue(queueFamilyProperties);
			auto presentQueueIndex = findPresentQueue(physicalDevice, surface, queueFamilyProperties);

			bool swapChainAdequate = false;
			bool extensionsSupported = checkDeviceExtensionSupport(physicalDevice, instance.m_DeviceExtensions);
			if (extensionsSupported)
			{
				SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);
				swapChainAdequate = (!swapChainSupport.formats.empty()) && (!swapChainSupport.presentModes.empty());
			}

			VkPhysicalDeviceFeatures supportedFeatures;
			vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

			return graphicsQueueIndex >= 0 && presentQueueIndex >= 0 && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
		}

		void pickPhysicalDevice(Instance& instance, VkSurfaceKHR surface)
		{
			// enumerate all physical devices (GPUs)
			uint32_t deviceCount = 0;
			vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

			if (deviceCount == 0)
			{
				throw std::runtime_error("failed to find GPUs with Vulkan support");
			}

			std::vector<VkPhysicalDevice> devices(deviceCount);
			vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

			// select the first suitable GPU as the device for use
			for (const auto& device : devices)
			{
				if (isDeviceSuitable(instance, device, surface))
				{
					m_PhysicalDevice = device;

					auto queueFamilyProps = getQueueFamilyProperties(device);
					m_GraphicsQueueIndex = findGraphicsQueue(queueFamilyProps);
					m_PresentQueueIndex = findPresentQueue(device, surface, queueFamilyProps);
					return;
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

	};

	class LogicalDevice : public Resource
	{
		friend class Buffer;
		friend class Image2D;
	public:
		operator VkDevice() { return m_Device; }

		void init(VkInstance instance, PhysicalDevice physicalDevice, std::set<int> queueFamilyIndices)
		{
			m_PhysicalDevice = physicalDevice;
			createLogicalDevice(queueFamilyIndices);
			m_Allocator.init(*this);

			m_GraphicsQueue.init(m_Device, physicalDevice.getGraphicsQueueIndex());
			m_PresentQueue.init(m_Device, physicalDevice.getPresentQueueIndex());
		}

		void cleanup()
		{
			if (m_Device != VK_NULL_HANDLE)
			{
				vkDestroyDevice(m_Device, nullptr);
				m_Device = VK_NULL_HANDLE;
			}
		}

		auto getGraphicsQueue() { return m_GraphicsQueue; }
		auto getPresentQueue() { return m_PresentQueue; }

	private:
		std::shared_ptr<Instance> m_Instance;
		VkPhysicalDevice m_PhysicalDevice;
		VkDevice m_Device;
		Allocator m_Allocator;
		Queue m_GraphicsQueue;
		Queue m_PresentQueue;

		void createLogicalDevice(std::set<int> uniqueQueueFamilies)
		{
			std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

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
			createInfo.enabledExtensionCount = static_cast<uint32_t>(m_Instance->m_DeviceExtensions.size());
			createInfo.ppEnabledExtensionNames = m_Instance->m_DeviceExtensions.data();

			// pass in validation layer info if in debug mode
			if (enableValidationLayers)
			{
				createInfo.enabledLayerCount = static_cast<uint32_t>(m_Instance->m_ValidationLayers.size());
				createInfo.ppEnabledLayerNames = m_Instance->m_ValidationLayers.data();
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
		}

	};

	class Buffer : public Resource
	{
		friend class PhysicalDevice;
		friend class LogicalDevice;
	public:
		struct MemoryInfo
		{
			uint32_t memoryType;
			VkDeviceSize size;
			VkDeviceSize alignment;
		};

		Buffer() : m_Device(nullptr), m_MemoryChunk(nullptr), m_Buffer(VK_NULL_HANDLE), m_Info{ 0,0,0 } {}

		void init(PhysicalDevice & physicalDevice, LogicalDevice & device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
		{
			VkBuffer vkBuffer;

			VkBufferCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = size;
			bufferInfo.usage = usage;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			if (vkCreateBuffer(device, &bufferInfo, nullptr, &m_Buffer) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create buffer!");
			}

			VkMemoryRequirements memRequirements;
			vkGetBufferMemoryRequirements(device, vkBuffer, &memRequirements);
			uint32_t memType = physicalDevice.findMemoryType(memRequirements.memoryTypeBits, properties);
			m_MemoryChunk = device.m_Allocator.allocateMemory(memType, size, memRequirements.alignment);
			m_Info.alignment = memRequirements.alignment;
			m_Info.size = m_MemoryChunk->size;
			m_Info.memoryType = memType;
		}

		void map()
		{
			m_MemoryChunk->map(m_Device);
		}

		void unmap()
		{
			m_MemoryChunk->unmap(m_Device);
		}

		void cleanup()
		{
			m_MemoryChunk->deallocate();
			if (m_Buffer != VK_NULL_HANDLE)
			{
				vkDestroyBuffer(m_Device, *this, nullptr);
				m_Buffer = VK_NULL_HANDLE;
			}
		}

		operator VkBuffer() { return m_Buffer; }

		operator VkBuffer*() { return &m_Buffer; }

	private:
		VkDevice m_Device;
		std::shared_ptr<Chunk> m_MemoryChunk;
		VkBuffer m_Buffer;
		MemoryInfo m_Info;
	};
	
	class Image2D : public Resource
	{
	public:
		Image2D() : 
			m_Device(VK_NULL_HANDLE), 
			m_Image(VK_NULL_HANDLE), 
			m_View(VK_NULL_HANDLE), 
			m_Format(VK_FORMAT_UNDEFINED), 
			m_CurrentLayout(VK_IMAGE_LAYOUT_PREINITIALIZED),
			m_Size(),
			m_MemoryChunk(nullptr)
		{}

		void init(PhysicalDevice& physicalDevice, LogicalDevice device, uint32_t width, uint32_t height, CommandPool pool, 
			VkFormat format = VK_FORMAT_R8G8B8A8_SRGB,
			VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL, 
			VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT,
			VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT)
		{
			m_Size.width = width;
			m_Size.height = height;
			m_Format = format;

			// Create Image
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

			if (vkCreateImage(device, &imageInfo, nullptr, &m_Image) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create image!");
			}

			// Allocate memory, bind image to memory
			VkMemoryRequirements imageMemRequirements;
			vkGetImageMemoryRequirements(device, m_Image, &imageMemRequirements);
			auto memType = physicalDevice.findMemoryType(imageMemRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			m_MemoryChunk = device.m_Allocator.allocateMemory(memType, imageMemRequirements.size, imageMemRequirements.alignment);

			vkBindImageMemory(device, m_Image, *m_MemoryChunk, m_MemoryChunk->offset);

			// create image view
			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_Image;
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
		}

		void transitionImageLayout(CommandPool pool, VkImageLayout newLayout)
		{
			// allocate a command buffer
			CommandBufferSingle commandBuffer;
			commandBuffer.init(m_Device, pool);

			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = m_CurrentLayout;
			barrier.newLayout = newLayout;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = m_Image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;

			if (m_CurrentLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
				barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			}
			else if (m_CurrentLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
				barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			}
			else if (m_CurrentLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			}
			else if (m_CurrentLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			}
			else {
				throw std::invalid_argument("unsupported layout transition!");
			}

			vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

			commandBuffer.execute(pool.getQueue());
			m_CurrentLayout = newLayout;
		}


		void copyFromBuffer(Buffer source, CommandPool pool)
		{
			// transition image for copying
			transitionImageLayout(pool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			CommandBufferSingle commandBuffer;
			commandBuffer.init(m_Device, pool);

			VkBufferImageCopy region = {};
			region.bufferOffset = m_MemoryChunk->offset;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;

			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;

			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = {
				m_Size.width,
				m_Size.height,
				1
			};

			vkCmdCopyBufferToImage(
				commandBuffer,
				source,
				m_Image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&region
			);

			commandBuffer.execute(pool.getQueue());

			transitionImageLayout(pool, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		void cleanup()
		{
			if (m_Image != VK_NULL_HANDLE)
			{
				vkDestroyImage(m_Device, m_Image, nullptr);
				vkDestroyImageView(m_Device, m_View, nullptr);
				m_Image = VK_NULL_HANDLE;
			}
		}

	private:
		VkDevice m_Device;
		VkImage m_Image;
		VkImageView m_View;
		VkFormat m_Format;
		VkImageLayout m_CurrentLayout;
		VkExtent2D m_Size;
		std::shared_ptr<Chunk> m_MemoryChunk;
	};

	class Sampler2D : public Resource
	{
	public:
		Sampler2D() : m_Device(VK_NULL_HANDLE), m_Sampler(VK_NULL_HANDLE) {}

		void init(VkDevice device)
		{
			m_Device = device;

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

			vkCreateSampler(device, &samplerInfo, nullptr, &m_Sampler);
		}

		void cleanup()
		{
			if (m_Sampler != VK_NULL_HANDLE)
			{
				vkDestroySampler(m_Device, m_Sampler, nullptr);
				m_Sampler = VK_NULL_HANDLE;
			}
		}

		operator VkSampler() { return m_Sampler; }

	private:
		VkDevice m_Device;
		VkSampler m_Sampler;
	};

	class RenderPass : public Resource
	{
	public:
		RenderPass() : m_Device(VK_NULL_HANDLE), m_RenderPass(VK_NULL_HANDLE) {}

		void init(VkDevice device, VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat)
		{
			m_Device = device;

			VkAttachmentDescription colorAttachment = {};
			colorAttachment.format = colorAttachmentFormat;
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkAttachmentReference colorAttachmentRef = {};
			colorAttachmentRef.attachment = 0;
			colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentDescription depthAttachment = {};
			depthAttachment.format = depthAttachmentFormat;
			depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentReference depthAttachmentRef = {};
			depthAttachmentRef.attachment = 1;
			depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentRef;
			subpass.pDepthStencilAttachment = &depthAttachmentRef;

			VkSubpassDependency dependency = {};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.srcAccessMask = 0;
			dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			renderPassInfo.pAttachments = attachments.data();
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 1;
			renderPassInfo.pDependencies = &dependency;

			if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS) 
			{
				throw std::runtime_error("failed to create render pass!");
			}
		}

		void cleanup()
		{
			if (m_RenderPass != VK_NULL_HANDLE)
			{
				vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
				m_RenderPass = VK_NULL_HANDLE;
			}
		}

	private:
		VkDevice m_Device;
		VkRenderPass m_RenderPass;
	};

	class PipelineLayout : public Resource
	{
	public:
		PipelineLayout() : m_Device(VK_NULL_HANDLE), m_PipelineLayout(VK_NULL_HANDLE) {}

		void init(VkDevice device, std::vector<VkPushConstantRange> pushConstantRanges, std::vector<VkDescriptorSetLayout> descriptorSetLayouts) 
		{
			m_Device = device;

			VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
			pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
			pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
			pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

			if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) 
			{
				throw std::runtime_error("failed to create pipeline layout!");
			}
		}

		void cleanup() 
		{
			if (m_PipelineLayout != VK_NULL_HANDLE)
			{
				vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
				m_PipelineLayout = VK_NULL_HANDLE;
			}
		}
	private:
		VkDevice m_Device;
		VkPipelineLayout m_PipelineLayout;
	};

	class Pipeline : public Resource
	{
	public:
		Pipeline() : m_Device(VK_NULL_HANDLE), m_Pipeline(VK_NULL_HANDLE) {}

		void init(
			LogicalDevice& device,
			VkRenderPass renderPass,
			VkShaderModule vertexShader,
			VkShaderModule fragmentShader,
			std::vector<VkVertexInputBindingDescription> bindingDescriptions,
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions,
			VkExtent2D swapChainExtent,
			VkPipelineLayout pipelineLayout
		)
		{
			m_Device = device;

			VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
			vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertShaderStageInfo.module = vertexShader;
			vertShaderStageInfo.pName = "main";

			VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
			fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragShaderStageInfo.module = fragmentShader;
			fragShaderStageInfo.pName = "main";

			VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

			VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
			vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
			vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
			vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

			VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
			inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssembly.primitiveRestartEnable = VK_FALSE;

			VkViewport viewport = {};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = (float)swapChainExtent.width;
			viewport.height = (float)swapChainExtent.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			VkRect2D scissor = {};
			scissor.offset = { 0, 0 };
			scissor.extent = swapChainExtent;

			VkPipelineViewportStateCreateInfo viewportState = {};
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.viewportCount = 1;
			viewportState.pViewports = &viewport;
			viewportState.scissorCount = 1;
			viewportState.pScissors = &scissor;

			VkPipelineRasterizationStateCreateInfo rasterizer = {};
			rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizer.depthClampEnable = VK_FALSE;
			rasterizer.rasterizerDiscardEnable = VK_FALSE;
			rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizer.lineWidth = 1.0f;
			rasterizer.cullMode = VK_CULL_MODE_NONE;
			rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			rasterizer.depthBiasEnable = VK_FALSE;
			rasterizer.depthBiasConstantFactor = 0.0f; // Optional
			rasterizer.depthBiasClamp = 0.0f; // Optional
			rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

			VkPipelineMultisampleStateCreateInfo multisampling = {};
			multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampling.sampleShadingEnable = VK_FALSE;
			multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			multisampling.minSampleShading = 1.0f; // Optional
			multisampling.pSampleMask = nullptr; // Optional
			multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
			multisampling.alphaToOneEnable = VK_FALSE; // Optional

			VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
			colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colorBlendAttachment.blendEnable = VK_FALSE;
			colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
			colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
			colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
			colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
			colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
			colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

			VkPipelineColorBlendStateCreateInfo colorBlending = {};
			colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlending.logicOpEnable = VK_FALSE;
			colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
			colorBlending.attachmentCount = 1;
			colorBlending.pAttachments = &colorBlendAttachment;
			colorBlending.blendConstants[0] = 0.0f; // Optional
			colorBlending.blendConstants[1] = 0.0f; // Optional
			colorBlending.blendConstants[2] = 0.0f; // Optional
			colorBlending.blendConstants[3] = 0.0f; // Optional

			VkPipelineDepthStencilStateCreateInfo depthStencil = {};
			depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencil.depthTestEnable = VK_TRUE;
			depthStencil.depthWriteEnable = VK_TRUE;
			depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
			depthStencil.depthBoundsTestEnable = VK_FALSE;
			depthStencil.minDepthBounds = 0.0f; // Optional
			depthStencil.maxDepthBounds = 1.0f; // Optional
			depthStencil.stencilTestEnable = VK_FALSE;
			depthStencil.front = {}; // Optional
			depthStencil.back = {}; // Optional

			VkGraphicsPipelineCreateInfo pipelineInfo = {};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo.stageCount = 2;
			pipelineInfo.pStages = shaderStages;
			pipelineInfo.pVertexInputState = &vertexInputInfo;
			pipelineInfo.pInputAssemblyState = &inputAssembly;
			pipelineInfo.pViewportState = &viewportState;
			pipelineInfo.pRasterizationState = &rasterizer;
			pipelineInfo.pMultisampleState = &multisampling;
			pipelineInfo.pDepthStencilState = &depthStencil;
			pipelineInfo.pColorBlendState = &colorBlending;
			pipelineInfo.pDynamicState = nullptr; // Optional
			pipelineInfo.layout = pipelineLayout;
			pipelineInfo.renderPass = renderPass;
			pipelineInfo.subpass = 0;
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
			pipelineInfo.basePipelineIndex = -1; // Optional

			if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create graphics pipeline!");
			}

			vkDestroyShaderModule(device, vertexShader, nullptr);
			vkDestroyShaderModule(device, fragmentShader, nullptr);
		}

		void cleanup()
		{
			if (m_Pipeline != VK_NULL_HANDLE)
			{
				vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
				m_Pipeline = VK_NULL_HANDLE;
			}
		}

		operator VkPipeline() { return m_Pipeline; }

	private:
		VkDevice m_Device;
		VkPipeline m_Pipeline;
	};

	class Framebuffer : public Resource
	{
	public:
		Framebuffer() : m_Device(VK_NULL_HANDLE), m_Framebuffer(VK_NULL_HANDLE) {}

		void init(
			LogicalDevice& device,
			VkRenderPass renderPass,
			VkImageView swapChainImageView, 
			VkImageView depthImageView, 
			VkExtent2D swapChainExtent
			)
		{
			m_Device = device;

			const uint32_t attachmentCount = 2;
			std::array<VkImageView, attachmentCount> attachments =
			{
				swapChainImageView,
				depthImageView
			};

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = attachmentCount;
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &m_Framebuffer) != VK_SUCCESS) 
			{
				throw std::runtime_error("failed to create framebuffer!");
			}
		}

		void cleanup()
		{
			if (m_Framebuffer != VK_NULL_HANDLE)
			{
				vkDestroyFramebuffer(m_Device, m_Framebuffer, nullptr);
				m_Framebuffer = VK_NULL_HANDLE;
			}
		}

		operator VkFramebuffer() { return m_Framebuffer; }

	private:
		VkDevice m_Device;
		VkFramebuffer m_Framebuffer;
	};

	class DescriptorSetLayout : public Resource
	{
	public:
		void init()
		{
		}

		void cleanup()
		{
		}

	private:

	};

	VkDescriptorSetLayout createDescriptorSetLayout(LogicalDevice& device, std::vector<VkDescriptorSetLayoutBinding> bindings)
	{
		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		VkDescriptorSetLayout descriptorSetLayout;

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	VkDescriptorPool createDescriptorPool(LogicalDevice& device, std::vector<VkDescriptorPoolSize> poolSizes)
	{
		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = 1;

		VkDescriptorPool descriptorPool;
		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor pool!");
		}
		return descriptorPool;
	}

	class DescriptorSets
	{
	public:
		DescriptorSets() : m_Device(VK_NULL_HANDLE), 
			m_DescriptorPool(VK_NULL_HANDLE), 
			m_DescriptorSets(VK_NULL_HANDLE), 
			m_DescriptorSetLayouts(VK_NULL_HANDLE)
		{
		}
		void init(LogicalDevice& device)
		{
			m_Device = device;
		}

		void createPool()
		{

		}

		void allocate()
		{
			VkDescriptorSetAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = m_DescriptorPool;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = m_DescriptorSetLayouts.data();

			if (vkAllocateDescriptorSets(m_Device, &allocInfo, m_DescriptorSets.data()) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to allocate descriptor set!");
			}
		}

		void updateSets();

		void addDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding> bindings)
		{
			VkDescriptorSetLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
			layoutInfo.pBindings = bindings.data();

			VkDescriptorSetLayout descriptorSetLayout;

			if (vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create descriptor set layout!");
			}
			m_DescriptorSetLayouts.push_back(descriptorSetLayout);
		}
	private:
		VkDevice m_Device;
		VkDescriptorPool m_DescriptorPool;
		std::vector<VkDescriptorSet> m_DescriptorSets;
		std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
	};
}