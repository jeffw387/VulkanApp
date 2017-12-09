

 
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

#include "VulkanData.h"
#include "VulkanBase.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define FMT_HEADER_ONLY
#include <fmt/printf.h>

#include <vector>
#include <tuple>
#include <string>
#include "fileIO.h"
#include "Input.h"

using StringPair = std::tuple<std::string, std::string>;

const auto IndicesPerQuad = 6U;

std::vector<const char*> ValidationLayers = 
{
	#ifdef DEBUG
	"VK_LAYER_LUNARG_standard_validation"
	#endif
};

std::vector<const char*> RequiredExtensions = 
{ 
	VK_EXT_DEBUG_REPORT_EXTENSION_NAME 
};

std::vector<const char*> DeviceExtensions =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

enum class DrawLayers
{
	Background = 0,
	Parallax = 1,
	Units = 2,
	Projectiles = 3,
	UI = 4
};

struct BufferStruct
{
	vke::CommandPool pool;
	vke::Fence drawCommandExecuted;
	vke::CommandBuffer drawCommandBuffer;
	vke::CommandBuffer stagingCommandBuffer;
	vke::Buffer matrixBuffer;
	vke::Buffer matrixStagingBuffer;
	vke::Semaphore matrixBufferStaged;
	vke::Fence stagingCommandExecuted;
	size_t matrixBufferCapacity;
	vke::Framebuffer framebuffer;
	vke::DescriptorPool descriptorPool;
	vke::DescriptorSet descriptorSet0;
	vke::DescriptorSet descriptorSet1;
};

struct Texture2D
{
	uint32_t index;
	VkImage imageHandle;
	uint32_t width;
	uint32_t height;
	VkDeviceSize size;
};

struct MatrixValidator
{
	glm::mat4 Matrix;
	bool Valid = false;
};

struct Sprite
{
	Texture2D texture;
	struct Position
	{
		float x;
		float y;
	};

	const Position& getPosition() { return m_Position; }
	void setPosition(Position&& position)
	{
		m_Position = position;
		m_Transform.Valid = false;
	}

	const float& getRotation() { return m_Rotation; }
	void setRotation(float&& rotation)
	{
		m_Rotation = rotation;
		m_Transform.Valid = false;
	}

	const float& getScale() { return m_Scale; }
	void setScale(float&& scale)
	{
		m_Scale = scale;
		m_Transform.Valid = false;
	}

	const glm::mat4& getTransform()
	{
		if (m_Transform.Valid != true)
		{
			auto scale = glm::scale(glm::mat4(1.0f), glm::vec3(m_Scale, m_Scale, m_Scale));
			auto rotation = glm::rotate(scale, m_Rotation, glm::vec3(0.0f, 0.0f, 1.0f));
			m_Transform.Matrix = glm::translate(scale, glm::vec3(m_Position.x, m_Position.y, 0.0f));
			m_Transform.Valid = true;
		}
		return m_Transform.Matrix;
	}
private:
	Position m_Position;
	float m_Rotation;
	float m_Scale;
	MatrixValidator m_Transform;
};

class Camera2D
{
public:
	struct Position
	{
		float x;
		float y;
	};

	struct Size
	{
		float width;
		float height;
	};

	const Position& getPosition() { return m_Position; }
	void setPosition(Position&& position)
	{
		m_Position = position;
		m_ViewProjectionMat.Valid = false;
	}

	const Size& getSize() { return m_Size; }
	void setSize(Size&& size)
	{
		m_Size = size;
		m_ViewProjectionMat.Valid = false;
	}

	const float& getRotation() { return m_Rotation; }
	void setRotation(float&& rotation) 
	{ 
		m_Rotation = rotation;
		m_ViewProjectionMat.Valid = false;
	}

	const glm::mat4& getMatrix()
	{
		if (m_ViewProjectionMat.Valid != true)
		{
			auto halfWidth = m_Size.width * 0.5f;
			auto halfHeight = m_Size.height * 0.5f;
			auto left = -halfWidth + m_Position.x;
			auto right = halfWidth + m_Position.x;
			auto bottom = halfHeight + m_Position.y;
			auto top = -halfHeight + m_Position.y;

			m_ViewProjectionMat.Matrix = glm::ortho(left, right, top, bottom, 0.0f, 1.0f);
			m_ViewProjectionMat.Valid = true;
		}
		return m_ViewProjectionMat.Matrix;
	}

private:
	Position m_Position;
	Size m_Size;
	float m_Rotation;
	MatrixValidator m_ViewProjectionMat;
};

class VulkanApp
{
public:
	struct AppInitData
	{
		unsigned int width;
		unsigned int height;
		std::vector<StringPair> texturePaths;
		std::string vertexShaderPath;
		std::string fragmentShaderPath;
	};

	void run(AppInitData initData)
	{
		initGLFW(static_cast<int>(initData.width), static_cast<int>(initData.height));
		m_Camera.setSize( { static_cast<float>(initData.width), static_cast<float>(initData.height) } );
		m_Camera.setPosition( { 0.0f, 0.0f } );
		initVulkan(initData.vertexShaderPath, initData.fragmentShaderPath);
		loadTextures(initData.texturePaths);
		
		updateDescriptorSet0();
	}

	// returns false if window should close
	bool render(std::vector<Sprite>& spriteVector)
	{
		glfwPollEvents();
		if (glfwWindowShouldClose(m_Window))
		{
			return false;
		}
		ImageIndex nextImage;
		auto success = acquireImage(nextImage);
		if (success != true)
		{
			return true;
		}
		update(nextImage, spriteVector);
		draw(nextImage);
		return true;
	}

	void cleanup()
	{
		vkDeviceWaitIdle(m_Device);
		m_ImageReadyForPresentation.cleanup();
		m_ImageReadyForWriting.cleanup();
		m_Sampler.cleanup();
		m_Pipeline.cleanup();
		m_GraphicsCommandPool.cleanup();
		for (auto& presentBuffer : m_PresentBuffers)
		{
			presentBuffer.pool.cleanup();
			presentBuffer.drawCommandExecuted.cleanup();
			presentBuffer.matrixBuffer.cleanup();
			presentBuffer.matrixStagingBuffer.cleanup();
			presentBuffer.matrixBufferStaged.cleanup();
			presentBuffer.framebuffer.cleanup();
			presentBuffer.stagingCommandExecuted.cleanup();
			presentBuffer.descriptorPool.cleanup();
		}
		m_Swapchain.cleanup();
		m_RenderPass.cleanup();
		m_PipelineLayout.cleanup();
		m_Set0Layout.cleanup();
		m_Set1Layout.cleanup();
		for (auto& image : m_Images)
		{
			image.cleanup();
		}
		m_DepthImage.cleanup();
		m_VertexBuffer.cleanup();
		m_IndexBuffer.cleanup();
		m_Allocator.cleanup();
		m_Device.cleanup();
		m_Surface.cleanup();
		m_Instance.cleanup();

		glfwDestroyWindow(m_Window);
	}

	Texture2D getTextureByName(std::string textureName)
	{
		return m_TexturesByName[textureName];
	}

private:
	VulkanData::ModelIndexPushConstant modelIndexPushconstant = VulkanData::ModelIndexPushConstant();
	VulkanData::SamplerAndImagesDescriptors set0_samplerAndImagesLayoutData = VulkanData::SamplerAndImagesDescriptors();
	VulkanData::MVPMatricesDescriptors set1_matricesLayoutData = VulkanData::MVPMatricesDescriptors();

	unsigned int m_Width;
	unsigned int m_Height;
	GLFWwindow* m_Window;
	Camera2D m_Camera;
	InputManager m_InputManager;
	vke::Instance m_Instance;
	vke::Surface m_Surface;
	vke::PhysicalDevice m_PhysicalDevice;
	vke::LogicalDevice m_Device;
	vke::Queue m_GraphicsQueue;
	vke::Queue m_PresentQueue;
	vke::CommandPool m_GraphicsCommandPool;
	std::vector<BufferStruct> m_PresentBuffers;
	vke::DescriptorSetLayout m_Set0Layout;
	vke::DescriptorSetLayout m_Set1Layout;
	vke::ShaderModule m_VertexShader;
	vke::ShaderModule m_FragmentShader;
	vke::PipelineLayout m_PipelineLayout;
	vke::Pipeline m_Pipeline;
	VkFormat m_DepthFormat;
	vke::Image2D m_DepthImage;
	vke::RenderPass m_RenderPass;
	vke::SwapchainSupportDetails m_SwapchainSupportDetails;
	VkSurfaceFormatKHR m_SurfaceFormat;
	VkPresentModeKHR m_PresentMode;
	VkExtent2D m_SwapExtent;
	uint32_t m_SwapImageCount;
	vke::Swapchain m_Swapchain;
	vke::Allocator m_Allocator;
	vke::Buffer m_VertexBuffer;
	vke::Buffer m_IndexBuffer;
	VkDeviceSize uniformBufferOffsetAlignment;
	std::vector<vke::Image2D> m_Images;
	vke::Sampler2D m_Sampler;
	vke::Semaphore m_ImageReadyForWriting;
	vke::Semaphore m_ImageReadyForPresentation;

	// host-side data
	std::vector<VulkanData::Vertex> m_VertexData;
	std::vector<uint32_t> m_IndexData;
	std::map<std::string, Texture2D> m_TexturesByName;
	std::vector<Texture2D> m_Textures;

	bool acquireImage(ImageIndex& imageIndex)
	{
		VkResult result = vkAcquireNextImageKHR(m_Device, m_Swapchain, std::numeric_limits<uint64_t>::max(), m_ImageReadyForWriting, VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) 
		{
			recreateSwapChain(getCurrentWindowExtent(m_Window));
			return false;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) 
		{
			throw std::runtime_error("failed to acquire swap chain image!");
		}
		return true;
	}

	void update(ImageIndex nextImage, std::vector<Sprite>& spriteVector)
	{
		updateUniformBuffers(nextImage, spriteVector);
		updateDescriptorSet1(nextImage);
		recordNextCommandBuffer(nextImage, spriteVector);
	}

	void draw(ImageIndex nextImage)
	{
		auto& drawCommandBuffer = m_PresentBuffers[nextImage].drawCommandBuffer;
		auto& drawCommandExecuted = m_PresentBuffers[nextImage].drawCommandExecuted;
		auto& matrixBufferStaged = m_PresentBuffers[nextImage].matrixBufferStaged;
		std::vector<VkSemaphore> queueWaitSemaphores = { m_ImageReadyForWriting, matrixBufferStaged };
		std::vector<VkSemaphore> queueSignalSemaphores = { m_ImageReadyForPresentation };
		drawCommandBuffer.submit(
			m_PresentQueue,
			std::vector<VkPipelineStageFlags>{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
			drawCommandExecuted,
			queueWaitSemaphores,
			queueSignalSemaphores
		);

		VkSwapchainKHR swapChains[] = { m_Swapchain };
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = static_cast<uint32_t>(queueSignalSemaphores.size());
		presentInfo.pWaitSemaphores = queueSignalSemaphores.data();
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &nextImage;

		auto result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) 
		{
			recreateSwapChain(getCurrentWindowExtent(m_Window));
		}
		else if (result != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to present swap chain image!");
		}
	}

	static VkExtent2D getCurrentWindowExtent(GLFWwindow* window)
	{
		int x;
		int y;
		glfwGetWindowSize(window, &x, &y);
		return { static_cast<uint32_t>(x), static_cast<uint32_t>(y) };
	}

	static void onWindowResized(GLFWwindow* window, int width, int height)
	{
		auto app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
		app->recreateSwapChain( { static_cast<uint32_t>(width), static_cast<uint32_t>(height) });
		app->m_Camera.setSize( {static_cast<float>(width), static_cast<float>(height) } );
	}

	void createFramebuffers(VkExtent2D swapExtent)
	{
		auto swapViews = m_Swapchain.getSwapChainViews();
		for (uint32_t i = 0; i < m_SwapImageCount; i++)
		{
			m_PresentBuffers[i].framebuffer.init(m_Device, m_RenderPass, swapViews[i], m_DepthImage, swapExtent);
		}
	}

	void recreateSwapChain(VkExtent2D currentWindowExtent)
	{
		// wait for resources to finish being used
		vkDeviceWaitIdle(m_Device);

		// clean up old swapchain dependencies
		for (auto& presentBuffer : m_PresentBuffers)
		{
			presentBuffer.framebuffer.cleanup();
		}
		m_DepthImage.cleanup();
		m_Swapchain.cleanup();

		m_SwapchainSupportDetails = vke::Swapchain::querySwapchainSupport(m_PhysicalDevice, m_Surface);
		m_SwapExtent = vke::Swapchain::chooseSwapExtent(m_SwapchainSupportDetails.capabilities, currentWindowExtent);

		// recreate swapchain
		m_Swapchain.init(m_PhysicalDevice,
			m_Device,
			m_Surface,
			m_SwapchainSupportDetails,
			m_SwapExtent,
			m_SwapImageCount,
			m_SurfaceFormat,
			m_PresentMode
		);

		// recreate depth image
		m_DepthImage.init(m_Device, &m_Allocator, m_SwapExtent.width, m_SwapExtent.height,
			findDepthFormat(m_PhysicalDevice),
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			VK_SHARING_MODE_EXCLUSIVE,
			VK_IMAGE_LAYOUT_UNDEFINED,
			vke::AllocationStyle::NewAllocation);
		m_DepthImage.transitionImageLayout(m_GraphicsCommandPool, m_GraphicsQueue, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		// recreate framebuffers
		createFramebuffers(m_SwapExtent);
	}

	void updateInput(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		auto app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
		auto cameraPosition = app->m_Camera.getPosition();
		switch (key)
		{
		case GLFW_KEY_LEFT:
			cameraPosition.x -= 1;
			break;
		case GLFW_KEY_RIGHT:
			cameraPosition.x += 1;
			break;
		case GLFW_KEY_UP:
			cameraPosition.y -= 1;
			break;
		case GLFW_KEY_DOWN:
			cameraPosition.y += 1;
			break;
		}
		app->m_Camera.setPosition(std::move(cameraPosition));
	}

	static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
	{
		auto app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
		app->m_InputManager.cursorPositionCallback(xpos, ypos);
	}

	static void mouseButtonCallback(GLFWwindow* window, int button, int state, int mods)
	{
		auto app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
		app->m_InputManager.mouseButtonCallback(button, state, mods);
	}

	static void keyCallback(GLFWwindow* window, int key, int scancode, int state, int mods)
	{
		auto app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
		app->m_InputManager.keyCallback(key, scancode, state, mods);
	}

	void initGLFW(int Width, int Height)
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		m_Window = glfwCreateWindow(Width, Height, "Vulkan", nullptr, nullptr);

		glfwSetWindowUserPointer(m_Window, this);
		glfwSetWindowSizeCallback(m_Window, VulkanApp::onWindowResized);
		glfwSetCursorPosCallback(m_Window, VulkanApp::cursorPositionCallback);
		glfwSetMouseButtonCallback(m_Window, VulkanApp::mouseButtonCallback);
		glfwSetKeyCallback(m_Window, VulkanApp::keyCallback);
	}

	void initVulkan(std::string vertexShaderPath, std::string fragmentShaderPath)
	{
		uint32_t glfwExtensionCount = 0;
		auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		for (uint32_t i = 0; i < glfwExtensionCount; i++)
		{
			RequiredExtensions.push_back(glfwExtensions[i]);
		}

		// create instance
		m_Instance.init(ValidationLayers, RequiredExtensions, DeviceExtensions);

		// create surface
		VkSurfaceKHR surface;
		glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &surface);
		m_Surface.init(surface, m_Instance);

		// pick physical device
		m_PhysicalDevice.init(m_Instance, m_Surface);

		// store the minimum uniform buffer offset alignment
		uniformBufferOffsetAlignment = m_PhysicalDevice.getPhysicalProperties().limits.minUniformBufferOffsetAlignment;

		// create logical device
		std::set<int> uniqueQueueFamilies = { m_PhysicalDevice.getGraphicsQueueIndex(), m_PhysicalDevice.getPresentQueueIndex() };
		m_Device.init(m_Instance, m_PhysicalDevice, uniqueQueueFamilies, DeviceExtensions, ValidationLayers);

		// init queues
		m_GraphicsQueue.init(m_Device, m_PhysicalDevice.getGraphicsQueueIndex());
		m_PresentQueue.init(m_Device, m_PhysicalDevice.getPresentQueueIndex());

		// create memory allocator
		m_Allocator.init(m_PhysicalDevice, m_Device);

		// create command pools
		m_GraphicsCommandPool.init(m_Device, m_PhysicalDevice.getGraphicsQueueIndex(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		// create descriptor set layouts
		m_Set0Layout.init(m_Device, set0_samplerAndImagesLayoutData.layoutBindings());
		m_Set1Layout.init(m_Device, set1_matricesLayoutData.layoutBindings());
		std::vector<VkDescriptorSetLayout> layouts = { m_Set0Layout, m_Set1Layout };

		VkExtent2D windowExtent;
		int width;
		int height;
		glfwGetWindowSize(m_Window, &width, &height);
		windowExtent.width = static_cast<uint32_t>(width);
		windowExtent.height = static_cast<uint32_t>(height);

		m_SwapchainSupportDetails = vke::Swapchain::querySwapchainSupport(m_PhysicalDevice, m_Surface);
		m_SurfaceFormat = vke::Swapchain::chooseSwapSurfaceFormat(m_SwapchainSupportDetails.formats);
		m_PresentMode = vke::Swapchain::chooseSwapPresentMode(m_SwapchainSupportDetails.presentModes);
		m_SwapExtent = vke::Swapchain::chooseSwapExtent(m_SwapchainSupportDetails.capabilities, windowExtent);
		m_SwapImageCount = vke::Swapchain::chooseImageCount(m_SwapchainSupportDetails);
		m_DepthFormat = findDepthFormat(m_PhysicalDevice);

		// create render pass
		m_RenderPass.init(m_Device, m_SurfaceFormat.format, m_DepthFormat);

		// create swap chain
		m_Swapchain.init(m_PhysicalDevice, m_Device, m_Surface, m_SwapchainSupportDetails, m_SwapExtent, m_SwapImageCount, m_SurfaceFormat, m_PresentMode);

		// create depth image
		m_DepthImage.init(
			m_Device,
			&m_Allocator,
			m_Swapchain.getExtent().width, 
			m_Swapchain.getExtent().height,
			/*m_GraphicsCommandPool,*/
			m_DepthFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			VK_SHARING_MODE_EXCLUSIVE,
			VK_IMAGE_LAYOUT_UNDEFINED);
		m_DepthImage.transitionImageLayout(m_GraphicsCommandPool, m_GraphicsQueue, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		// create descriptor pool sizes
		std::vector<VkDescriptorPoolSize> poolSizes;
		auto poolsizes0 = set0_samplerAndImagesLayoutData.poolSizes();
		auto poolsizes1 = set1_matricesLayoutData.poolSizes();
		poolSizes.insert(poolSizes.end(), poolsizes0.begin(), poolsizes0.end());
		poolSizes.insert(poolSizes.end(), poolsizes1.begin(), poolsizes1.end());

		// create present buffers
		auto swapViews = m_Swapchain.getSwapChainViews();
		auto presentBufferCount = swapViews.size();
		m_PresentBuffers.resize(presentBufferCount);
		// create present buffers
		size_t bufferIndex = 0;
		for (auto& presentBuffer : m_PresentBuffers)
		{
			presentBuffer.framebuffer.init(m_Device, m_RenderPass, swapViews[bufferIndex], m_DepthImage, m_SwapExtent);
			presentBuffer.drawCommandExecuted.init(m_Device);
			presentBuffer.pool.init(m_Device, m_PhysicalDevice.getPresentQueueIndex(), 
				VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
			presentBuffer.drawCommandBuffer.init(m_Device, presentBuffer.pool);
			presentBuffer.matrixBufferCapacity = 0;
			presentBuffer.matrixBufferStaged.init(m_Device);
			presentBuffer.stagingCommandBuffer.init(m_Device, presentBuffer.pool);
			presentBuffer.stagingCommandExecuted.init(m_Device);
			
			// initialize descriptor pool
			presentBuffer.descriptorPool.init(m_Device, poolSizes, 2);
			
			// initialize descriptor sets
			std::vector<VkDescriptorSetLayout> set0Layouts = { m_Set0Layout };
			presentBuffer.descriptorSet0.init(m_Device, presentBuffer.descriptorPool, set0Layouts);

			std::vector<VkDescriptorSetLayout> set1Layouts = { m_Set1Layout };
			presentBuffer.descriptorSet1.init(m_Device, presentBuffer.descriptorPool, set1Layouts);
			bufferIndex++;
		}

		// create shader modules
		m_VertexShader.init(m_Device, fileIO::readFile(vertexShaderPath));
		m_FragmentShader.init(m_Device, fileIO::readFile(fragmentShaderPath));

		// create pipeline layout
		m_PipelineLayout.init(m_Device, modelIndexPushconstant.ranges(), layouts);

		// create pipeline
		m_Pipeline.init(
			m_Device, 
			m_RenderPass, 
			m_VertexShader, 
			m_FragmentShader, 
			VulkanData::Vertex::getBindingDescription(), 
			VulkanData::Vertex::getAttributeDescriptions(),
			windowExtent,
			m_PipelineLayout);



		// create sampler
		m_Sampler.init(m_Device);

		// create semaphores
		m_ImageReadyForWriting.init(m_Device);
		m_ImageReadyForPresentation.init(m_Device);
	}

	void loadTextures(std::vector<StringPair> texturePaths)
	{
		std::vector<stbi_uc> pixelData;
		std::vector<Texture2D> textures;

		uint32_t textureIndex = 0;
		// for each texture name/path
		for (auto pair : texturePaths)
		{
			// Load the texture into an array of pixel data
			int texWidth, texHeight, texChannels;
			stbi_uc* pixels = stbi_load(std::get<1>(pair).c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
			VkDeviceSize imageSizeOnHost = texWidth * texHeight * 4;

			if (!pixels)
			{
				throw std::runtime_error("failed to load texture image!");
			}

			// create vke::Image2D
			m_Images.emplace_back();
			m_Images.back().init(m_Device, &m_Allocator, texWidth, texHeight);

			glm::vec3 LeftTopPos, LeftBottomPos, RightBottomPos, RightTopPos;
			auto halfWidth = texWidth * 0.5f;
			auto halfHeight = texHeight * 0.5f;
			LeftTopPos = {-halfWidth, -halfHeight, 0.0f};
			LeftBottomPos = {-halfWidth, halfHeight, 0.0f};
			RightBottomPos = {halfWidth, halfHeight, 0.0f};
			RightTopPos = {halfWidth, -halfHeight, 0.0f};

			glm::vec2 LeftTopUV, LeftBottomUV, RightBottomUV, RightTopUV;
			LeftTopUV = {0.0f, 0.0f};
			LeftBottomUV = {0.0f, 1.0f};
			RightBottomUV = {1.0f, 1.0f};
			RightTopUV = {1.0f, 0.0f};

			VulkanData::Vertex LeftTop, LeftBottom, RightBottom, RightTop;
			LeftTop = {LeftTopPos, LeftTopUV};
			LeftBottom = {LeftBottomPos, LeftBottomUV};
			RightBottom = {RightBottomPos, RightBottomUV};
			RightTop = {RightTopPos, RightTopUV};

			// push back vertices
			m_VertexData.push_back(LeftTop);
			m_VertexData.push_back(LeftBottom);
			m_VertexData.push_back(RightBottom);
			m_VertexData.push_back(RightTop);

			auto verticesPerTexture = 4U;
			auto indexOffset = verticesPerTexture * textureIndex;

			// push back indices
			m_IndexData.push_back(indexOffset + 0);
			m_IndexData.push_back(indexOffset + 1);
			m_IndexData.push_back(indexOffset + 2);
			m_IndexData.push_back(indexOffset + 2);
			m_IndexData.push_back(indexOffset + 3);
			m_IndexData.push_back(indexOffset + 0);

			// Create a Texture2D object for this texture
			Texture2D texture = {};
			texture.index = textureIndex;
			texture.width = texWidth;
			texture.height = texHeight;
			texture.size = m_Images.back().getMemoryRange().totalSize;
			texture.imageHandle = m_Images.back();

			// add the texture to a vector and a map by name
			m_Textures.push_back(texture);
			auto texName = std::get<0>(pair);
			m_TexturesByName.emplace(std::make_pair(texName, m_Textures.back()));

			vke::Buffer stagingBuffer;
			stagingBuffer.init(
				m_Device, 
				&m_Allocator, 
				texture.size, 
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			);
			stagingBuffer.DebugName = "Image Staging Buffer";

			if (stagingBuffer.mapMemoryRange() != true)
			{
				throw std::runtime_error("unable to map buffer");
			}

			memcpy(stagingBuffer.mappedData, pixels, static_cast<size_t>(imageSizeOnHost));
			stagingBuffer.unmap();

			m_Images.back().copyFromBuffer(stagingBuffer, m_GraphicsCommandPool, m_GraphicsQueue);

			stagingBuffer.cleanup();
			// free the host-side pixel data memory
			stbi_image_free(pixels);
			textureIndex++;
		}

		//
		// Copy vertex and index data to device buffers
		//
		vke::Buffer vertexStagingBuffer;
		vke::Buffer indexStagingBuffer;
		vke::CommandBuffer copyCmdBuffer;
		vke::Fence copyCmdExecuted;
		copyCmdBuffer.init(m_Device, m_GraphicsCommandPool);
		copyCmdExecuted.init(m_Device, 0U);
		auto vertexDataSize = m_VertexData.size() * sizeof(VulkanData::Vertex);
		auto indexDataSize = m_IndexData.size() * sizeof(ImageIndex);

		// initialize staging buffers
		vertexStagingBuffer.init(m_Device, &m_Allocator, vertexDataSize, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		vertexStagingBuffer.DebugName = "Vertex Staging Buffer";

		indexStagingBuffer.init(m_Device, &m_Allocator, indexDataSize, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		indexStagingBuffer.DebugName = "Index Staging Buffer";

		// initialize device buffers
		m_VertexBuffer.init(m_Device, &m_Allocator, vertexDataSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			vke::AllocationStyle::NewAllocation
			);
		m_VertexBuffer.DebugName = "Vertex Buffer";

		m_IndexBuffer.init(m_Device, &m_Allocator, indexDataSize,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			vke::AllocationStyle::NewAllocation
		);
		m_IndexBuffer.DebugName = "Index Buffer";

		// map vertex staging buffer
		if (vertexStagingBuffer.mapMemoryRange() != true)
		{
			throw std::runtime_error("Unable to map vertex staging buffer!");
		}
		// copy from host to staging buffer
		memcpy(vertexStagingBuffer.mappedData, m_VertexData.data(), vertexDataSize);
		// unmap vertex staging buffer
		vertexStagingBuffer.unmap();
		
		// copy from vertex staging buffer to device memory
		m_VertexBuffer.copyFromBuffer(vertexStagingBuffer, copyCmdBuffer, m_GraphicsQueue, copyCmdExecuted,
			std::vector<VkSemaphore>(), std::vector<VkSemaphore>());

		// map index staging buffer
		if (indexStagingBuffer.mapMemoryRange() != true)
		{
			throw std::runtime_error("Unable to map index staging buffer!");
		}
		// copy from host to staging buffer
		memcpy(indexStagingBuffer.mappedData, m_IndexData.data(), indexDataSize);
		// unmap index staging buffer
		indexStagingBuffer.unmap();
		// copy from index staging buffer to device memory
		vkWaitForFences(m_Device, 1, copyCmdExecuted, true, (uint64_t)std::numeric_limits<uint64_t>::max());
		copyCmdBuffer.reset();
		copyCmdExecuted.reset();
		m_IndexBuffer.copyFromBuffer(indexStagingBuffer, copyCmdBuffer, m_GraphicsQueue, copyCmdExecuted,
			std::vector<VkSemaphore>(), std::vector<VkSemaphore>());

		vkWaitForFences(m_Device, 1, copyCmdExecuted, true, (uint64_t)std::numeric_limits<uint64_t>::max());

		vertexStagingBuffer.cleanup();
		indexStagingBuffer.cleanup();
		copyCmdExecuted.cleanup();
		copyCmdBuffer.cleanup();
	}

	// TODO: figure out if I need to triple buffer/ring buffer this buffer
	void updateUniformBuffers(ImageIndex nextImage, std::vector<Sprite>& spriteVector)
	{
		auto& matrixBufferCapacity = m_PresentBuffers[nextImage].matrixBufferCapacity;
		auto& matrixBufferStaged = m_PresentBuffers[nextImage].matrixBufferStaged;
		auto& stagingCommandBuffer = m_PresentBuffers[nextImage].stagingCommandBuffer;
		auto& stagingCommandExecuted = m_PresentBuffers[nextImage].stagingCommandExecuted;
		auto& matrixBuffer = m_PresentBuffers[nextImage].matrixBuffer;
		auto& matrixStagingBuffer = m_PresentBuffers[nextImage].matrixStagingBuffer;

		const uint64_t minimumCount = 1;
		uint64_t instanceCount = std::max(minimumCount, spriteVector.size());
		static uint32_t runCount = 0;
		runCount++;
		// (re)create matrix buffer if it is smaller than required
		if (instanceCount > matrixBufferCapacity)
		{
			matrixBufferCapacity = instanceCount * 2;

			auto newBufferSize = matrixBufferCapacity * uniformBufferOffsetAlignment;
			matrixStagingBuffer.cleanup();
			matrixBuffer.cleanup();

			matrixBuffer.init(m_Device, 
				&m_Allocator, 
				newBufferSize, 
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			);

			matrixStagingBuffer.init(m_Device, 
				&m_Allocator, 
				newBufferSize, 
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			);
		}
		
		matrixBuffer.DebugName = "Matrix Buffer";
		
		matrixStagingBuffer.DebugName = "Matrix Staging Buffer";

		if (matrixStagingBuffer.mapMemoryRange() != true)
		{
			std::runtime_error("Unable to map matrix staging buffer!");
		}
		VkDeviceSize copyOffset = 0;
		glm::mat4 vp = m_Camera.getMatrix();
		for (auto& sprite : spriteVector)
		{
			auto m = sprite.getTransform();
			glm::mat4 mvp =  vp * m;
			memcpy((char*)matrixStagingBuffer.mappedData + copyOffset, &mvp, sizeof(glm::mat4));
			copyOffset += uniformBufferOffsetAlignment;
		}
		matrixStagingBuffer.unmap();
		vkWaitForFences(m_Device, 1, stagingCommandExecuted, true, std::numeric_limits<uint64_t>::max());
		stagingCommandExecuted.reset();
		matrixBuffer.copyFromBuffer(matrixStagingBuffer, stagingCommandBuffer, 
			m_GraphicsQueue, stagingCommandExecuted, std::vector<VkSemaphore>(),
			std::vector<VkSemaphore>({ matrixBufferStaged })
		);
	}

	void updateDescriptorSet0()
	{
		std::array<VkWriteDescriptorSet, 2> set0Writes = {};
		VkDescriptorImageInfo  samplerInfo = {};
		samplerInfo.sampler = m_Sampler;
		set0Writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		set0Writes[0].dstBinding = 0;
		set0Writes[0].dstArrayElement = 0;
		set0Writes[0].descriptorCount = 1;
		set0Writes[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		set0Writes[0].pImageInfo = &samplerInfo;

		std::vector<VkDescriptorImageInfo> imageInfos;
		for (const VkImageView& view : m_Images)
		{
			VkDescriptorImageInfo imageInfo = {};
			imageInfo.imageView = view;
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			imageInfos.push_back(imageInfo);
		}

		set0Writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		set0Writes[1].dstBinding = 1;
		set0Writes[1].dstArrayElement = 0;
		set0Writes[1].descriptorCount = static_cast<uint32_t>(imageInfos.size());
		set0Writes[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		set0Writes[1].pImageInfo = imageInfos.data();

		for (auto& presentBuffer : m_PresentBuffers)
		{
			set0Writes[0].dstSet = presentBuffer.descriptorSet0;
			set0Writes[1].dstSet = presentBuffer.descriptorSet0;
			vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(set0Writes.size()), set0Writes.data(), 0, nullptr);
		}
	}

	void updateDescriptorSet1(ImageIndex nextImage)
	{
		auto& matrixBuffer = m_PresentBuffers[nextImage].matrixBuffer;
		auto& commandBufferFence = m_PresentBuffers[nextImage].drawCommandExecuted;

		std::array<VkWriteDescriptorSet, 1> set1Writes = {};
		VkDescriptorBufferInfo matrixBufferInfo = {};
		matrixBufferInfo.buffer = matrixBuffer;
		matrixBufferInfo.offset = 0;
		matrixBufferInfo.range = uniformBufferOffsetAlignment;

		set1Writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		set1Writes[0].dstSet = m_PresentBuffers[nextImage].descriptorSet1;
		set1Writes[0].dstBinding = 0;
		set1Writes[0].dstArrayElement = 0;
		set1Writes[0].descriptorCount = 1;
		set1Writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		set1Writes[0].pBufferInfo = &matrixBufferInfo;

		// wait for this command pool's buffer(s) to finish execution, then reset the fence
		vkWaitForFences(m_Device, 1, commandBufferFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
		commandBufferFence.reset();
		vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(set1Writes.size()), set1Writes.data(), 0, nullptr);
	}

	void recordNextCommandBuffer(ImageIndex nextImage, std::vector<Sprite>& spriteVector)
	{
		auto& commandPool = m_PresentBuffers[nextImage].pool;
		auto& drawCommandBuffer = m_PresentBuffers[nextImage].drawCommandBuffer;
		auto& framebuffer = m_PresentBuffers[nextImage].framebuffer;
		auto& matrixBuffer = m_PresentBuffers[nextImage].matrixBuffer;
		auto& descriptorSet0 = m_PresentBuffers[nextImage].descriptorSet0;
		auto& descriptorSet1 = m_PresentBuffers[nextImage].descriptorSet1;
		
		// allocate the command buffer from the pool
		drawCommandBuffer.reset();
		drawCommandBuffer.init(m_Device, commandPool);

		/////////////////////////////////////////////////////////////
		// set up data that is constant between all command buffers
		//

		// Dynamic viewport info
		VkViewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = static_cast<float>(m_SwapExtent.width);
		viewport.height = static_cast<float>(m_SwapExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		// Dynamic scissor info
		VkRect2D scissorRect = {};
		scissorRect.extent = m_SwapExtent;
		scissorRect.offset = { 0, 0 };

		// clear values for the color and depth attachments
		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		// Render pass info
		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_RenderPass;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_SwapExtent;
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();
		renderPassInfo.framebuffer = framebuffer;

		VkBuffer buffers{m_VertexBuffer};
		VkDeviceSize offsets{m_VertexBuffer.getUsableOffset()};

		// record the command buffer
		drawCommandBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		vkCmdSetViewport(drawCommandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(drawCommandBuffer, 0, 1, &scissorRect);
		vkCmdBeginRenderPass(drawCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(drawCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
		vkCmdBindVertexBuffers(drawCommandBuffer, 0, 1, &buffers, &offsets);
		vkCmdBindIndexBuffer(drawCommandBuffer, m_IndexBuffer, m_IndexBuffer.getUsableOffset(), VK_INDEX_TYPE_UINT32);

		// bind sampler and images uniforms
		vkCmdBindDescriptorSets(drawCommandBuffer, 
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_PipelineLayout,
			0,
			1,
			descriptorSet0,
			0,
			nullptr
		);

		uint32_t spriteIndex = 0;
		// iterate through each sprite
		for (const auto& sprite : spriteVector)
		{
			vkCmdPushConstants(drawCommandBuffer, m_PipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ImageIndex), &sprite.texture.index);
			uint32_t dynamicOffset = spriteIndex * static_cast<uint32_t>(uniformBufferOffsetAlignment);

			// bind dynamic matrix uniform
			vkCmdBindDescriptorSets(drawCommandBuffer, 
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_PipelineLayout,
				1,
				1,
				descriptorSet1,
				1,
				&dynamicOffset
			);

			// draw the sprite
			vkCmdDrawIndexed(drawCommandBuffer, IndicesPerQuad, 1, 0, 0, 0);
			spriteIndex++;
		}
		vkCmdEndRenderPass(drawCommandBuffer);
		drawCommandBuffer.end();
	}

	VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) 
	{
		for (VkFormat format : candidates) 
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
				return format;
			}
		}
		throw std::runtime_error("failed to find supported format!");
	}

	VkFormat findDepthFormat(VkPhysicalDevice physicalDevice) 
	{
		std::vector<VkFormat> candidates = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
		return findSupportedFormat(
			physicalDevice,
			candidates,
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}
};
