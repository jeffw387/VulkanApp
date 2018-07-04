#pragma once

#include "vulkan/vulkan.h"
#include "UniqueVulkan.hpp"
#include "Results.hpp"
#include "fileIO.hpp"
#include "Allocator.hpp"
#include "VulkanFunctionLoader.hpp"
#include "gsl.hpp"
#include "nlohmann/json.hpp"
#include "jsonConfig.hpp"

#include <tuple>
#include <vector>
#include <functional>
#include <optional>
#include <algorithm>
#include <stdexcept>
#include <string>

namespace vka
{
	using json = nlohmann::json;
	constexpr float graphicsQueuePriority = 0.f;
	constexpr float presentQueuePriority = 0.f;

	class Device
	{
	public:
		static constexpr uint32_t ImageCount = 1;
		static constexpr uint32_t PushConstantSize = sizeof(PushConstants);

		Device(
			VkInstance instance,
			std::vector<const char *> deviceExtensions,
			VkSurfaceKHR surface)
			: instance(instance),
			deviceExtensions(deviceExtensions),
			surface(surface)
		{
			SelectPhysicalDevice();
			GetQueueFamilyProperties();
			SelectGraphicsQueue();
			SelectPresentQueue();
			CreateDevice();
			CreateAllocator();
		}

		VkPhysicalDevice GetPhysicalDevice()
		{
			return physicalDevice;
		}

		VkDevice GetDevice()
		{
			return device;
		}

		auto &GetAllocator()
		{
			return allocator;
		}

		VkQueue GetGraphicsQueue()
		{
			return graphicsQueue;
		}

		VkQueue GetPresentQueue()
		{
			return presentQueue;
		}

		uint32_t GetGraphicsQueueID()
		{
			return graphicsQueueCreateInfo.queueFamilyIndex;
		}

		uint32_t GetPresentQueueID()
		{
			return presentQueueCreateInfo.queueFamilyIndex;
		}

		VkFence CreateFence(bool signaled)
		{
			VkFence fence;
			VkFenceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			createInfo.pNext = nullptr;
			if (signaled)
			{
				createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			}
			vkCreateFence(GetDevice(), &createInfo, nullptr, &fence);
			fences[fence] = VkFenceUnique(fence, VkFenceDeleter(GetDevice()));
			return fence;
		}

		VkSemaphore CreateSemaphore()
		{
			VkSemaphore semaphore;
			VkSemaphoreCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			createInfo.pNext = nullptr;
			vkCreateSemaphore(GetDevice(), &createInfo, nullptr, &semaphore);
			semaphores[semaphore] =
				VkSemaphoreUnique(semaphore, VkSemaphoreDeleter(GetDevice()));
			return semaphore;
		}

		VkCommandPool CreateCommandPool(
			uint32_t queueFamilyIndex, 
			bool transient, 
			bool poolReset)
		{
			VkCommandPool pool;
			VkCommandPoolCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.flags |= transient ? VK_COMMAND_POOL_CREATE_TRANSIENT_BIT : 0;
			createInfo.flags |= poolReset ? VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT : 0;
			createInfo.queueFamilyIndex = queueFamilyIndex;
			vkCreateCommandPool(GetDevice(), &createInfo, nullptr, &pool);
			commandPools[pool] = VkCommandPoolUnique(pool, VkCommandPoolDeleter(GetDevice()));
			return pool;
		}

		std::vector<VkCommandBuffer> AllocateCommandBuffers(VkCommandPool pool, uint32_t count)
		{
			std::vector<VkCommandBuffer> commandBuffers;
			commandBuffers.resize(count);
			VkCommandBufferAllocateInfo allocateInfo = {};
			allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocateInfo.pNext = nullptr;
			allocateInfo.commandPool = pool;
			allocateInfo.level = (VkCommandBufferLevel)0;
			allocateInfo.commandBufferCount = count;
			vkAllocateCommandBuffers(GetDevice(), &allocateInfo, commandBuffers.data());
			return commandBuffers;
		}

		VkRenderPass CreateRenderPass(
			const json &renderPassConfig)
		{
			std::vector<VkAttachmentDescription> attachmentDescriptions;
			for (const auto& attachmentJson : renderPassConfig["attachments"])
			{
				auto& description = attachmentDescriptions.emplace_back(VkAttachmentDescription());
				description.initialLayout = attachmentJson["initialLayout"];
				description.finalLayout = attachmentJson["finalLayout"];
				description.samples = attachmentJson["samples"];
				description.format = attachmentJson["format"];
				description.loadOp = attachmentJson["loadOp"];
				description.storeOp = attachmentJson["storeOp"];
				description.stencilLoadOp = attachmentJson["stencilLoadOp"];
				description.stencilStoreOp = attachmentJson["stencilStoreOp"];
			}

			std::vector<VkSubpassDescription> subpassDescriptions;
			struct SubpassData
			{
				std::vector<VkAttachmentReference> colorAttachments;
				std::optional<VkAttachmentReference> depthStencilAttachment;
				std::vector<VkAttachmentReference> inputAttachments;
				std::vector<VkAttachmentReference> resolveAttachments;
				std::vector<uint32_t> preserveAttachments;
			};
			std::vector<SubpassData> subpassDatas;
			auto subpassIndex = 0U;
			for (const auto& subpassJson : renderPassConfig["subpasses"])
			{
				auto& subpass = subpassDescriptions.emplace_back(VkSubpassDescription());
				auto& subpassData = subpassDatas.emplace_back(SubpassData());

				for (const auto& colorAttachmentJson : subpassJson["colorAttachments"])
				{
					auto& colorAttachment = subpassData.colorAttachments.emplace_back(
						VkAttachmentReference());
					colorAttachment.attachment = colorAttachmentJson["attachment"];
					colorAttachment.layout = colorAttachmentJson["layout"];
				}
				subpass.colorAttachmentCount = gsl::narrow<uint32_t>(subpassData.colorAttachments.size());
				subpass.pColorAttachments = subpassData.colorAttachments.data();

				auto depthStencilAttachmentJson = subpassJson["depthStencilAttachment"];
				if (!depthStencilAttachmentJson.is_null())
				{
					subpassData.depthStencilAttachment = VkAttachmentReference();
					subpassData.depthStencilAttachment.value().attachment = depthStencilAttachmentJson["attachment"];
					subpassData.depthStencilAttachment.value().layout = depthStencilAttachmentJson["layout"];
				}
				subpass.pDepthStencilAttachment = &subpassData.depthStencilAttachment.value_or(nullptr);

				for (const auto& inputAttachmentJson : subpassJson["inputAttachments"])
				{
					auto& inputAttachment = subpassData.inputAttachments.emplace_back(
						VkAttachmentReference());
					inputAttachment.attachment = inputAttachmentJson["attachment"];
					inputAttachment.layout = inputAttachmentJson["layout"];
				}
				subpass.inputAttachmentCount = gsl::narrow<uint32_t>(subpassData.inputAttachments.size());
				subpass.pInputAttachments = subpassData.inputAttachments.data();

				for (const auto& resolveAttachmentJson : subpassJson["resolveAttachments"])
				{
					auto& resolveAttachment = subpassData.resolveAttachments.emplace_back(
						VkAttachmentReference());
					resolveAttachment.attachment = resolveAttachmentJson["attachment"];
					resolveAttachment.layout = resolveAttachmentJson["layout"];
				}
				if (subpassData.resolveAttachments.size() > 0)
				{
					subpass.pResolveAttachments = subpassData.resolveAttachments.data();
				}

				for (const auto& preserveAttachmentJson : subpassJson["preserveAttachments"])
				{
					auto& preserveAttachment = subpassData.preserveAttachments.emplace_back(
						VkAttachmentReference());
					preserveAttachment = preserveAttachmentJson;
				}
				subpass.preserveAttachmentCount = gsl::narrow<uint32_t>(subpassData.preserveAttachments.size());
				subpass.pPreserveAttachments = subpassData.preserveAttachments.data();

				++subpassIndex;
			}

			std::vector<VkSubpassDependency> subpassDependencies;
			for (const auto& dependencyJson : renderPassConfig["dependencies"])
			{
				auto& dependency = subpassDependencies.emplace_back(VkSubpassDependency());
				for (const auto& flag : dependencyJson["dependencyFlags"])
				{
					dependency.dependencyFlags |= static_cast<VkDependencyFlags>(flag);
				}
				for (const auto& maskBit : dependencyJson["srcAccessMask"])
				{
					dependency.srcAccessMask |= static_cast<VkAccessFlags>(maskBit);
				}
				for (const auto& maskBit : dependencyJson["dstAccessMask"])
				{
					dependency.dstAccessMask |= maskBit;
				}
				for (const auto& maskBit : dependencyJson["srcStageMask"])
				{
					dependency.srcStageMask |= maskBit;
				}
				for (const auto& maskBit : dependencyJson["dstStageMask"])
				{
					dependency.dstStageMask |= maskBit;
				}
				dependency.srcSubpass = dependencyJson["srcSubpass"];
				dependency.dstSubpass = dependencyJson["dstSubpass"];
			}
			VkRenderPassCreateInfo createInfo = {};
			createInfo.attachmentCount = gsl::narrow<uint32_t>(
				attachmentDescriptions.size());
			createInfo.pAttachments = attachmentDescriptions.data();
			createInfo.subpassCount = gsl::narrow<uint32_t>(
				subpassDescriptions.size());
			createInfo.pSubpasses = subpassDescriptions.data();
			createInfo.dependencyCount = gsl::narrow<uint32_t>(
				subpassDependencies.size());
			createInfo.pDependencies = subpassDependencies.data();

			VkRenderPass renderPass;
			vkCreateRenderPass(device, &createInfo, nullptr, &renderPass);

			renderPasses[renderPass] = VkRenderPassUnique(renderPass, VkRenderPassDeleter(device));

			return renderPass;
		}

		VkSwapchainKHR CreateSwapchain(
			const VkSurfaceKHR& surface,
			const uint32_t& graphicsQueueFamilyID,
			const uint32_t& presentQueueFamilyID,
			const json& swapchainConfig)
		{
			VkSwapchainCreateInfoKHR createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			createInfo.pNext = nullptr;
			for (const auto& flagBit : swapchainConfig["flags"])
			{
				createInfo.flags |= flagBit;
			}
			createInfo.clipped = swapchainConfig["clipped"];
			createInfo.compositeAlpha = swapchainConfig["compositeAlpha"];
			createInfo.imageArrayLayers = swapchainConfig["imageArrayLayers"];
			createInfo.imageColorSpace = swapchainConfig["imageColorSpace"];
			createInfo.imageExtent = swapchainConfig["imageExtent"];
			createInfo.imageFormat = swapchainConfig["imageFormat"];
			std::vector<uint32_t> queueFamilyIndices;
			if (graphicsQueueFamilyID == presentQueueFamilyID)
			{
				createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			}
			else
			{
				createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
				queueFamilyIndices.push_back(graphicsQueueFamilyID);
				queueFamilyIndices.push_back(presentQueueFamilyID);
			}
			createInfo.imageUsage = swapchainConfig["imageUsage"];
			createInfo.minImageCount = swapchainConfig["minImageCount"];
			createInfo.queueFamilyIndexCount = gsl::narrow<uint32_t>(queueFamilyIndices.size());
			createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
			// TODO: add check in for present mode support here
			createInfo.presentMode = swapchainConfig["presentMode"];
			createInfo.preTransform = swapchainConfig["preTransform"];
			createInfo.surface = surface;
			createInfo.oldSwapchain = VK_NULL_HANDLE;

			VkSwapchainKHR swapchain;
			vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain);
			swapchains[swapchain] = VkSwapchainKHRUnique(swapchain, VkSwapchainKHRDeleter(device));

			return swapchain;
		}

		VkShaderModule CreateShaderModule(
			const json& shaderJson)
		{
			auto shaderJsonModule = GetModule(shaderJson);
			auto shaderBinary = fileIO::readFile(shaderJsonModule["module"]);
			VkShaderModuleCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.flags = 0;
			createInfo.codeSize = shaderBinary.size();
			createInfo.pCode = reinterpret_cast<const uint32_t *>(shaderBinary.data());

			VkShaderModule shaderModule;
			vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
			shaderModules[shaderModule] = VkShaderModuleUnique(shaderModule, VkShaderModuleDeleter(device));

			return shaderModule;
		}

		VkSampler CreateSampler(
			const json& samplerJson)
		{
			VkSamplerCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.flags = 0;
			createInfo.magFilter = samplerJson["magFilter"];
			createInfo.minFilter = samplerJson["minFilter"];
			createInfo.mipmapMode = samplerJson["mipmapMode"];
			createInfo.addressModeU = samplerJson["addressModeU"];
			createInfo.addressModeV = samplerJson["addressModeV"];
			createInfo.addressModeW = samplerJson["addressModeW"];
			createInfo.mipLodBias = samplerJson["mipLodBias"];
			createInfo.anisotropyEnable = samplerJson["anistropyEnable"];
			createInfo.maxAnisotropy = samplerJson["maxAnistropy"];
			createInfo.compareEnable = samplerJson["compareEnable"];
			createInfo.compareOp = samplerJson["compareOp"];
			createInfo.minLod = samplerJson["minLod"];
			createInfo.maxLod = samplerJson["maxLod"];
			createInfo.borderColor = samplerJson["borderColor"];
			createInfo.unnormalizedCoordinates = samplerJson["unnormalizedCoordinates"];

			VkSampler sampler;
			vkCreateSampler(GetDevice(), &createInfo, nullptr, &sampler);
			samplers[sampler] = VkSamplerUnique(sampler, VkSamplerDeleter(GetDevice()));

			return sampler;
		}

		VkDescriptorSetLayout CreateDescriptorSetLayout(
			const VkSampler& sampler,
			const json& descriptorSetLayoutJson,
			const std::vector<VkSampler>& immutableSamplers)
		{
			VkDescriptorSetLayoutCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			createInfo.pNext = nullptr;

			std::vector<VkDescriptorSetLayoutBinding> bindings;
			for (const auto& bindingJson : descriptorSetLayoutJson["bindings"])
			{
				auto bindingModule = GetModule(bindingJson);
				auto& binding = bindings.emplace_back(VkDescriptorSetLayoutBinding());
				binding.binding = bindingModule["binding"];
				binding.descriptorType = bindingModule["descriptorType"];
				binding.descriptorCount = bindingModule["descriptorCount"];
				for (const auto& flag : bindingModule["stageFlags"])
				{
					binding.stageFlags |= flag;
				}
				if (immutableSamplers.size() > 0)
				{
					binding.pImmutableSamplers = immutableSamplers.data();
				}
			}
			createInfo.bindingCount = gsl::narrow<uint32_t>(bindings.size());
			createInfo.pBindings = bindings.data();
			for (const auto& flag : descriptorSetLayoutJson["flags"])
			{
				createInfo.flags |= flag;
			}

			VkDescriptorSetLayout setLayout;
			vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &setLayout);
			descriptorSetLayouts[setLayout] = VkDescriptorSetLayoutUnique(setLayout, VkDescriptorSetLayoutDeleter(device));

			return setLayout;
		}

		VkPipelineLayout CreatePipelineLayout(
			const std::vector<VkDescriptorSetLayout>& setLayouts,
			const json& pipelineLayoutJson)
		{
			VkPipelineLayoutCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			createInfo.pNext = nullptr;
			std::vector<VkPushConstantRange> pushConstantRanges;
			for (const auto& rangeJson : pipelineLayoutJson["pushConstantRanges"])
			{
				auto& range = pushConstantRanges.emplace_back(VkPushConstantRange());
				range.offset = rangeJson["offset"];
				range.size = rangeJson["size"];
				for (const auto& flag : rangeJson["stage"])
				{
					range.stageFlags |= flag;
				}
			}
			createInfo.setLayoutCount = gsl::narrow<uint32_t>(setLayouts.size());
			createInfo.pSetLayouts = setLayouts.data();
			createInfo.pushConstantRangeCount = gsl::narrow<uint32_t>(pushConstantRanges.size());
			createInfo.pPushConstantRanges = pushConstantRanges.data();
			createInfo.flags = 0;

			VkPipelineLayout pipelineLayout;
			vkCreatePipelineLayout(device, &createInfo, nullptr, &pipelineLayout);
			pipelineLayouts[pipelineLayout] = VkPipelineLayoutUnique(pipelineLayout, VkPipelineLayoutDeleter(device));

			return pipelineLayout;
		}

		VkPipeline CreateGraphicsPipeline(
			const VkPipelineLayout& layout,
			const VkRenderPass& renderPass,
			std::map<std::string, VkShaderModule>& shaderModules,
			std::map<VkShaderModule, gsl::span<gsl::byte>>& shaderSpecializationData,
			const json& pipelineJson)
		{
			auto colorBlendConfigJson = pipelineJson["colorBlendConfig"];
			VkPipelineColorBlendStateCreateInfo colorBlendState = {};
			colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlendState.pNext = nullptr;
			colorBlendState.logicOpEnable = colorBlendConfigJson["logicOpEnable"];
			colorBlendState.logicOp = colorBlendConfigJson["logicOp"];
			for (auto i = 0U; i < 4; ++i)
			{
				colorBlendState.blendConstants[i] = colorBlendConfigJson["blendConstants"][i];
			}
			std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
			for (const auto& colorBlendJson : colorBlendConfigJson["colorBlendAttachments"])
			{
				auto& colorBlendAttachment = colorBlendAttachments.emplace_back(VkPipelineColorBlendAttachmentState());
				colorBlendAttachment.blendEnable = colorBlendJson["blendEnable"];
				colorBlendAttachment.colorBlendOp = colorBlendJson["colorBlendOp"];
				colorBlendAttachment.srcColorBlendFactor = colorBlendJson["srcColorBlendFactor"];
				colorBlendAttachment.dstColorBlendFactor = colorBlendJson["dstColorBlendFactor"];
				colorBlendAttachment.alphaBlendOp = colorBlendJson["alphaBlendOp"];
				colorBlendAttachment.srcAlphaBlendFactor = colorBlendJson["srcAlphaBlendFactor"];
				colorBlendAttachment.dstAlphaBlendFactor = colorBlendJson["dstAlphaBlendFactor"];
				auto colorWriteMaskJson = colorBlendJson["colorWriteMask"];
				if (colorWriteMaskJson["R"])
					colorBlendAttachment.colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
				if (colorWriteMaskJson["G"])
					colorBlendAttachment.colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
				if (colorWriteMaskJson["B"])
					colorBlendAttachment.colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
				if (colorWriteMaskJson["A"])
					colorBlendAttachment.colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;
			}
			colorBlendState.attachmentCount = gsl::narrow<uint32_t>(colorBlendAttachments.size());
			colorBlendState.pAttachments = colorBlendAttachments.data();

			auto depthStencilJson = pipelineJson["depthStencilConfig"];
			VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
			depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencilState.pNext = nullptr;
			depthStencilState.depthTestEnable = depthStencilJson["depthTestEnable"];
			depthStencilState.depthWriteEnable = depthStencilJson["depthWriteEnable"];
			depthStencilState.depthBoundsTestEnable = depthStencilJson["depthBoundsTestEnable"];
			depthStencilState.depthCompareOp = depthStencilJson["depthCompareOp"];
			depthStencilState.stencilTestEnable = depthStencilJson["stencilTestEnable"];
			depthStencilState.minDepthBounds = depthStencilJson["minDepthBounds"];
			depthStencilState.maxDepthBounds = depthStencilJson["maxDepthBounds"];

			auto shaderStagesJson = pipelineJson["shaderStageConfigs"];
			std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
			std::map<VkShaderModule, std::string> shaderEntryPoints;
			std::map<VkShaderModule, VkSpecializationInfo> shaderSpecializationInfos;
			std::map<VkShaderModule, std::vector<VkSpecializationMapEntry>> shaderSpecializationMapEntries;
			for (const auto& shaderStageJson : shaderStagesJson)
			{
				std::string uri = shaderStageJson["module"]["jsonID"]["uri"];
				std::string entryPoint = shaderStageJson["name"];
				VkShaderModule shaderModule = shaderModules[uri];
				shaderEntryPoints[shaderModule] = entryPoint;

				VkSpecializationInfo specInfo = {};
				specInfo.dataSize = shaderSpecializationData[shaderModule].length_bytes();
				specInfo.pData = shaderSpecializationData[shaderModule].begin();
				for (const auto& mapEntryJson : shaderStageJson["specializationMapEntries"])
				{
					VkSpecializationMapEntry mapEntry = {};
					mapEntry.constantID = mapEntryJson["constantID"];
					mapEntry.offset = mapEntryJson["offset"];
					mapEntry.size = mapEntryJson["size"];
					shaderSpecializationMapEntries[shaderModule].push_back(mapEntry);
				}
				specInfo.mapEntryCount = gsl::narrow<uint32_t>(shaderSpecializationMapEntries[shaderModule].size());
				specInfo.pMapEntries = shaderSpecializationMapEntries[shaderModule].data();
				shaderSpecializationInfos[shaderModule] = specInfo;

				auto& shaderStage = shaderStages.emplace_back(VkPipelineShaderStageCreateInfo());
				shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStage.pNext = nullptr;
				shaderStage.module = shaderModule;
				shaderStage.pSpecializationInfo = &shaderSpecializationInfos[shaderModule];
				shaderStage.pName = shaderEntryPoints[shaderModule].data();
				shaderStage.stage = shaderStageJson["stage"];
			}

			auto dynamicStatesJson = pipelineJson["dynamicStates"];
			std::vector<VkDynamicState> dynamicStates;
			for (const auto& dynamicStateJson : dynamicStatesJson)
			{
				dynamicStates.push_back(dynamicStateJson);
			}
			VkPipelineDynamicStateCreateInfo dynamicState = {};
			dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicState.pNext = nullptr;
			dynamicState.dynamicStateCount = gsl::narrow<uint32_t>(dynamicStates.size());
			dynamicState.pDynamicStates = dynamicStates.data();

			auto inputAssemblyJson = pipelineJson["inputAssemblyConfig"];
			VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
			inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssemblyState.pNext = nullptr;
			inputAssemblyState.primitiveRestartEnable = inputAssemblyJson["primitiveRestartEnable"];
			inputAssemblyState.topology = inputAssemblyJson["topology"];

			VkPipelineMultisampleStateCreateInfo multisampleState = {};
			if (pipelineJson.find("multisampleConfig") != pipelineJson.end())
			{
				auto multisampleJson = pipelineJson["multisampleConfig"];
				VkSampleMask sampleMask = {};
				multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
				multisampleState.pNext = nullptr;
				multisampleState.sampleShadingEnable = multisampleJson["sampleShadingEnable"];
				multisampleState.minSampleShading = multisampleJson["minSampleShading"];
				multisampleState.rasterizationSamples = multisampleJson["rasterizationSamples"];
				multisampleState.alphaToCoverageEnable = multisampleJson["alphaToCoverageEnable"];
				multisampleState.alphaToOneEnable = multisampleJson["alphaToOneEnable"];
				multisampleState.pSampleMask = &sampleMask;
			}

			auto rasterizationJson = pipelineJson["rasterizationConfig"];
			VkPipelineRasterizationStateCreateInfo rasterizationState = {};
			rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizationState.pNext = nullptr;
			rasterizationState.rasterizerDiscardEnable = rasterizationJson["rasterizerDiscardEnable"];
			rasterizationState.cullMode = rasterizationJson["cullMode"];
			rasterizationState.depthBiasClamp = rasterizationJson["depthBiasClamp"];
			rasterizationState.depthBiasConstantFactor = rasterizationJson["depthBiasConstantFactor"];
			rasterizationState.depthBiasEnable = rasterizationJson["depthBiasEnable"];
			rasterizationState.depthBiasSlopeFactor = rasterizationJson["depthBiasSlopeFactor"];
			rasterizationState.depthClampEnable = rasterizationJson["depthClampEnable"];
			rasterizationState.frontFace = rasterizationJson["frontFace"];
			rasterizationState.lineWidth = rasterizationJson["lineWidth"];
			rasterizationState.polygonMode = rasterizationJson["polygonMode"];

			auto tesselationJson = pipelineJson["tesselationConfig"];
			VkPipelineTessellationStateCreateInfo tesselationState = {};
			tesselationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
			tesselationState.pNext = nullptr;
			tesselationState.patchControlPoints = tesselationJson["patchControlPoints"];

			auto vertexInputJson = pipelineJson["vertexInputConfig"];
			std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;
			for (const auto& inputAttrDescJson : vertexInputJson["vertexAttributeDescriptions"])
			{
				auto& attributeDescription = vertexAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription());
				attributeDescription.binding = inputAttrDescJson["binding"];
				attributeDescription.format = inputAttrDescJson["format"];
				attributeDescription.location = inputAttrDescJson["location"];
				attributeDescription.offset = inputAttrDescJson["offset"];
			}

			std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions;
			for (const auto& inputBindingDescJson : vertexInputJson["vertexBindingDescriptions"])
			{
				auto& bindingDescription = vertexBindingDescriptions.emplace_back(VkVertexInputBindingDescription());
				bindingDescription.binding = inputBindingDescJson["binding"];
				bindingDescription.inputRate = inputBindingDescJson["inputRate"];
				bindingDescription.stride = inputBindingDescJson["stride"];
			}

			VkPipelineVertexInputStateCreateInfo vertexInputState = {};
			vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputState.pNext = nullptr;
			vertexInputState.vertexAttributeDescriptionCount = gsl::narrow<uint32_t>(vertexAttributeDescriptions.size());
			vertexInputState.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();
			vertexInputState.vertexBindingDescriptionCount = gsl::narrow<uint32_t>(vertexBindingDescriptions.size());
			vertexInputState.pVertexBindingDescriptions = vertexBindingDescriptions.data();

			auto viewportJson = pipelineJson["viewportConfig"];
			std::vector<VkViewport> viewports;
			for (const auto& viewportJson : viewportJson["viewports"])
			{
				auto& viewport = viewports.emplace_back(VkViewport());
				viewport.x = viewportJson["x"];
				viewport.y = viewportJson["y"];
				viewport.width = viewportJson["width"];
				viewport.height = viewportJson["height"];
				viewport.minDepth = viewportJson["minDepth"];
				viewport.maxDepth = viewportJson["maxDepth"];
			}

			std::vector<VkRect2D> scissors;
			for (const auto& scissorJson : viewportJson["scissors"])
			{
				auto& scissor = scissors.emplace_back(VkRect2D());
				scissor.extent.width = scissorJson["extent"]["width"];
				scissor.extent.height = scissorJson["extent"]["height"];
				scissor.offset.x = scissorJson["offset"]["x"];
				scissor.offset.y = scissorJson["offset"]["y"];
			}

			VkPipelineViewportStateCreateInfo viewportState = {};
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.pNext = nullptr;
			viewportState.viewportCount = gsl::narrow<uint32_t>(viewports.size());
			viewportState.pViewports = viewports.data();
			viewportState.scissorCount = gsl::narrow<uint32_t>(scissors.size());
			viewportState.pScissors = scissors.data();

			VkGraphicsPipelineCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.layout = layout;
			createInfo.pColorBlendState = &colorBlendState;
			createInfo.pDepthStencilState = &depthStencilState;
			createInfo.pDynamicState = &dynamicState;
			createInfo.pInputAssemblyState = &inputAssemblyState;
			createInfo.pMultisampleState = &multisampleState;
			createInfo.pRasterizationState = &rasterizationState;
			createInfo.stageCount = gsl::narrow<uint32_t>(shaderStages.size());
			createInfo.pStages = shaderStages.data();
			createInfo.pTessellationState = &tesselationState;
			createInfo.pVertexInputState = &vertexInputState;
			createInfo.pViewportState = &viewportState;
			createInfo.renderPass = renderPass;
			createInfo.subpass = pipelineJson["subpass"];

			VkPipeline pipeline;
			vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline);
			pipelines[pipeline] = VkPipelineUnique(pipeline, VkPipelineDeleter(device));

			return pipeline;
		}

	private:
		VkInstance instance;
		std::vector<const char *> deviceExtensions;
		VkSurfaceKHR surface;

		VkPhysicalDevice physicalDevice;
		std::vector<VkQueueFamilyProperties> queueFamilyProperties;

		VkDeviceQueueCreateInfo graphicsQueueCreateInfo;
		VkDeviceQueueCreateInfo presentQueueCreateInfo;
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		VkDeviceCreateInfo createInfo;
		VkDevice device;
		VkDeviceUnique deviceUnique;
		VkQueue graphicsQueue;
		VkQueue presentQueue;
		Allocator allocator;

		std::map<VkCommandPool, VkCommandPoolUnique> commandPools;
		std::map<VkFence, VkFenceUnique> fences;
		std::map<VkSemaphore, VkSemaphoreUnique> semaphores;
		std::map<VkSampler, VkSamplerUnique> samplers;
		std::map<VkShaderModule, VkShaderModuleUnique> shaderModules;
		std::map<VkSwapchainKHR, VkSwapchainKHRUnique> swapchains;
		std::map<VkRenderPass, VkRenderPassUnique> renderPasses;
		std::map<VkDescriptorSetLayout, VkDescriptorSetLayoutUnique> descriptorSetLayouts;
		std::map<VkPipelineLayout, VkPipelineLayoutUnique> pipelineLayouts;
		std::map<VkPipeline, VkPipelineUnique> pipelines;


		void SelectPhysicalDevice()
		{
			std::vector<VkPhysicalDevice> physicalDevices;
			uint32_t physicalDeviceCount;
			vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
			physicalDevices.resize(physicalDeviceCount);
			auto result = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

			physicalDevice = physicalDevices[0];
		}

		void GetQueueFamilyProperties()
		{
			uint32_t propCount;
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
				&propCount,
				nullptr);
			queueFamilyProperties.resize(propCount);
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
				&propCount,
				queueFamilyProperties.data());
		}

		void SelectGraphicsQueue()
		{
			std::optional<uint32_t> graphicsQueueFamilyIndex;
			for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i)
			{
				if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					graphicsQueueFamilyIndex = i;
					break;
				}
			}

			if (graphicsQueueFamilyIndex.has_value() == false)
			{
				std::runtime_error("Device does not support graphics queue!");
			}

			graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			graphicsQueueCreateInfo.pNext = nullptr;
			graphicsQueueCreateInfo.flags = 0;
			graphicsQueueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex.value();
			graphicsQueueCreateInfo.queueCount = 1;
			graphicsQueueCreateInfo.pQueuePriorities = &graphicsQueuePriority;
		}

		void SelectPresentQueue()
		{
			std::optional<uint32_t> presentQueueFamilyIndex;
			for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i)
			{
				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
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

			presentQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			presentQueueCreateInfo.pNext = nullptr;
			presentQueueCreateInfo.flags = 0;
			presentQueueCreateInfo.queueFamilyIndex = presentQueueFamilyIndex.value();
			presentQueueCreateInfo.queueCount = 1;
			presentQueueCreateInfo.pQueuePriorities = &presentQueuePriority;
		}

		void CreateDevice()
		{
			queueCreateInfos.push_back(graphicsQueueCreateInfo);
			auto graphicsQueueID = GetGraphicsQueueID();
			auto presentQueueID = GetPresentQueueID();
			auto singleQueue = (graphicsQueueID == presentQueueID);
			if (!singleQueue)
			{
				queueCreateInfos.push_back(presentQueueCreateInfo);
			}

			createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.flags = 0;
			createInfo.queueCreateInfoCount = gsl::narrow<uint32_t>(queueCreateInfos.size());
			createInfo.pQueueCreateInfos = queueCreateInfos.data();
			createInfo.enabledLayerCount = 0;
			createInfo.ppEnabledLayerNames = nullptr;
			createInfo.enabledExtensionCount = gsl::narrow<uint32_t>(deviceExtensions.size());
			createInfo.ppEnabledExtensionNames = deviceExtensions.data();
			createInfo.pEnabledFeatures = nullptr;
			vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
			deviceUnique = VkDeviceUnique(device, VkDeviceDeleter());
			LoadDeviceLevelEntryPoints(device);

			vkGetDeviceQueue(device, graphicsQueueID, 0, &graphicsQueue);
			if (!singleQueue)
			{
				vkGetDeviceQueue(device, presentQueueID, 0, &presentQueue);
			}
		}

		void CreateAllocator()
		{
			constexpr auto allocSize = 512000U;
			allocator = Allocator(physicalDevice, GetDevice(), allocSize);
		}
	};
} // namespace vka