#pragma once

#include "vulkan/vulkan.h"
#include "UniqueVulkan.hpp"
#include "gsl.hpp"

#include <vector>
#include <tuple>
#include <algorithm>
#include <string>

namespace vka
{
    static VkSpecializationInfo MakeSpecialization(
        const gsl::span<VkSpecializationMapEntry> constantMapEntries, 
        const void* pConstantData)
    {
        VkSpecializationInfo specialization = {};
        specialization.mapEntryCount = constantMapEntries.size();
        specialization.pMapEntries = constantMapEntries.data();
        std::for_each(
            constantMapEntries.begin(), 
            constantMapEntries.end(),
            [&specialization](const auto& mapEntry) { specialization.dataSize += mapEntry.size; });
        specialization.pData = pConstantData;
        return specialization;
    }

    class Shader
    {
    public:
        Shader(VkDevice device,
            std::vector<char>&& shaderBinary,
            VkShaderStageFlagBits stage,
            std::string entryPointName)
            :
            device(device),
            shaderBinary(std::move(shaderBinary)),
            stage(stage),
            entryPointName(entryPointName)
        {
            CreateShaderModule();
        }

        Shader(Shader&&) = default;
        Shader& operator =(Shader&&) = default;

        VkShaderModule GetShaderModule()
        {
            return shaderUnique.get();
        }

        VkPipelineShaderStageCreateInfo GetShaderStageInfo(
            const VkSpecializationInfo* pSpecializationInfo)
        {
            VkPipelineShaderStageCreateInfo shaderStageInfo;
            shaderStageInfo.sType = 
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStageInfo.pNext = nullptr;
            shaderStageInfo.flags = 0;
            shaderStageInfo.stage = stage;
            shaderStageInfo.module = GetShaderModule();
            shaderStageInfo.pName = entryPointName.c_str();
            shaderStageInfo.pSpecializationInfo = pSpecializationInfo;
            return shaderStageInfo;
        }

    private:
        VkDevice device;
        std::vector<char> shaderBinary;
        VkShaderStageFlagBits stage;
        std::string entryPointName;
        VkShaderModuleCreateInfo shaderCreateInfo;
        VkShaderModuleUnique shaderUnique;
        
        void CreateShaderModule()
        {
            shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            shaderCreateInfo.pNext = nullptr;
            shaderCreateInfo.flags = 0;
            shaderCreateInfo.codeSize = shaderBinary.size();
            shaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shaderBinary.data());

            VkShaderModule shaderModule;
            vkCreateShaderModule(device, &shaderCreateInfo, nullptr, &shaderModule);
            shaderUnique = VkShaderModuleUnique(shaderModule, VkShaderModuleDeleter(device));
        }
    };
}