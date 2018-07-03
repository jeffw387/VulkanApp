#pragma once

#include "vulkan/vulkan.h"
#include "UniqueVulkan.hpp"
#include "Surface.hpp"
#include "Swapchain.hpp"
#include "RenderPass.hpp"
#include "Shader.hpp"
#include "DescriptorSet.hpp"
#include "Results.hpp"
#include "fileIO.hpp"
#include "Vertex.hpp"
#include "VertexData.hpp"
#include "GLFW/glfw3.h"
#include "Allocator.hpp"
#include "Pipeline.hpp"
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
	struct VertexPushConstants
	{
		glm::mat4 mvp;
	};

	struct FragmentPushConstants
	{
		glm::uint32 imageOffset;
		glm::vec3 padding;
		glm::vec4 color;
	};

	struct PushConstants
	{
		VertexPushConstants vertexPushConstants;
		FragmentPushConstants fragmentPushConstants;
	};

	constexpr float graphicsQueuePriority = 0.f;
	constexpr float presentQueuePriority = 0.f;

	class Device
	{
	public:
		static constexpr uint32_t ImageCount = 1;
		static constexpr uint32_t PushConstantSize = sizeof(PushConstants);

		Device(VkInstance instance,
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
			return deviceUnique.get();
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

		VkCommandPool CreateCommandPool(uint32_t queueFamilyIndex, bool transient, bool poolReset)
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

	private:
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
			VkDevice device;
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

		VkRenderPass CreateRenderPass(const VkDevice& device, const json &renderPassConfig)
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
			const VkDevice& device, 
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

		VkShaderModule CreateShaderModule(const VkDevice& device, const json& shaderJson)
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

		VkSampler CreateSampler(const VkDevice& device, const json& samplerJson)
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

		VkDescriptorSetLayout CreateDescriptorSetLayout(const VkDevice& device, const VkSampler& sampler, const json& descriptorSetLayoutJson, const std::vector<VkSampler>& immutableSamplers)
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

		VkPipelineLayout CreatePipelineLayout(const VkDevice& device, const std::vector<VkDescriptorSetLayout> setLayouts, const json& pipelineLayoutJson)
		{


			setLayouts.push_back(fragmentDescriptorSetOptional->GetSetLayout());

			VkPushConstantRange vertexPushRange = {};
			vertexPushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			vertexPushRange.offset = offsetof(PushConstants, vertexPushConstants);
			vertexPushRange.size = sizeof(VertexPushConstants);
			pushConstantRanges.push_back(vertexPushRange);

			VkPushConstantRange fragmentPushRange = {};
			fragmentPushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragmentPushRange.offset = offsetof(PushConstants, fragmentPushConstants);
			fragmentPushRange.size = sizeof(FragmentPushConstants);
			pushConstantRanges.push_back(fragmentPushRange);

			pipelineLayoutOptional = PipelineLayout(GetDevice(),
				setLayouts,
				pushConstantRanges);
		}

		void CreatePipeline()
		{
			VkSpecializationMapEntry imageArraySize;
			imageArraySize.constantID = 0;
			imageArraySize.offset = 0;
			imageArraySize.size = sizeof(uint32_t);

			auto vertexSpecializationMapEntries = std::vector<VkSpecializationMapEntry>();
			auto fragmentSpecializationMapEntries = std::vector<VkSpecializationMapEntry>{ imageArraySize };
			auto vertexSpecialization = MakeSpecialization(
				gsl::span<VkSpecializationMapEntry>(nullptr, 0),
				nullptr);

			auto fragmentSpecialization = MakeSpecialization(
				gsl::span<VkSpecializationMapEntry>(
					fragmentSpecializationMapEntries.data(),
					fragmentSpecializationMapEntries.size()),
				&Device::ImageCount);

			auto vertexShaderStageInfo = vertexShaderOptional->GetShaderStageInfo(&vertexSpecialization);
			auto fragmentShaderStageInfo = fragmentShaderOptional->GetShaderStageInfo(&fragmentSpecialization);

			pipelineOptional = Pipeline(GetDevice(),
				pipelineLayoutOptional->GetPipelineLayout(),
				renderPassOptional->GetRenderPass(),
				vertexShaderStageInfo,
				fragmentShaderStageInfo,
				vertexData);
		}
	};
} // namespace vka