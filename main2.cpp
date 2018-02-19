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

class ClientApp
{
	ECS::Manager entityManager;
	using SpriteMap = stx::btree_map<Entity, Sprite>;
	SpriteMap spriteComponents;
	std::vector<const char*> Layers;
	std::vector<const char*> InstanceExtensions;
	vk::ApplicationInfo appInfo;
	vk::InstanceCreateInfo instanceCreateInfo;
	std::vector<const char*> deviceExtensions;
	vka::ShaderData shaderData;
	std::function<void(vka::VulkanApp*)> imageLoadCallback;
	vka::InitData initData;
	vka::VulkanApp app;

public:
	int run()
	{
		Layers = 
		{
		#ifndef NO_VALIDATION
			"VK_LAYER_LUNARG_standard_validation",
			//"VK_LAYER_LUNARG_api_dump",
		#endif
		};

		InstanceExtensions = 
		{
			VK_KHR_SURFACE_EXTENSION_NAME,
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
		#ifndef NO_VALIDATION
			VK_EXT_DEBUG_REPORT_EXTENSION_NAME
		#endif
		};

		appInfo = vk::ApplicationInfo();
		instanceCreateInfo = vk::InstanceCreateInfo(
			vk::InstanceCreateFlags(),
			&appInfo,
			static_cast<uint32_t>(Layers.size()),
			Layers.data(),
			static_cast<uint32_t>(InstanceExtensions.size()),
			InstanceExtensions.data()
		);
		deviceExtensions = 
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
		shaderData = { CONTENTROOT "Shaders/vert.spv", CONTENTROOT "Shaders/frag.spv" };
		imageLoadCallback = LoadTextures;
		initData = 
		{
			"Vulkan App",
			500,
			500,
			instanceCreateInfo,
			deviceExtensions,
			shaderData,
			imageLoadCallback,
			this
		};
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
		callbacks.UpdateCallback = [](void* userPtr, TimePoint_ms timePoint) -> vka::SpriteCount
		{
			ClientApp& client = *((ClientApp)userPtr);
			client.app->m_InputBuffer.popFirstIf([timePoint](InputMessage msg){ return msg.time < timePoint; })
			return client.spriteComponents.size();
		};
		callbacks.RenderCallback = [](void* userPtr)
		{
			ClientApp& client = *((ClientApp)userPtr);
			for (const auto& [entity, sprite] : client.spriteComponents)
			{
				client.app->RenderSprite(sprite.textureIndex, sprite.transform, sprite.color);
			}
		};
		app.Run(callbacks);

		return 0;
	}
};

