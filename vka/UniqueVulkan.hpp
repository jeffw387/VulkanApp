#pragma once
#include <memory>
#include "vulkan/vulkan.h"

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
			VkDestroyDevice(device, nullptr);
		}
	};
	using VkDeviceUnique = std::unique_ptr<VkDevice, VkDeviceDeleter>;

    struct VkDeviceMemoryDeleter
    {
        using pointer = VkDeviceMemory;
        VkDevice device;
        void operator()(VkDeviceMemory memory)
        {
            VkFreeMemory(device, memory, nullptr);
        }
    };
    using VkDeviceMemoryUnique = std::unique_ptr<VkDeviceMemory, VkDeviceMemoryDeleter>;

    struct VkBufferDeleter
    {
        using pointer = VkBuffer;
        VkDevice device;

        void operator()(VkBuffer buffer)
        {
            VkDestroyBuffer(device, buffer, nullptr);
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

}