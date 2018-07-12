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

	static VkPhysicalDevice SelectPhysicalDevice(VkInstance instance)
	{
		std::vector<VkPhysicalDevice> physicalDevices;
		uint32_t physicalDeviceCount;
		vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
		physicalDevices.resize(physicalDeviceCount);
		auto result = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

		return physicalDevices[0];
	}

	class DeviceManager
	{
	public:
		static constexpr uint32_t ImageCount = 1;

		DeviceManager(
			const VkPhysicalDevice& physicalDevice,
			const std::vector<const char*>& deviceExtensions,
			const VkSurfaceKHR& surface)
			:
			physicalDevice(physicalDevice),
			deviceExtensions(deviceExtensions),
			surface(surface)
		{
			CheckMemoryLocality();
			CreateDevice();
			CreateAllocator();
		}

		VkPhysicalDevice GetPhysicalDevice()
		{
			return physicalDevice;
		}

		bool HostDeviceCombined() { return hostDeviceCombined; }

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

#undef CreateSemaphore
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

		VkImageView CreateColorImageView2D(
			VkImage image,
			VkFormat format = VK_FORMAT_R8G8B8A8_SRGB)
		{
			VkImageView imageView;
			VkImageViewCreateInfo viewCreateInfo = {};
			viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewCreateInfo.pNext = nullptr;
			viewCreateInfo.flags = (VkImageViewCreateFlags)0;
			viewCreateInfo.image = image;
			viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewCreateInfo.format = format;
			viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewCreateInfo.subresourceRange.baseMipLevel = 0;
			viewCreateInfo.subresourceRange.levelCount = 1;
			viewCreateInfo.subresourceRange.baseArrayLayer = 0;
			viewCreateInfo.subresourceRange.layerCount = 1;
			vkCreateImageView(device, &viewCreateInfo, nullptr, &imageView);
			imageViews[imageView] = VkImageViewUnique(imageView, VkImageViewDeleter(device));

			return imageView;
		}

		void DestroyImageView(const VkImageView& view)
		{
			imageViews.erase(view);
		}

		VkFramebuffer CreateFramebuffer(
			const VkRenderPass& renderPass,
			const VkExtent2D& extent,
			const VkImageView& view)
		{
			VkFramebuffer framebuffer;
			VkFramebufferCreateInfo framebufferCreateInfo = {};
			framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCreateInfo.pNext = nullptr;
			framebufferCreateInfo.flags = 0;
			framebufferCreateInfo.renderPass = renderPass;
			framebufferCreateInfo.attachmentCount = 1;
			framebufferCreateInfo.width = extent.width;
			framebufferCreateInfo.height = extent.height;
			framebufferCreateInfo.layers = 1;
			framebufferCreateInfo.pAttachments = &view;
			vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &framebuffer);
			framebuffers[framebuffer] = VkFramebufferUnique(framebuffer, VkFramebufferDeleter(device));

			return framebuffer;
		}

		void DestroyFramebuffer(const VkFramebuffer& framebuffer)
		{
			framebuffers.erase(framebuffer);
		}

		VkCommandPool CreateCommandPool(
			const uint32_t& queueFamilyIndex,
			const bool& transient,
			const bool& poolReset)
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

		std::vector<VkCommandBuffer> AllocateCommandBuffers(
			const VkCommandPool& pool,
			const uint32_t& count)
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
			VkSurfaceFormatKHR& surfaceFormat,
			json renderPassConfig)
		{

			std::vector<VkSurfaceFormatKHR> formats;
			uint32_t formatCount = 0;
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
			formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());
			// TODO: possibly pass this in so that the application can choose the defaults
			surfaceFormat = SelectSurfaceFormat(formats, VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);

			std::vector<VkAttachmentDescription> attachmentDescriptions;
			for (const auto& attachmentJson : renderPassConfig["attachments"])
			{
				auto& description = attachmentDescriptions.emplace_back(VkAttachmentDescription());
				description.initialLayout = attachmentJson["initialLayout"];
				description.finalLayout = attachmentJson["finalLayout"];
				description.samples = attachmentJson["samples"];
				description.format = surfaceFormat.format;
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
				if (subpassData.depthStencilAttachment.has_value())
				{
					subpass.pDepthStencilAttachment = &subpassData.depthStencilAttachment.value();
				}
				else
				{
					subpass.pDepthStencilAttachment = nullptr;
				}

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
					subpassData.preserveAttachments.push_back(preserveAttachmentJson);
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
					dependency.dependencyFlags |= flag.get<uint32_t>();
				}
				for (const auto& maskBit : dependencyJson["srcAccessMask"])
				{
					dependency.srcAccessMask |= maskBit.get<uint32_t>();
				}
				for (const auto& maskBit : dependencyJson["dstAccessMask"])
				{
					dependency.dstAccessMask |= maskBit.get<uint32_t>();
				}
				for (const auto& maskBit : dependencyJson["srcStageMask"])
				{
					dependency.srcStageMask |= maskBit.get<uint32_t>();
				}
				for (const auto& maskBit : dependencyJson["dstStageMask"])
				{
					dependency.dstStageMask |= maskBit.get<uint32_t>();
				}
				dependency.srcSubpass = dependencyJson["srcSubpass"].get<int>();
				dependency.dstSubpass = dependencyJson["dstSubpass"].get<int>();
			}
			VkRenderPassCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
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

	private:
		VkSurfaceFormatKHR SelectSurfaceFormat(
			const std::vector<VkSurfaceFormatKHR>& supported,
			VkFormat preferredFormat,
			VkColorSpaceKHR preferredColorSpace)
		{
			VkSurfaceFormatKHR result = {};
			VkSurfaceFormatKHR preferred;
			preferred.format = preferredFormat;
			preferred.colorSpace = preferredColorSpace;
			if (supported.at(0).format == VK_FORMAT_UNDEFINED)
			{
				return preferred;
			}
			for (const auto& supportedElement : supported)
			{
				if (supportedElement.format == preferred.format && 
					supportedElement.colorSpace == preferred.colorSpace)
				{
					return preferred;
				}
			}
			return supported.at(0);
		}

		::VkPresentModeKHR SelectPresentMode(
			const std::vector<::VkPresentModeKHR>& supported,
			::VkPresentModeKHR preferred)
		{
			auto testPreferredSupport = std::find(supported.begin(), supported.end(), preferred);
			if (testPreferredSupport != supported.end())
			{
				return preferred;
			}
			return supported.at(0);
		}

	public:
		VkSwapchainKHR CreateSwapchain(
			const VkSurfaceKHR& surface,
			const VkSurfaceFormatKHR& surfaceFormat,
			json swapchainConfig)
		{
			VkSurfaceCapabilitiesKHR capabilities = {};
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

			std::vector<::VkPresentModeKHR> presentModes;
			uint32_t presentCount = 0;
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentCount, nullptr);
			presentModes.resize(presentCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentCount, presentModes.data());
			auto selectedPresentMode = SelectPresentMode(presentModes, swapchainConfig["presentMode"]);


			VkSwapchainCreateInfoKHR createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			createInfo.pNext = nullptr;
			for (const auto& flagBit : swapchainConfig["flags"])
			{
				createInfo.flags |= flagBit;
			}
			createInfo.clipped = swapchainConfig["clipped"];
			auto preferredCompositeAlpha = swapchainConfig["compositeAlpha"].get<VkCompositeAlphaFlagsKHR>();
			if (capabilities.supportedCompositeAlpha & preferredCompositeAlpha)
			{
				createInfo.compositeAlpha = static_cast<VkCompositeAlphaFlagBitsKHR>(preferredCompositeAlpha);
			}
			else
			{
				createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			}
			createInfo.imageArrayLayers = swapchainConfig["imageArrayLayers"];
			createInfo.imageFormat = surfaceFormat.format;
			createInfo.imageColorSpace = surfaceFormat.colorSpace;
			createInfo.imageExtent = capabilities.currentExtent;
			std::vector<uint32_t> queueFamilyIndices;
			if (GetGraphicsQueueID() == GetPresentQueueID())
			{
				createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			}
			else
			{
				createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
				queueFamilyIndices.push_back(GetGraphicsQueueID());
				queueFamilyIndices.push_back(GetPresentQueueID());
			}
			createInfo.imageUsage = swapchainConfig["imageUsage"];
			createInfo.minImageCount = swapchainConfig["minImageCount"];
			createInfo.queueFamilyIndexCount = gsl::narrow<uint32_t>(queueFamilyIndices.size());
			createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
			createInfo.presentMode = selectedPresentMode;
			createInfo.preTransform = capabilities.currentTransform;
			createInfo.surface = surface;
			createInfo.oldSwapchain = VK_NULL_HANDLE;

			VkSwapchainKHR swapchain;
			vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain);
			swapchains[swapchain] = VkSwapchainKHRUnique(swapchain, VkSwapchainKHRDeleter(device));

			return swapchain;
		}

		void DestroySwapchain(VkSwapchainKHR swapchain)
		{
			swapchains.erase(swapchain);
		}

		VkShaderModule CreateShaderModule(json shaderJson)
		{
			auto shaderBinary = fileIO::readFile(shaderJson["path"]);
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
			json samplerJson)
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
			createInfo.anisotropyEnable = samplerJson["anisotropyEnable"];
			createInfo.maxAnisotropy = samplerJson["maxAnisotropy"];
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
			json descriptorSetLayoutJson,
			const gsl::span<VkSampler> immutableSamplers)
		{
			VkDescriptorSetLayoutCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			createInfo.pNext = nullptr;

			std::vector<VkDescriptorSetLayoutBinding> bindings;
			for (const auto& bindingJson : descriptorSetLayoutJson["bindings"])
			{
				auto& binding = bindings.emplace_back(VkDescriptorSetLayoutBinding());
				binding.binding = bindingJson["binding"];
				binding.descriptorType = bindingJson["descriptorType"];
				binding.descriptorCount = bindingJson["descriptorCount"];
				for (const auto& flag : bindingJson["stageFlags"])
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

		VkDescriptorPool CreateDescriptorPool(
			json descriptorSetLayoutJson,
			const uint32_t& maxSets = 1,
			const bool& freeDescriptorSet = false)
		{
			std::vector<VkDescriptorPoolSize> poolSizes;
			for (const auto& bindingJson : descriptorSetLayoutJson["bindings"])
			{
				VkDescriptorPoolSize size;
				size.type = bindingJson["descriptorType"];
				size.descriptorCount = bindingJson["descriptorCount"];
				poolSizes.push_back(size);
			}
			VkDescriptorPoolCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.poolSizeCount = gsl::narrow<uint32_t>(poolSizes.size());
			createInfo.pPoolSizes = poolSizes.data();
			createInfo.maxSets = maxSets;
			if (freeDescriptorSet)
			{
				createInfo.flags |= VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			}

			VkDescriptorPool pool;
			vkCreateDescriptorPool(device, &createInfo, nullptr, &pool);
			descriptorPools[pool] = VkDescriptorPoolUnique(pool, VkDescriptorPoolDeleter(device));

			return pool;
		}

		std::vector<VkDescriptorSet> AllocateDescriptorSets(
			const VkDescriptorPool& pool,
			const gsl::span<VkDescriptorSetLayout> layouts,
			const uint32_t& count = 1)
		{
			VkDescriptorSetAllocateInfo allocateInfo = {};
			allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocateInfo.pNext = nullptr;
			allocateInfo.descriptorPool = pool;
			allocateInfo.descriptorSetCount = count;
			allocateInfo.pSetLayouts = layouts.data();

			std::vector<VkDescriptorSet> sets;
			sets.resize(count);
			vkAllocateDescriptorSets(device, &allocateInfo, sets.data());

			return sets;
		}

		VkPipelineLayout CreatePipelineLayout(
			const gsl::span<VkDescriptorSetLayout> setLayouts,
			json pipelineLayoutJson)
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
				for (const auto& flag : rangeJson["stageFlags"])
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
			json pipelineJson)
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
			auto multisampleJson = pipelineJson["multisampleConfig"];
			multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			VkSampleMask sampleMask = {};
			multisampleState.pNext = nullptr;
			multisampleState.sampleShadingEnable = multisampleJson["sampleShadingEnable"];
			multisampleState.minSampleShading = multisampleJson["minSampleShading"];
			multisampleState.rasterizationSamples = multisampleJson["rasterizationSamples"];
			multisampleState.alphaToCoverageEnable = multisampleJson["alphaToCoverageEnable"];
			multisampleState.alphaToOneEnable = multisampleJson["alphaToOneEnable"];
			multisampleState.pSampleMask = nullptr;

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
			createInfo.pTessellationState = nullptr;
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
		VkPhysicalDevice physicalDevice;
		std::vector<const char*> deviceExtensions;
		VkSurfaceKHR surface;
		bool hostDeviceCombined;

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

		std::map<VkImageView, VkImageViewUnique> imageViews;
		std::map<VkFramebuffer, VkFramebufferUnique> framebuffers;
		std::map<VkCommandPool, VkCommandPoolUnique> commandPools;
		std::map<VkFence, VkFenceUnique> fences;
		std::map<VkSemaphore, VkSemaphoreUnique> semaphores;
		std::map<VkSampler, VkSamplerUnique> samplers;
		std::map<VkShaderModule, VkShaderModuleUnique> shaderModules;
		std::map<VkSwapchainKHR, VkSwapchainKHRUnique> swapchains;
		std::map<VkRenderPass, VkRenderPassUnique> renderPasses;
		std::map<VkDescriptorSetLayout, VkDescriptorSetLayoutUnique> descriptorSetLayouts;
		std::map<VkDescriptorPool, VkDescriptorPoolUnique> descriptorPools;
		std::map<VkPipelineLayout, VkPipelineLayoutUnique> pipelineLayouts;
		std::map<VkPipeline, VkPipelineUnique> pipelines;

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
			GetQueueFamilyProperties();
			SelectGraphicsQueue();
			SelectPresentQueue();

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

		void CheckMemoryLocality()
		{
			VkPhysicalDeviceMemoryProperties properties = {};
			vkGetPhysicalDeviceMemoryProperties(physicalDevice, &properties);
			for (uint32_t i = 0; i < properties.memoryTypeCount; ++i)
			{
				const auto& memType = properties.memoryTypes[i];
				auto memoryLocalityFlags = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
					VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
				if (memType.propertyFlags & memoryLocalityFlags)
				{
					hostDeviceCombined = true;
					return;
				}
			}
		}

		void CreateAllocator()
		{
			constexpr auto allocSize = 512000U;
			allocator = Allocator(physicalDevice, device, allocSize);
		}
	};
} // namespace vka