#pragma once

#include "vulkan/vulkan.h"
#include "UniqueVulkan.hpp"

#include <vector>

namespace vka
{
    class DescriptorSet
    {
        VkDevice m_Device = 0;
        VkDescriptorSetLayoutCreateInfo m_LayoutCreateInfo = {};
        VkDescriptorSetLayoutUnique m_Layout;
        VkDescriptorPoolCreateInfo m_PoolCreateInfo = {};
        VkDescriptorPoolUnique m_Pool;
        VkDescriptorSetAllocateInfo m_AllocateInfo = {};
        std::vector<VkDescriptorSet> m_Sets;
        size_t m_MaxSets = 0;
        size_t m_CurrentSets = 0;

    public:
        DescriptorSet(VkDevice device, std::vector<VkDescriptorSetLayoutBinding> bindings, 
            std::vector<VkDescriptorPoolSizes> poolSizes, 
            VkDescriptorPoolCreateFlags poolFlags, 
            uint32_t maxSets)
        {
            m_Device = device;
            m_MaxSets = maxSets;

            m_LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            m_LayoutCreateInfo.pNext = nullptr;
            m_LayoutCreateInfo.flags = 0;
            m_LayoutCreateInfo.bindingCount = bindings.size();
            m_LayoutCreateInfo.pBindings = bindings.data();
            VkDescriptorSetLayout layout;
            vkCreateDescriptorSetLayout(device, &m_LayoutCreateInfo, nullptr, &layout);
            m_Layout = VkDescriptorSetLayoutUnique(layout, VkDescriptorSetLayoutDeleter(device));

            m_PoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            m_PoolCreateInfo.pNext = nullptr;
            m_PoolCreateInfo.flags = poolFlags;
            m_PoolCreateInfo.maxSets = maxSets;
            m_PoolCreateInfo.poolSizeCount = poolSizes.size();
            m_PoolCreateInfo.pPoolSizes = poolSizes.data();
        }
        std::vector<size_t> AllocateSets(size_t count);
        VkDescriptorSet GetSet(size_t setID);
        void ResetPool();
    };
}