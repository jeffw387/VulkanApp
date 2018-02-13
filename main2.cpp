#define VMA_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "VulkanApp2.hpp"
#include <algorithm>
#include <iostream>
#include <map>

#include "stx/btree_map.h"
#include "ECS.hpp"

#ifndef CONTENTROOT
#define CONTENTROOT
#endif

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
		CONTENTROOT "Content/Textures/star.png",
		CONTENTROOT "Content/Textures/texture.jpg"
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
	CONTENTROOT "Content/Fonts/AeroviasBrasilNF.ttf"
};
}

void LoadTextures(vka::VulkanApp* app)
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

	std::vector<const char*> Layers = 
	{
	#ifndef NO_VALIDATION
		"VK_LAYER_LUNARG_standard_validation",
		//"VK_LAYER_LUNARG_api_dump",
	#endif
	};

	std::vector<const char*> InstanceExtensions = 
	{
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
	#ifndef NO_VALIDATION
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME
	#endif
	};

    auto appInfo = vk::ApplicationInfo();
	const vk::InstanceCreateInfo instanceCreateInfo = vk::InstanceCreateInfo(
		vk::InstanceCreateFlags(),
		&appInfo,
		static_cast<uint32_t>(Layers.size()),
		Layers.data(),
		static_cast<uint32_t>(InstanceExtensions.size()),
		InstanceExtensions.data()
	);
	const std::vector<const char*> deviceExtensions = 
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	const vka::ShaderData shaderData = { CONTENTROOT "Shaders/vert.spv", CONTENTROOT "Shaders/frag.spv" };
	std::function<void(vka::VulkanApp*)> imageLoadCallback = LoadTextures;
	vka::InitData initData = 
	{
		"Vulkan App",
		500,
		500,
		instanceCreateInfo,
		deviceExtensions,
		shaderData,
		imageLoadCallback
	};
	vka::VulkanApp app;
	app.init(initData);
	
	TextureIndex starTexture = static_cast<TextureIndex>(Image::ImageIDToTextureID[Image::Star]);
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

	vka::LoopCallbacks callbacks;
	callbacks.BeforeRenderCallback = [&]() -> vka::SpriteCount
	{
		return spriteComponents.size();
	};
	callbacks.RenderCallback = [&](vka::VulkanApp* app)
	{
		for (const auto& [entity, sprite] : spriteComponents)
		{
			app->RenderSprite(sprite.textureIndex, sprite.transform, sprite.color);
		}
	};
	callbacks.AfterRenderCallback = [&](){};
	app.Run(callbacks);

	return 0;
}