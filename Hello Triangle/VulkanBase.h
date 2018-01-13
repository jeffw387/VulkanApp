#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <iostream>
#include <set>
#include <array>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include "MemoryAllocator.h"
#include "fileIO.h"

#ifdef NDEBUG
constexpr bool DebugMode = false;
constexpr bool enableValidationLayers = false;
#else
constexpr bool DebugMode = true;
constexpr bool enableValidationLayers = true;
#endif

namespace vke
{
	static uint64_t imagesCreated = 0;
	static uint64_t imagesDestroyed = 0;
	static uint64_t buffersCreated = 0;
	static uint64_t buffersDestroyed = 0;

	struct SwapchainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	class Instance
	{
		friend class PhysicalDevice;
		friend class LogicalDevice;
	public:
		void init(const std::vector<const char*>& validationLayers, const std::vector<const char*>& requiredExtensions, const std::vector<const char*>& deviceExtensions, VkAllocationCallbacks* allocators = nullptr)
		{
			m_ValidationLayers = validationLayers;
			m_VulkanExtensions = requiredExtensions;
			m_DeviceExtensions = deviceExtensions;
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

			if (vkCreateInstance(&createInfo, allocators, &m_Instance) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create vulkan instance!");
			}

			setupDebugCallback(m_Callback);
		}

		void cleanup()
		{
			if (m_Instance != VK_NULL_HANDLE)
			{
				if (enableValidationLayers)
					DestroyDebugReportCallbackEXT(m_Instance, m_Callback, nullptr);
				vkDestroyInstance(m_Instance, nullptr);
				m_Instance = VK_NULL_HANDLE;
			}
		}

		operator VkInstance() { return m_Instance; }

		operator VkInstance*() { return &m_Instance; }

	private:
		VkInstance m_Instance = VK_NULL_HANDLE;
		std::vector<const char*> m_VulkanExtensions;
		std::vector<const char*> m_DeviceExtensions;
		std::vector<const char*> m_ValidationLayers;
		VkDebugReportCallbackEXT m_Callback;

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
		bool checkExtensionSupport(std::vector<const char*> requiredExtensions)
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
		void setupDebugCallback(VkDebugReportCallbackEXT& callback)
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

	class Surface
	{
	public:
		void init(const VkInstance instance, const HINSTANCE hInstance, const HWND hWnd)
		{
			m_Instance = instance;
			VkWin32SurfaceCreateInfoKHR createInfo = {};
			createInfo.hinstance = hInstance;
			createInfo.hwnd = hWnd;
			createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			createInfo.flags = 0;

			vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &m_Surface);
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

	class CommandPool
	{
	public:
		void init(VkDevice device, uint32_t queueFamily, VkCommandPoolCreateFlags flags)
		{
			m_Device = device;
			m_QueueFamily = queueFamily;
			VkCommandPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.queueFamilyIndex = queueFamily;
			poolInfo.flags = flags;

			if (vkCreateCommandPool(device, &poolInfo, nullptr, &m_Pool) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create command pool!");
			}
		}

		void reset(VkCommandPoolResetFlags flags = 0)
		{
			vkResetCommandPool(m_Device, m_Pool, flags);
		}

		void cleanup()
		{
			vkDestroyCommandPool(m_Device, m_Pool, nullptr);
		}

		auto getQueue() { return m_QueueFamily; }
		operator VkCommandPool() { return m_Pool; }

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkCommandPool m_Pool = VK_NULL_HANDLE;
		uint32_t m_QueueFamily = 0;
	};

	class CommandBuffer
	{
	public:
		void init(VkDevice device, VkCommandPool pool)
		{
			m_Device = device;
			m_Pool = pool;

			VkCommandBufferAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandPool = pool;
			allocInfo.commandBufferCount = 1;

			vkAllocateCommandBuffers(device, &allocInfo, &m_Buffer);
		}

		void begin(VkCommandBufferUsageFlags flags)
		{
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = flags;

			vkBeginCommandBuffer(m_Buffer, &beginInfo);
		}

		void end()
		{
			vkEndCommandBuffer(m_Buffer);
		}

		void submit(VkQueue queue, 
			std::vector<VkPipelineStageFlags> waitMask,
			VkFence fence,
			std::vector<VkSemaphore>& waitSemaphores,
			std::vector<VkSemaphore>& signalSemaphores
		)
		{
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &m_Buffer;
			submitInfo.pWaitDstStageMask = waitMask.data();
			submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
			submitInfo.pWaitSemaphores = waitSemaphores.data();
			submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
			submitInfo.pSignalSemaphores = signalSemaphores.data();

			vkQueueSubmit(queue, 1, &submitInfo, fence);
		}

		void reset(VkCommandBufferResetFlags flags = 0)
		{
			vkResetCommandBuffer(m_Buffer, flags);
		}

		void cleanup()
		{
			if (m_Buffer != VK_NULL_HANDLE)
			{
				vkFreeCommandBuffers(m_Device, m_Pool, 1, &m_Buffer);
				m_Buffer = VK_NULL_HANDLE;
			}
		}

		operator VkCommandBuffer() { return m_Buffer; }

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkCommandPool m_Pool = VK_NULL_HANDLE;
		VkCommandBuffer m_Buffer = VK_NULL_HANDLE;
	};

	class PhysicalDevice
	{
		friend class Image2D;
		friend class Buffer;
	public:
		void init(Instance& instance, VkSurfaceKHR surface)
		{
			pickPhysicalDevice(instance, surface);

			vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_DeviceProperties);
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
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties m_DeviceProperties = {};
		int m_GraphicsQueueIndex = 0;
		int m_PresentQueueIndex = 0;

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

		bool checkDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*> deviceExtensions)
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

		SwapchainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
		{
			SwapchainSupportDetails details;

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

		auto findGraphicsQueue(std::vector<VkQueueFamilyProperties>& queueFamilyProperties)
		{
			// find graphics queue index
			int graphicsIndex = 0;
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

		auto findPresentQueue(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, std::vector<VkQueueFamilyProperties>& queueFamilyProperties)
		{
			// find present queue index
			VkBool32 surfaceSupport = false;
			int presentIndex = 0;
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
				SwapchainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);
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

	class LogicalDevice
	{
	public:
		operator VkDevice() { return m_Device; }

		void init(VkInstance instance, 
			PhysicalDevice physicalDevice, 
			std::set<int> queueFamilyIndices, 
			const std::vector<const char*>& deviceExtensions,
			const std::vector<const char*>& validationLayers
			)
		{
			m_Instance = instance;
			m_PhysicalDevice = physicalDevice;

			createLogicalDevice(queueFamilyIndices, deviceExtensions, validationLayers);

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
		VkInstance m_Instance = VK_NULL_HANDLE;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_Device = VK_NULL_HANDLE;
		Queue m_GraphicsQueue = Queue();
		Queue m_PresentQueue = Queue();

		void createLogicalDevice(
			const std::set<int>& uniqueQueueFamilies, 
			const std::vector<const char*>& deviceExtensions, 
			const std::vector<const char*>& validationLayers
		)
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
			createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
			createInfo.ppEnabledExtensionNames = deviceExtensions.data();

			// pass in validation layer info if in debug mode
			if (enableValidationLayers)
			{
				createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
				createInfo.ppEnabledLayerNames = validationLayers.data();
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

	class Fence
	{
	public:
		Fence() {}

		void init(VkDevice device, VkFenceCreateFlags flags = VK_FENCE_CREATE_SIGNALED_BIT)
		{
			m_Device = device;

			VkFenceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			createInfo.flags = flags;
			createInfo.pNext = nullptr;

			vkCreateFence(m_Device, &createInfo, nullptr, &m_Fence);
		}

		void reset()
		{
			vkResetFences(m_Device, 1, &m_Fence);
		}

		void cleanup()
		{
			if (m_Fence != VK_NULL_HANDLE)
			{
				vkDestroyFence(m_Device, m_Fence, nullptr);
				m_Fence = VK_NULL_HANDLE;
			}
		}

		operator VkFence() { return m_Fence; }
		operator VkFence*() { return &m_Fence; }

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkFence m_Fence = VK_NULL_HANDLE;
	};

	class Semaphore
	{
	public:
		void init(VkDevice device)
		{
			m_Device = device;

			VkSemaphoreCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			vkCreateSemaphore(m_Device, &createInfo, nullptr, &m_Semaphore);
		}

		void cleanup()
		{
			if (m_Semaphore != VK_NULL_HANDLE)
			{
				vkDestroySemaphore(m_Device, m_Semaphore, nullptr);
				m_Semaphore = VK_NULL_HANDLE;
			}
		}

		operator VkSemaphore() { return m_Semaphore; }

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkSemaphore m_Semaphore = VK_NULL_HANDLE;
	};

	class ShaderModule
	{
	public:
		ShaderModule() : m_Device(VK_NULL_HANDLE), m_ShaderModule(VK_NULL_HANDLE) {}

		void init(VkDevice device, const std::vector<char>& code)
		{
			m_Device = device;

			VkShaderModuleCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = code.size();

			// 12 byte (96 bit) size / 4 == 3 (uint32_t) + 1 = 4; i.e. codeAligned size == 16 bytes or 128 bits;
			std::vector<uint32_t> codeAligned(code.size() / sizeof(uint32_t) + 1);
			memcpy(codeAligned.data(), code.data(), code.size());
			createInfo.pCode = codeAligned.data();

			if (vkCreateShaderModule(device, &createInfo, nullptr, &m_ShaderModule) != VK_SUCCESS) 
			{
				throw std::runtime_error("failed to create shader module!");
			}
		}

		void cleanup()
		{
			if (m_ShaderModule != VK_NULL_HANDLE)
			{
				vkDestroyShaderModule(m_Device, m_ShaderModule, nullptr);
				m_ShaderModule = VK_NULL_HANDLE;
			}
		}

		operator VkShaderModule() { return m_ShaderModule; }

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkShaderModule m_ShaderModule = VK_NULL_HANDLE;
	};

	class Buffer
	{
	public:
		struct MemoryInfo
		{
			uint32_t memoryType;
			VkDeviceSize size;
			VkDeviceSize alignment;
		};

		void init(VkDevice device, Allocator* allocator, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, vke::AllocationStyle allocationStyle = vke::AllocationStyle::NoPreference)
		{
			m_Device = device;
			m_Allocator = allocator;
			m_MemoryProperties = properties;

			m_BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			m_BufferInfo.size = size;
			m_BufferInfo.usage = usage;
			m_BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			if (vkCreateBuffer(device, &m_BufferInfo, nullptr, &m_Buffer) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create buffer!");
			}
			buffersCreated++;
			if (m_Allocator->allocateBuffer(m_MemoryRange, m_Buffer, properties, allocationStyle) == false)
			{
				throw std::runtime_error("failed to allocate memory for buffer!");
			}

			vkBindBufferMemory(m_Device, m_Buffer, m_MemoryRange.memory, m_MemoryRange.usableStartOffset);
		}

		bool mapMemoryRange()
		{
			if (m_MemoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT && m_Mapped == false)
			{
				if (m_Allocator->mapMemoryRange(m_MemoryRange, mappedData))
				{
					m_Mapped = true;
					return true;
				}
			}
			return false;
		}

		VkMappedMemoryRange getMappedMemoryRange()
		{
			VkMappedMemoryRange range = {};
			range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			range.memory = m_MemoryRange.memory;
			range.offset = m_MemoryRange.usableStartOffset;
			range.size = m_MemoryRange.usableSize;

			return range;
		}

		void unmap()
		{
			if (m_Mapped)
			{
				m_Allocator->unmapRange(m_MemoryRange);
				m_Mapped = false;
			}
		}

		void copyFromBuffer(Buffer& source, CommandBuffer& commandBuffer, 
		VkQueue queue, VkFence fence, std::vector<VkSemaphore>& waitSemaphores, std::vector<VkSemaphore>& signalSemaphores, 
		VkDeviceSize srcBufferOffset = 0, VkDeviceSize dstBufferOffset = 0, VkDeviceSize size = 0)
		{
			commandBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

			VkDeviceSize copySize;
			if (size == 0)
			{
				copySize = source.getUsableSize();
			}
			else
			{
				copySize = size;
			}
			VkBufferCopy copyRegion = {};
			copyRegion.srcOffset = srcBufferOffset;
			copyRegion.dstOffset = dstBufferOffset;
			copyRegion.size = copySize;

			vkCmdCopyBuffer(commandBuffer, source, m_Buffer, 1, &copyRegion);

			commandBuffer.end();
			commandBuffer.submit(queue, std::vector<VkPipelineStageFlags>{0}, fence, waitSemaphores, signalSemaphores);
		}

		void cleanup()
		{
			if (m_Allocator != nullptr)
				m_Allocator->deallocateRange(m_MemoryRange);
			if (m_Buffer != VK_NULL_HANDLE)
			{
				vkDestroyBuffer(m_Device, *this, nullptr);
				buffersDestroyed++;
				m_Buffer = VK_NULL_HANDLE;
			}
		}

		operator VkBuffer() { return m_Buffer; }

		VkDeviceSize getUsableOffset() { return m_MemoryRange.usableStartOffset; }
		VkDeviceSize getUsableSize() { return m_MemoryRange.usableSize; }

		void* mappedData = nullptr;

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkBufferCreateInfo m_BufferInfo = {};
		VkBuffer m_Buffer = VK_NULL_HANDLE;
		Allocator* m_Allocator = nullptr;
		VkMemoryPropertyFlags m_MemoryProperties = {};
		vke::MemoryRange m_MemoryRange = {};
		bool m_Mapped = false;
	};
	
	class Image2D
	{
	public:
		void init(VkDevice device, Allocator* allocator, uint32_t width, uint32_t height, 
			VkFormat format = VK_FORMAT_R8G8B8A8_SRGB,
			VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
			VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
			VkSharingMode shareMode = VK_SHARING_MODE_EXCLUSIVE,
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			vke::AllocationStyle allocationStyle = vke::AllocationStyle::NoPreference)
		{
			m_Device = device;
			m_Allocator = allocator;

			m_Size.width = width;
			m_Size.height = height;
			m_Format = format;
			m_CurrentLayout = imageLayout;
			m_AspectFlags = aspectFlags;

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
			imageInfo.initialLayout = imageLayout;
			imageInfo.usage = usage;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.sharingMode = shareMode;

			if (vkCreateImage(device, &imageInfo, nullptr, &m_Image) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create image!");
			}
			imagesCreated++;
			// Allocate memory, bind image to memory
			if (m_Allocator->allocateImage(m_MemoryRange, m_Image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, allocationStyle) == false)
			{
				throw std::runtime_error("failed to allocate memory for image!");
			}

			if (vkBindImageMemory(device, m_Image, m_MemoryRange, m_MemoryRange.usableStartOffset) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to bind image to memory!");
			}

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

			if (vkCreateImageView(m_Device, &viewInfo, nullptr, &m_View) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create image view!");
			}
		}

		void transitionImageLayout(VkCommandPool pool, VkQueue queue, VkImageLayout newLayout)
		{
			// allocate a command buffer
			CommandBuffer commandBuffer;
			commandBuffer.init(m_Device, pool);
			commandBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = m_CurrentLayout;
			barrier.newLayout = newLayout;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = m_Image;
			barrier.subresourceRange.aspectMask = m_AspectFlags;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;

			VkPipelineStageFlags sourceStageFlags;
			VkPipelineStageFlags destStageFlags;

			if (m_CurrentLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				sourceStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				destStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			else if (m_CurrentLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
			{
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				sourceStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
				destStageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}
			else if (m_CurrentLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) 
			{
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

				sourceStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				destStageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			}
			else 
			{
				throw std::invalid_argument("unsupported layout transition!");
			}

			vkCmdPipelineBarrier(
				commandBuffer,
				sourceStageFlags, destStageFlags,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

			commandBuffer.end();
			std::vector<VkSemaphore> semaphores;
			commandBuffer.submit(queue, std::vector<VkPipelineStageFlags>{0}, VK_NULL_HANDLE, semaphores, semaphores);
			vkQueueWaitIdle(queue);
			m_CurrentLayout = newLayout;
			commandBuffer.cleanup();
		}

		void copyFromBuffer(VkBuffer source, VkCommandPool pool, VkQueue queue, VkDeviceSize bufferOffset = 0)
		{
			// transition image for copying
			transitionImageLayout(pool, queue, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			CommandBuffer commandBuffer;
			commandBuffer.init(m_Device, pool);
			commandBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

			VkBufferImageCopy region = {};
			region.bufferOffset = bufferOffset;
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

			commandBuffer.end();
			std::vector<VkSemaphore> semaphores;
			commandBuffer.submit(queue, std::vector<VkPipelineStageFlags>(), VK_NULL_HANDLE, semaphores, semaphores);
			vkQueueWaitIdle(queue);
			transitionImageLayout(pool, queue, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			commandBuffer.cleanup();
		}

		void cleanup()
		{
			if (m_Allocator != nullptr)
				m_Allocator->deallocateRange(m_MemoryRange);
			if (m_Image != VK_NULL_HANDLE)
			{
				vkDestroyImage(m_Device, m_Image, nullptr);
				imagesDestroyed++;
				vkDestroyImageView(m_Device, m_View, nullptr);
				m_Image = VK_NULL_HANDLE;
			}
		}

		MemoryRange getMemoryRange() { return m_MemoryRange; }

		VkImage getImage() { return m_Image; }
		
		VkImageView getView() { return m_View; }

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkImage m_Image = VK_NULL_HANDLE;
		VkImageView m_View = VK_NULL_HANDLE;
		VkFormat m_Format;
		VkImageLayout m_CurrentLayout;
		VkExtent2D m_Size;
		VkImageAspectFlags m_AspectFlags;
		Allocator* m_Allocator = nullptr;
		MemoryRange m_MemoryRange;
	};

	class Sampler2D
	{
	public:
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
		operator VkSampler*() { return &m_Sampler; }
		operator void*() { return (void*)&m_Sampler; }

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkSampler m_Sampler = VK_NULL_HANDLE;
	};

	class Swapchain
	{
	public:
		void init (PhysicalDevice& physicalDevice, 
			VkDevice device, 
			VkSurfaceKHR surface,
			SwapchainSupportDetails supportDetails,
			VkExtent2D swapExtent,
			uint32_t imageCount,
			VkSurfaceFormatKHR surfaceFormat,
			VkPresentModeKHR presentMode)
		{
			m_Device = device;
			m_SurfaceFormat = surfaceFormat;
			m_SwapchainExtent = swapExtent;
			m_ImageCount = imageCount;

			VkSwapchainCreateInfoKHR createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			createInfo.surface = surface;
			createInfo.minImageCount = m_ImageCount;
			createInfo.imageFormat = m_SurfaceFormat.format;
			createInfo.imageColorSpace = m_SurfaceFormat.colorSpace;
			createInfo.imageExtent = m_SwapchainExtent;
			createInfo.imageArrayLayers = 1;
			createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

			auto graphicsQueueIndex = physicalDevice.getGraphicsQueueIndex();
			auto presentQueueIndex = physicalDevice.getPresentQueueIndex();

			std::array<uint32_t, 2> indices = { static_cast<uint32_t>(graphicsQueueIndex), static_cast<uint32_t>(presentQueueIndex) };

			if (graphicsQueueIndex != presentQueueIndex) {
				createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
				createInfo.queueFamilyIndexCount = 2;
				createInfo.pQueueFamilyIndices = indices.data();
			}
			else {
				createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
				createInfo.queueFamilyIndexCount = 0; // Optional
				createInfo.pQueueFamilyIndices = nullptr; // Optional
			}

			createInfo.preTransform = supportDetails.capabilities.currentTransform;
			createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			createInfo.presentMode = presentMode;
			createInfo.clipped = VK_TRUE;
			createInfo.oldSwapchain = VK_NULL_HANDLE;

			if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &m_Swapchain) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create swap chain!");
			}

			// get swap chain images
			vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &m_ImageCount, nullptr);
			m_SwapchainImages.resize(m_ImageCount);
			vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &m_ImageCount, m_SwapchainImages.data());

			// create image views
			m_SwapchainViews.resize(m_ImageCount);
			for (uint32_t i = 0; i < m_ImageCount; i++)
			{
				m_SwapchainViews[i] = createImageView(m_SwapchainImages[i], m_SurfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
			}
		}

		void cleanup()
		{
			if (m_Swapchain != VK_NULL_HANDLE)
			{
				for (auto& view : m_SwapchainViews)
				{
					vkDestroyImageView(m_Device, view, nullptr);
				}
				vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
				m_Swapchain = VK_NULL_HANDLE;
			}
		}

		operator VkSwapchainKHR() { return m_Swapchain; }

		const auto& getSwapChainImages() { return m_SwapchainImages; }
		const auto& getSwapChainViews() { return m_SwapchainViews; }
		const auto& getFormat() { return m_SurfaceFormat.format; }
		const auto& getExtent() { return m_SwapchainExtent; }
		const auto& getImageCount() { return m_ImageCount; }

		// populates SwapChainSupportDetails struct with information about the swapchain support on the given GPU
		static SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
		{
			SwapchainSupportDetails details;

			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
			if(formatCount != 0)
			{
				details.formats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
			}

			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

			if(presentModeCount != 0)
			{
				details.presentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
			}

			return details;
		}

		static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
		{
			if(availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
			{
				return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
			}

			for(const auto& availableFormat : availableFormats)
			{
				if(availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
					return availableFormat;
			}

			return availableFormats[0];
		}

		// iterate through available present modes and return the best choice available
		static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
		{
			// Fallback mode
			VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

			for (const auto& availablePresentMode : availablePresentModes)
			{
				// ideal mode
				if(availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					bestMode = availablePresentMode;
				}
				// second best mode
				else if(availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
				{
					bestMode = availablePresentMode;
				}
			}

			return bestMode;
		}

		static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, VkExtent2D currentWindowSize)
		{
			if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
				return capabilities.currentExtent;
			}
			else 
			{
				VkExtent2D actualExtent = currentWindowSize;

				actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
				actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

				return actualExtent;
			}
		}

		static uint32_t chooseImageCount(const SwapchainSupportDetails& supportDetails)
		{
			auto result = supportDetails.capabilities.minImageCount + 1;
			if (supportDetails.capabilities.maxImageCount > 0 && result > supportDetails.capabilities.maxImageCount)
				result = supportDetails.capabilities.maxImageCount;
			return result;
		}

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
		std::vector<VkImage> m_SwapchainImages;
		std::vector<VkImageView> m_SwapchainViews;
		VkSurfaceFormatKHR m_SurfaceFormat;
		VkExtent2D m_SwapchainExtent;
		uint32_t m_ImageCount;

		VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
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
	};

	class RenderPass
	{
	public:
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

		operator VkRenderPass() { return m_RenderPass; }

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkRenderPass m_RenderPass = VK_NULL_HANDLE;
	};

	class PipelineLayout
	{
	public:
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

		operator VkPipelineLayout() { return m_PipelineLayout; }

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
	};

	class Pipeline
	{
	public:
		void init(
			LogicalDevice& device,
			VkRenderPass renderPass,
			VkShaderModule vertexShader,
			uint32_t imageCount,
			VkShaderModule fragmentShader,
			std::vector<VkVertexInputBindingDescription> bindingDescriptions,
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions,
			VkExtent2D windowExtent,
			VkPipelineLayout pipelineLayout,
			bool DepthEnabled = false
		)
		{
			m_Device = device;

			VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
			vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertShaderStageInfo.mod = vertexShader;
			vertShaderStageInfo.pName = "main";

			auto FragmentSpecializationSize = sizeof(uint32_t);
			VkSpecializationMapEntry imageCountSpecialization = {};
			imageCountSpecialization.constantID = 0;
			imageCountSpecialization.offset = 0;
			imageCountSpecialization.size = FragmentSpecializationSize;

			VkSpecializationInfo fragmentSpecializationInfo = {};
			fragmentSpecializationInfo.dataSize = FragmentSpecializationSize;
			fragmentSpecializationInfo.mapEntryCount = 1;
			fragmentSpecializationInfo.pData = &imageCount;
			fragmentSpecializationInfo.pMapEntries = &imageCountSpecialization;

			VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
			fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragShaderStageInfo.mod = fragmentShader;
			fragShaderStageInfo.pName = "main";
			fragShaderStageInfo.pSpecializationInfo = &fragmentSpecializationInfo;

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

			VkPipelineViewportStateCreateInfo viewportState = {};
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.viewportCount = 1;
			viewportState.pViewports = nullptr;
			viewportState.scissorCount = 1;
			viewportState.pScissors = nullptr;

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
			colorBlendAttachment.blendEnable = VK_TRUE;
			colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
			colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

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
			depthStencil.flags = 0;
			depthStencil.depthTestEnable = DepthEnabled;
			depthStencil.depthWriteEnable = DepthEnabled;
			depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
			depthStencil.depthBoundsTestEnable = VK_FALSE;
			depthStencil.minDepthBounds = 0;
			depthStencil.maxDepthBounds = 0;
			depthStencil.stencilTestEnable = VK_FALSE;
			depthStencil.back.failOp = VK_STENCIL_OP_KEEP;
			depthStencil.back.passOp = VK_STENCIL_OP_KEEP;
			depthStencil.back.compareOp = VK_COMPARE_OP_ALWAYS;
			depthStencil.back.compareMask = 0;
			depthStencil.back.writeMask = 0;
			depthStencil.back.reference = 0;
			depthStencil.back.depthFailOp = VK_STENCIL_OP_KEEP;
			depthStencil.front = depthStencil.back; // Optional

			const uint32_t dynamicStateCount = 2;
			std::array<VkDynamicState, dynamicStateCount> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
			VkPipelineDynamicStateCreateInfo dynamicStateInfo = {};
			dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicStateInfo.pDynamicStates = dynamicStates.data();
			dynamicStateInfo.dynamicStateCount = dynamicStateCount;

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
			pipelineInfo.pDynamicState = &dynamicStateInfo;
			pipelineInfo.layout = pipelineLayout;
			pipelineInfo.renderPass = renderPass;
			pipelineInfo.subpass = 0;
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
			pipelineInfo.basePipelineIndex = -1; // Optional

			try
			{
			if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create graphics pipeline!");
			}
			}
			catch (std::exception e)
			{
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
		VkDevice m_Device = VK_NULL_HANDLE;
		VkPipeline m_Pipeline = VK_NULL_HANDLE;
	};

	class Framebuffer
	{
	public:
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
		VkDevice m_Device = VK_NULL_HANDLE;
		VkFramebuffer m_Framebuffer = VK_NULL_HANDLE;
	};

	class DescriptorSetLayout
	{
	public:
		void init(LogicalDevice& device, std::vector<VkDescriptorSetLayoutBinding> bindings)
		{
			m_Device = device;

			VkDescriptorSetLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
			layoutInfo.pBindings = bindings.data();

			if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_DSLayout) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create descriptor set layout!");
			}
		}

		operator VkDescriptorSetLayout() { return m_DSLayout; }

		void cleanup()
		{
			if (m_DSLayout != VK_NULL_HANDLE)
			{
				vkDestroyDescriptorSetLayout(m_Device, m_DSLayout, nullptr);
				m_DSLayout = VK_NULL_HANDLE;
			}
		}

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_DSLayout = VK_NULL_HANDLE;
	};

	class DescriptorSet
	{
	public:
		void init(VkDevice device, VkDescriptorPool pool, std::vector<VkDescriptorSetLayout>& setLayouts)
		{
			m_Device = device;
			m_DescriptorPool = pool;

			VkDescriptorSetAllocateInfo allocateInfo = {};
			allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocateInfo.descriptorPool = pool;
			allocateInfo.descriptorSetCount = static_cast<uint32_t>(setLayouts.size());
			allocateInfo.pSetLayouts = setLayouts.data();

			vkAllocateDescriptorSets(device, &allocateInfo, &m_DescriptorSet);
		}

		//~DescriptorSet()
		//{
		//	if (m_DescriptorSet != VK_NULL_HANDLE)
		//	{
		//		vkFreeDescriptorSets(m_Device, m_DescriptorPool, 1, &m_DescriptorSet);
		//		m_DescriptorSet = VK_NULL_HANDLE;
		//	}
		//}

		void update(VkWriteDescriptorSet writeInfo)
		{
			vkUpdateDescriptorSets(m_Device, 1, &writeInfo, 0, nullptr);
		}

		operator VkDescriptorSet() { return m_DescriptorSet; }
		operator VkDescriptorSet*() { return &m_DescriptorSet; }

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
		VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
	};

	class DescriptorPool
	{
	public:
		void init(LogicalDevice& device, std::vector<VkDescriptorPoolSize> poolSizes, uint32_t maxSets)
		{
			m_Device = device;

			VkDescriptorPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
			poolInfo.pPoolSizes = poolSizes.data();
			poolInfo.maxSets = maxSets;

			if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_Pool) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create descriptor pool!");
			}
		}

		auto allocateSets(std::vector<VkDescriptorSetLayout> layouts)
		{
			VkDescriptorSetAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = m_Pool;
			allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
			allocInfo.pSetLayouts = layouts.data();

			std::vector<VkDescriptorSet> sets(layouts.size());

			if (vkAllocateDescriptorSets(m_Device, &allocInfo, sets.data()) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to allocate descriptor set!");
			}

			return sets;
		}

		void cleanup()
		{
			if (m_Pool != VK_NULL_HANDLE)
			{
				vkDestroyDescriptorPool(m_Device, m_Pool, nullptr);
				m_Pool = VK_NULL_HANDLE;
			}
		}

		operator VkDescriptorPool() { return m_Pool; }

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkDescriptorPool m_Pool = VK_NULL_HANDLE;
	};
}