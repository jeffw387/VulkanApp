#pragma once

#include "UniqueVulkan.hpp"
#include "vulkan/vulkan.h"
#include "VulkanFunctions.hpp"

namespace vka
{
    class Instance
    {
	public:
		Instance(VkApplicationInfo&& applicationInfo, VkInstanceCreateInfo&& createInfo)
			:
			applicationInfo(std::move(applicationInfo)),
			createInfo(std::move(createInfo))
		{
			CreateInstance();
		}

		VkInstance GetInstance()
		{
			return instanceUnique.get();
		}
		
	private:
		VkApplicationInfo applicationInfo;
		VkInstanceCreateInfo createInfo;
        VkInstanceUnique instanceUnique;
		
		void CreateInstance()
		{
			createInfo.pApplicationInfo = &applicationInfo;

			VkInstance instance;
			auto result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
			instanceUnique = VkInstanceUnique(instance, VkInstanceDeleter());
		}
    };




    
}
