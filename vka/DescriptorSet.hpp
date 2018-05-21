#pragma once

#include "vulkan/vulkan.h"
#include "UniqueVulkan.hpp"

#include <vector>
#include <algorithm>

namespace vka
{
    class DescriptorSet
    {
    public:
        DescriptorSet(VkDevice device, 
            std::vector<VkDescriptorSetLayoutBinding>&& bindings, 
            std::vector<VkDescriptorPoolSize>&& poolSizes, 
            uint32_t maxSets)
            :
            device(device),
            bindings(std::move(bindings)),
            poolSizes(std::move(poolSizes)),
            maxSets(maxSets)
        {
            CreateDescriptorSetLayout();
            CreateDescriptorPool();
            SetupAllocation();            
        }

        std::vector<size_t> AllocateSets(size_t count)
        {
            auto currentSets = sets.size();
            auto setsAvailable = maxSets - currentSets;
            auto allocateCount = std::min(count, setsAvailable);
            std::vector<VkDescriptorSet> newSets;
            std::vector<VkDescriptorSetLayout> newSetLayouts;
            newSetLayouts.resize(allocateCount);
            std::fill(newSetLayouts.begin(), newSetLayouts.end(), layoutUnique.get());
            newSets.resize(allocateCount);
            allocateInfo.descriptorSetCount = allocateCount;
            allocateInfo.pSetLayouts = newSetLayouts.data();
            vkAllocateDescriptorSets(device, &allocateInfo, newSets.data());
            std::vector<size_t> setIDs;
            setIDs.resize(allocateCount);
            for (auto newSet : newSets)
            {
                setIDs.push_back(currentSets);
                sets.push_back(newSet);
            }
            return setIDs;
        }

        VkDescriptorSet GetSet(size_t setID)
        {
            return sets[setID];
        }

        VkDescriptorPool GetPool()
        {
            return poolUnique.get();
        }

        VkDescriptorSetLayout GetSetLayout()
        {
            return layoutUnique.get();
        }

        void ResetPool()
        {
            vkResetDescriptorPool(device, poolUnique.get(), 0);
        }

    private:
        VkDevice device;
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        std::vector<VkDescriptorPoolSize> poolSizes;
        size_t maxSets;

        VkDescriptorSetLayoutCreateInfo layoutCreateInfo;
        VkDescriptorSetLayoutUnique layoutUnique;
        VkDescriptorPoolCreateInfo poolCreateInfo;
        VkDescriptorPoolUnique poolUnique;
        VkDescriptorSetAllocateInfo allocateInfo;
        std::vector<VkDescriptorSet> sets;

        void CreateDescriptorSetLayout()
        {
            layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutCreateInfo.pNext = nullptr;
            layoutCreateInfo.flags = 0;
            layoutCreateInfo.bindingCount = bindings.size();
            layoutCreateInfo.pBindings = bindings.data();
            VkDescriptorSetLayout layout;
            vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &layout);
            layoutUnique = VkDescriptorSetLayoutUnique(layout, VkDescriptorSetLayoutDeleter(device));
        }

        void CreateDescriptorPool()
        {
            poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolCreateInfo.pNext = nullptr;
            poolCreateInfo.flags = 0;
            poolCreateInfo.maxSets = maxSets;
            poolCreateInfo.poolSizeCount = poolSizes.size();
            poolCreateInfo.pPoolSizes = poolSizes.data();
            VkDescriptorPool pool;
            vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &pool);
            poolUnique = VkDescriptorPoolUnique(pool, VkDescriptorPoolDeleter(device));
        }

        void SetupAllocation()
        {
            allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocateInfo.pNext = nullptr;
            allocateInfo.descriptorPool = poolUnique.get();
            allocateInfo.descriptorSetCount = 0;
            allocateInfo.pSetLayouts = nullptr;
        }
    };
}