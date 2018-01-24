#include "VulkanApp2.hpp"
#include <algorithm>
#include <iostream>
#include <map>

#include "stx/btree_map.h"
//#include "profiler.hpp"
#include "ECS.hpp"
#include "text.hpp"
#include "UniqueHandle.hpp"

namespace Image
{
	enum 
	{
		Star,
		Test1,
		COUNT
	};

	std::array<std::string, static_cast<size_t>(Image::COUNT)> Paths =
	{
		"Content/Textures/star.png",
		"Content/Textures/texture.jpg"
	};

	std::map<size_t, size_t> ImageIDToTextureID;

	void LoadHelper(vka::VulkanApp* app, size_t imageID)
	{
		ImageIDToTextureID[imageID] = app->createTexture(loadImageFromFile(Paths[imageID]));
	}
}

namespace Font
{
enum
{
	AeroviasBrasil,
	COUNT
};

std::array<const char*, static_cast<size_t>(Font::COUNT)> Paths = 
{
	"Content/Fonts/AeroviasBrasilNF.ttf"
};
}

void LoadTextures(vka::VulkanApp* app)
{
	// Load Textures Here
	Image::LoadHelper(app, Image::Star);
	Image::LoadHelper(app, Image::Test1);
	vka::VulkanApp::Text::FontID fontID;
	vka::VulkanApp::Text::FontSize fontSize;
	uint32_t DPI;
	const char* fontPath;

	app->LoadFont(Font::AeroviasBrasil, 12U, 166U, Font::Paths[Font::AeroviasBrasil]);
}

int main()
{

	ECS::Manager entityManager;
	using SpriteMap = stx::btree_map<Entity, Sprite>;
	SpriteMap spriteComponents;

	char const* WindowClassName = "VulkanWindowClass";
	char const* WindowTitle = "Vulkan App";
	Window::WindowStyle windowStyle = Window::WindowStyle::Windowed;
	int width = 500; int height = 500;
	std::vector<const char*> Layers = 
	{
		"VK_LAYER_LUNARG_standard_validation",
		//"VK_LAYER_LUNARG_device_limits",
		"VK_LAYER_LUNARG_api_dump",
		//"VK_LAYER_GOOGLE_unique_objects",
	};
	std::vector<const char*> InstanceExtensions = 
	{
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME
	};
	const vk::InstanceCreateInfo instanceCreateInfo = vk::InstanceCreateInfo(
		vk::InstanceCreateFlags(),
		&vk::ApplicationInfo(),
		gsl::narrow<uint32_t>(Layers.size()),
		Layers.data(),
		gsl::narrow<uint32_t>(InstanceExtensions.size()),
		InstanceExtensions.data()
	);
	const std::vector<const char*> deviceExtensions = 
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	const vka::ShaderData shaderData = { "Shaders/vert.spv", "Shaders/frag.spv" };
	std::function<void(vka::VulkanApp*)> imageLoadCallback = LoadTextures;
	vka::InitData initData = 
	{
		WindowClassName,
		WindowTitle,
		windowStyle,
		width,
		height,
		instanceCreateInfo,
		deviceExtensions,
		shaderData,
		imageLoadCallback
	};
	auto app = vka::VulkanApp::VulkanApp(initData);
	vka::LoopCallbacks callbacks;
	callbacks.BeforeRenderCallback = [&]() -> vka::SpriteCount{
		return 0U;
	};
	callbacks.RenderCallback = [&](vka::VulkanApp* app){};
	callbacks.AfterRenderCallback = [&](){};
	app.Run(callbacks);
	vka::VulkanApp::Text::InitInfo initInfo;
	initInfo.baseline_x = 0;
	initInfo.baseline_y = 0;
	initInfo.depth = -0.5f;
	initInfo.fontID = Font::AeroviasBrasil;
	initInfo.fontSize = 12;
	initInfo.text = "Test Text that's a little longer!";
	initInfo.textColor = glm::vec4(1.f,0.f,0.f,1.f);
	TextureIndex starTexture = gsl::narrow<TextureIndex>(Image::ImageIDToTextureID[Image::Star]);
	for (auto i = 0.f; i < 3.f; i++)
	{
		auto entity = entityManager.CreateEntity();
		auto& sprite = spriteComponents[entity];
		sprite.textureIndex = starTexture;
		sprite.color = glm::vec4(1.f);
		auto position = glm::vec3(i*2, i*2, -1.f);
		auto transform = glm::translate(glm::mat4(1.f), position);
		sprite.transform = transform;
	}
	auto TextGroup = app.m_Text.createTextGroup(initInfo);

	return 0;
}