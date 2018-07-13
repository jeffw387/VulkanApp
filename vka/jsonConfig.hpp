#pragma once

#include "nlohmann/json.hpp"
#include "vulkan/vulkan.h"
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <map>

namespace vka
{
	using json = nlohmann::json;

	static json LoadJson(const std::string path)
	{
		std::ifstream f(path);
		json j;
		f >> j;
		return j;
	}

	enum class VulkanType
	{
		Instance,
		PhysicalDevice,
		Surface,
		Device,
		Sampler,
		Buffer,
		Image,
		RenderPass,
		Subpass,
		ColorAttachment,
		DepthAttachment,
		InputAttachment,
		Swapchain,
		Framebuffer,
		ShaderModule,
		DescriptorSetLayout,
		DescriptorPool,
		DescriptorSet,
		PushConstantRange,
		PipelineLayout,
		Pipeline,
		CommandPool,
		CommandBuffer
	};

	struct NodeProps
	{
		VulkanType vulkanType;
		std::string path;
	};

	struct Frame
	{
		std::vector<VkRenderPass> renderPasses;
		std::map<VkRenderPass, std::vector<VkPipeline>> subpasses;

	};

	/* static json GetModule(const json& inputJson)
	{
		auto jsonID = inputJson.find("jsonID");
		if (jsonID != inputJson.end())
		{
			bool isInline = (*jsonID)["inline"];
			if (isInline)
			{
				return inputJson;
			}
			else
			{
				std::string uri = (*jsonID)["uri"];
				auto f = std::ifstream(uri);
				json j;
				f >> j;
				return j;
			}
		}
		return json();
	}*/
	
	template <typename moduleT>
	static moduleT&& StoreHandle(
		moduleT&& module, 
		const json& moduleJson, 
		std::map<std::string, moduleT>& moduleMap)
	{
		std::string uri = moduleJson["jsonID"]["uri"];
		moduleMap[uri] = module;

		return std::move(module);
	}
}