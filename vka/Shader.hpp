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
    struct ShaderSpecialization
    {
        std::vector<VkSpecializationMapEntry> constantMapEntries;
        VkSpecializationInfo info;
    };
    
    static ShaderSpecialization MakeSpecialization(std::vector<VkSpecializationMapEntry>&& constantMapEntries, void* pConstantData)
    {
        ShaderSpecialization specialization;
        specialization.constantMapEntries = std::move(constantMapEntries);
        specialization.info.mapEntryCount = constantMapEntries.size();
        specialization.info.pMapEntries = constantMapEntries.data();
        std::for_each(
            constantMapEntries.begin(), 
            constantMapEntries.end(),
            [&specialization](const auto& mapEntry) { specialization.info.dataSize += mapEntry.size; });
        specialization.info.pData = pConstantData;
        return specialization;
    }

    struct ShaderData
    {
        ShaderSpecialization specialization;
        VkPipelineShaderStageCreateInfo shaderStageInfo;
    };

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

        template <typename T>
        ShaderData GetShaderData(
            std::vector<VkSpecializationMapEntry>&& constantMapEntries, 
            T& constantData)
        {
            ShaderData shaderData;
            shaderData.specialization = MakeSpecialization(
                std::move(constantMapEntries), (void*)(&constantData));
            shaderData.shaderStageInfo.sType = 
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderData.shaderStageInfo.pNext = nullptr;
            shaderData.shaderStageInfo.flags = 0;
            shaderData.shaderStageInfo.stage = stage;
            shaderData.shaderStageInfo.module = GetShaderModule();
            shaderData.shaderStageInfo.pName = entryPointName.c_str();
            shaderData.shaderStageInfo.pSpecializationInfo = 
                &shaderData.specialization.info;
            return shaderData;
        }

        ShaderData GetShaderData()
        {
            ShaderData shaderData;
            shaderData.shaderStageInfo.sType = 
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderData.shaderStageInfo.pNext = nullptr;
            shaderData.shaderStageInfo.flags = 0;
            shaderData.shaderStageInfo.stage = stage;
            shaderData.shaderStageInfo.module = GetShaderModule();
            shaderData.shaderStageInfo.pName = entryPointName.c_str();
            shaderData.shaderStageInfo.pSpecializationInfo = nullptr;
            return shaderData;
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