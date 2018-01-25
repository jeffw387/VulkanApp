#define VMA_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "VulkanApp2.hpp"
#include <algorithm>
#include <iostream>
#include <map>

#include "stx/btree_map.h"
#include "ECS.hpp"
#include "text.hpp"

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

	void LoadHelper(vka::SuperClass* app, size_t imageID)
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

void LoadTextures(vka::SuperClass* app)
{
	// Load Textures Here
	Image::LoadHelper(app, Image::Star);
	Image::LoadHelper(app, Image::Test1);
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
		//"VK_LAYER_LUNARG_api_dump",
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
	std::function<void(vka::SuperClass*)> imageLoadCallback = LoadTextures;
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
	vka::SuperClass app;
	app.init(initData);
	vka::LoopCallbacks callbacks;
	callbacks.BeforeRenderCallback = [&]() -> vka::SpriteCount{
		return 0U;
	};
	callbacks.RenderCallback = [&](vka::SuperClass* app){};
	callbacks.AfterRenderCallback = [&](){};
	app.Run(callbacks);
	
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

	return 0;
}