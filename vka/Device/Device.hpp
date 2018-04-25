#pragma once

#include "vulkan/vulkan.h"
#include "UniqueVulkan.hpp"

#include <tuple>
#include <vector>
#include <functional>
#include <optional>
#include <algorithm>
#include <stdexcept>

namespace vka
{
    class Device
    {
    public:
        Device(VkInstance instance, VkSurfaceKHR surface, std::vector<const char*> deviceExtensions) : 
            m_Instance(instance), m_Surface(surface)
        {
            SelectPhysicalDevice();
            GetQueueFamilyProperties();
            SelectGraphicsQueue();
            SelectPresentQueue();
            CreateDevice();
        }

        VkDevice get()
        {
            return m_DeviceUnique.get();
        }

        VkQueue GetGraphicsQueue()
        {
            return m_GraphicsQueue;
        }

        VkQueue GetPresentQueue()
        {
            return m_PresentQueue;
        }

        uint32_t GetGraphicsQueueID()
        {
            return m_GraphicsQueueCreateInfo.queueFamilyIndex;
        }

        uint32_t GetPresentQueueID()
        {
            return m_PresentQueueCreateInfo.queueFamilyIndex;
        }
        
    private:
        VkInstance m_Instance;
        VkPhysicalDevice m_PhysicalDevice;
        VkSurfaceKHR m_Surface;
        std::vector<VkQueueFamilyProperties> m_QueueFamilyProperties;
        const float m_GraphicsQueuePriority = 0.f;
        VkDeviceQueueCreateInfo m_GraphicsQueueCreateInfo;
        const float m_PresentQueuePriority = 0.f;
        VkDeviceQueueCreateInfo m_PresentQueueCreateInfo;
        std::vector<VkDeviceQueueCreateInfo> m_QueueCreateInfos;
        VkDeviceCreateInfo m_CreateInfo;
        VkDeviceUnique m_DeviceUnique;
        VkQueue m_GraphicsQueue;
        VkQueue m_PresentQueue;

        void SelectPhysicalDevice()
        {

        }

        void GetQueueFamilyProperties()
        {
            uint32_t propCount;
            vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &propCount, nullptr);
            m_QueueFamilyProperties.resize(propCount);
            vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, 
                &propCount, 
                m_QueueFamilyProperties.data());
        }

        void SelectGraphicsQueue()
        {
            std::optional<uint32_t> graphicsQueueFamilyIndex;
            for (uint32_t i = 0; i < m_QueueFamilyProperties.size(); ++i)
            {
                if (m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    graphicsQueueFamilyIndex = i;
                    break;
                }
            }

            if (graphicsQueueFamilyIndex.has_value() == false)
            {
                std::runtime_error("Device does not support graphics queue!");
            }

            m_GraphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            m_GraphicsQueueCreateInfo.pNext = nullptr;
            m_GraphicsQueueCreateInfo.flags = 0;
            m_GraphicsQueueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex.value();
            m_GraphicsQueueCreateInfo.queueCount = 1;
            m_GraphicsQueueCreateInfo.pQueuePriorities = &m_GraphicsQueuePriority;
        }

        void SelectPresentQueue()
        {
            std::optional<uint32_t> presentQueueFamilyIndex;
            for (uint32_t i = 0; i < m_QueueFamilyProperties.size(); ++i)
            {
                bool presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, i, surface, &presentSupport);
                if (presentSupport)
                {
                    presentQueueFamilyIndex = i;
                    break;
                }
            }

            if (presentQueueFamilyIndex.has_value() == false)
            {
                std::runtime_error("Device does not support present queue!");
            }

            m_PresentQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            m_PresentQueueCreateInfo.pNext = nullptr;
            m_PresentQueueCreateInfo.flags = 0;
            m_PresentQueueCreateInfo.queueFamilyIndex = presentQueueFamilyIndex.value();
            m_PresentQueueCreateInfo.queueCount = 1;
            m_PresentQueueCreateInfo.pQueuePriorities = &m_PresentQueuePriority;
        }

        void CreateDevice()
        {
            m_QueueCreateInfos.push_back(m_GraphicsQueueCreateInfo);
            if (m_GraphicsQueueCreateInfo.queueFamilyIndex != m_PresentQueueCreateInfo.queueFamilyIndex)
            {
                m_QueueCreateInfos.push_back(m_PresentQueueCreateInfo);
            }

            m_CreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            m_CreateInfo.pNext = nullptr;
            m_CreateInfo.flags = 0;
            m_CreateInfo.m_GraphicsQueueCreateInfoCount = m_QueueCreateInfos.size();
            m_CreateInfo.pQueueCreateInfos = m_QueueCreateInfos.data();
            m_CreateInfo.enabledLayerCount = 0;
            m_CreateInfo.ppEnabledLayerNames = nullptr;
            m_CreateInfo.enabledExtensionCount = deviceExtensions.size();
            m_CreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
            m_CreateInfo.pEnabledFeatures = nullptr;
            VkDevice device;
            vkCreateDevice(m_PhysicalDevice, &m_CreateInfo, nullptr, &device);
            m_DeviceUnique = VkDeviceUnique(device, VkDeviceDeleter());

            vkGetDeviceQueue(get(), GetGraphicsQueueID(), 0, &m_GraphicsQueue);
            vkGetDeviceQueue(get(), GetPresentQueueID(), 0, &m_PresentQueue);
        }
    };
}