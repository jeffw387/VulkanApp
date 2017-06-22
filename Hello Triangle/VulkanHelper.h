#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

VkResult CreateDebugReportCallbackEXT(
	VkInstance instance, 
	const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkDebugReportCallbackEXT* pCallback
)
{
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if(func != nullptr)
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
	if(func != nullptr)
	{
		func(instance, callback, pAllocator);
	}
}

// Wrapper functions for aligned memory allocation
// There is currently no standard for this in C++ that works across all platforms and vendors, so we abstract this
void* alignedAlloc(size_t size, size_t alignment)
{
	void *data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
	data = _aligned_malloc(size, alignment);
#else 
	int res = posix_memalign(&data, alignment, size);
	if (res != 0)
		data = nullptr;
#endif
	return data;
}

void alignedFree(void* data)
{
#if	defined(_MSC_VER) || defined(__MINGW32__)
	_aligned_free(data);
#else 
	free(data);
#endif
}

template <typename T>
class VDeleter
{
public:
	VDeleter() : VDeleter([] (T, VKAllocationCallbacks*) {}) {}

	VDeleter(std::function<void(T, VkAllocationCallbacks*)> deletef) { this->deleter = [=] (T obj) { deletef(obj, nullptr); }; }

	VDeleter(const VDeleter<VkInstance>& instance, 
		std::function<void(VkInstance, T, VkAllocationCallbacks*)> deletef) 
	{ this->deleter = [&instance, deletef] (T obj) { deletef(instance, obj, nullptr); }; }

	VDeleter(const VDeleter<VkDevice>& device, 
		std::function<void(VkDevice, T, VkAllocationCallbacks*)> deletef) 
	{ this->deleter = [&device, deletef] (T obj) { deletef(device, obj, nullptr); }; }

	~VDeleter() { cleanup(); }

	const T* operator &() const { return &object; }

	T* replace() { cleanup(); return &object; }

	operator T() const
	{
		return object;
	}

	void operator=(T rhs) { if(rhs != object) { cleanup(); object = rhs; } }

	template<typename V> bool operator==(V rhs) { return object == T(rhs); }

	void cleanup() { if(object != VK_NULL_HANDLE) { deleter(object); } object = VK_NULL_HANDLE; }
private: 
	T object {VK_NULL_HANDLE}; 
	std::function<void(T)> deleter;
};

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