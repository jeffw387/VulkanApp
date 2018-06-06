#pragma once

#include "UniqueVulkan.hpp"
#include "vulkan/vulkan.h"
#include "Shader.hpp"
#include "VertexData.hpp"

#include <vector>

namespace vka
{
    class PipelineLayout
    {
    public:
        PipelineLayout(VkDevice device, 
            std::vector<VkDescriptorSetLayout> setLayouts,
            std::vector<VkPushConstantRange> pushConstantRanges)
            :
            device(device),
            setLayouts(setLayouts),
            pushConstantRanges(pushConstantRanges)
        {
            CreatePipelineLayout();
        }

        PipelineLayout(PipelineLayout&&) = default;
        PipelineLayout& operator =(PipelineLayout&&) = default;

        VkPipelineLayout GetPipelineLayout()
        {
            return pipelineLayoutUnique.get();
        }
    private:
        VkDevice device;
        std::vector<VkDescriptorSetLayout> setLayouts;
        std::vector<VkPushConstantRange> pushConstantRanges;
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
        VkPipelineLayoutUnique pipelineLayoutUnique;

        void CreatePipelineLayout()
        {
            VkPipelineLayout layout;
            pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutCreateInfo.pNext = nullptr;
            pipelineLayoutCreateInfo.flags = 0;
            pipelineLayoutCreateInfo.setLayoutCount = setLayouts.size();
            pipelineLayoutCreateInfo.pSetLayouts = setLayouts.data();
            pipelineLayoutCreateInfo.pushConstantRangeCount = pushConstantRanges.size();
            pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();
            vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &layout);
            pipelineLayoutUnique = VkPipelineLayoutUnique(layout, VkPipelineLayoutDeleter(device));
        }
    };

    class Pipeline
    {
    public:
        Pipeline(VkDevice device, 
            VkPipelineLayout pipelineLayout,
            VkRenderPass renderPass,
            VkPipelineShaderStageCreateInfo vertexShaderStageInfo,
            VkPipelineShaderStageCreateInfo fragmentShaderStageInfo,
            VertexData vertexData)
            :
            device(device),
            pipelineLayout(pipelineLayout),
            renderPass(renderPass),
            vertexShaderStageInfo(vertexShaderStageInfo),
            fragmentShaderStageInfo(fragmentShaderStageInfo),
            vertexData(vertexData)
        {
            CreatePipeline();
        }

        VkPipeline GetPipeline()
        {
            return pipelineUnique.get();
        }
    private:
        VkDevice device;
        VkPipelineLayout pipelineLayout;
        VkRenderPass renderPass;
        VkPipelineShaderStageCreateInfo vertexShaderStageInfo;
        VkPipelineShaderStageCreateInfo fragmentShaderStageInfo;
        VertexData vertexData;

        std::vector<VkPipelineShaderStageCreateInfo> shaderStageInfo;
		VkPipelineViewportStateCreateInfo viewportInfo;
		VkPipelineRasterizationStateCreateInfo rasterizationInfo;
		VkPipelineMultisampleStateCreateInfo multisampleInfo;
        VkPipelineColorBlendAttachmentState colorBlendAttachmentState;
		VkPipelineColorBlendStateCreateInfo blendStateInfo;
		std::vector<VkDynamicState> dynamicStates;
		VkPipelineDynamicStateCreateInfo dynamicStateInfo;
        VkGraphicsPipelineCreateInfo pipelineCreateInfo;
        VkPipelineUnique pipelineUnique;

        void CreatePipeline()
        {
            shaderStageInfo.push_back(vertexShaderStageInfo);
            shaderStageInfo.push_back(fragmentShaderStageInfo);

            viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportInfo.pNext = nullptr;
            viewportInfo.flags = 0;
            viewportInfo.viewportCount = 1;
            viewportInfo.pViewports = nullptr;
            viewportInfo.scissorCount = 1;
            viewportInfo.pScissors = nullptr;

            rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizationInfo.pNext = nullptr;
            rasterizationInfo.flags = 0;
            rasterizationInfo.depthClampEnable = false;
            rasterizationInfo.rasterizerDiscardEnable = false;
            rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
            rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
            rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            rasterizationInfo.depthBiasEnable = false;
            rasterizationInfo.depthBiasConstantFactor = 0.f;
            rasterizationInfo.depthBiasClamp = 0.f;
            rasterizationInfo.depthBiasSlopeFactor = 0.f;
            rasterizationInfo.lineWidth = 1.f;

            multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampleInfo.pNext = nullptr;
            multisampleInfo.flags = 0;
            multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            multisampleInfo.sampleShadingEnable = false;
            multisampleInfo.minSampleShading = 0.f;
            multisampleInfo.pSampleMask = nullptr;
            multisampleInfo.alphaToCoverageEnable = false;
            multisampleInfo.alphaToOneEnable = false;

            colorBlendAttachmentState.blendEnable = true;
            colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachmentState.colorWriteMask = 
                VK_COLOR_COMPONENT_R_BIT |
                VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT;

            blendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            blendStateInfo.pNext = nullptr;
            blendStateInfo.flags = 0;
            blendStateInfo.logicOpEnable = false;
            blendStateInfo.logicOp = VK_LOGIC_OP_CLEAR;
            blendStateInfo.attachmentCount = 1;
            blendStateInfo.pAttachments = &colorBlendAttachmentState;
            blendStateInfo.blendConstants[0] = 1.f;
            blendStateInfo.blendConstants[1] = 1.f;
            blendStateInfo.blendConstants[2] = 1.f;
            blendStateInfo.blendConstants[3] = 1.f;

            dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
            dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
            dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicStateInfo.pNext = nullptr;
            dynamicStateInfo.flags = 0;
            dynamicStateInfo.dynamicStateCount = dynamicStates.size();
            dynamicStateInfo.pDynamicStates = dynamicStates.data();

            auto vertexInputInfo = vertexData.GetVertexInputInfo();
            auto vertexInputAssemblyInfo = vertexData.GetInputAssemblyInfo();

            pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineCreateInfo.pNext = nullptr;
            pipelineCreateInfo.flags = 0;
            pipelineCreateInfo.stageCount = shaderStageInfo.size();
            pipelineCreateInfo.pStages = shaderStageInfo.data();
            pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
            pipelineCreateInfo.pInputAssemblyState = &vertexInputAssemblyInfo;
            pipelineCreateInfo.pTessellationState = nullptr;
            pipelineCreateInfo.pViewportState = &viewportInfo;
            pipelineCreateInfo.pRasterizationState = &rasterizationInfo;
            pipelineCreateInfo.pMultisampleState = &multisampleInfo;
            pipelineCreateInfo.pDepthStencilState = nullptr;
            pipelineCreateInfo.pColorBlendState = &blendStateInfo;
            pipelineCreateInfo.pDynamicState = &dynamicStateInfo;
            pipelineCreateInfo.layout = pipelineLayout;
            pipelineCreateInfo.renderPass = renderPass;
            pipelineCreateInfo.subpass = 0;
            pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
            pipelineCreateInfo.basePipelineIndex = 0;
            
            VkPipeline pipeline;
            vkCreateGraphicsPipelines(
                device,
                VK_NULL_HANDLE,
                1,
                &pipelineCreateInfo,
                nullptr,
                &pipeline);
            pipelineUnique = VkPipelineUnique(pipeline, VkPipelineDeleter(device));
        }
    };
}