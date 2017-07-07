#define GLFW_INCLUDE_VULKAN 
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define STB_IMAGE_IMPLEMENTATION

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <stb-master/stb_image.h>
#include <vector>
#include "VulkanBase.h"

const auto Width = 800U;
const auto Height = 600U;

std::vector<char*> ValidationLayers = 
{
	"VK_LAYER_LUNARG_standard_validation"
};

std::vector<char*> RequiredExtensions = {};

std::vector<char*> DeviceExtensions =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

class VulkanApp
{
public:
	void run()
	{
		initWindow();
	}
private:
	GLFWwindow* m_Window;
	std::shared_ptr<vke::Instance> m_Instance;
	std::unique_ptr<vke::Surface> m_Surface;
	std::unique_ptr<vke::PhysicalDevice> m_PhysicalDevice;
	std::shared_ptr<vke::LogicalDevice> m_Device;
	std::unique_ptr<vke::CommandPool> m_TransientCommandPool;
	std::unique_ptr<vke::CommandPool> m_ExtendedCommandPool;
	

	static void onWindowResized(GLFWwindow* window, int width, int height)
	{
		throw std::exception("Window resize function not implemented yet.");
	}

	void initWindow()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		m_Window = glfwCreateWindow(Width, Height, "Vulkan", nullptr, nullptr);

		glfwSetWindowUserPointer(m_Window, this);
		glfwSetWindowSizeCallback(m_Window, VulkanApp::onWindowResized);
	}

	void initVulkan()
	{
		// create instance
		m_Instance->init(ValidationLayers, RequiredExtensions, DeviceExtensions);

		// create surface
		VkSurfaceKHR surface;
		glfwCreateWindowSurface(*m_Instance, m_Window, nullptr, &surface);
		m_Surface->init(surface, *m_Instance);

		// pick physical device
		m_PhysicalDevice->init(*m_Instance, *m_Surface);

		m_TransientCommandPool->init(*m_Device, m_Device->getGraphicsQueue(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
		// TODO: decide whether the present queue pool should be transient
		m_ExtendedCommandPool->init(*m_Device, m_Device->getPresentQueue(), 0);
	}
};

int main()
{
	VulkanApp app;

	try
	{
		app.run();
	}
	catch(const std::runtime_error &e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}