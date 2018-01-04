#include <algorithm>
#include <iostream>
#include <map>

#include "VulkanApp.h"
#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
#include "stx/btree_map.h"
#include "profiler.hpp"
#include "text.hpp"
#include "ECS.hpp"
#include "ImageManager.hpp"


namespace Image
{
	struct Star
	{
		static const char* path;
	};
	const char* Star::path = "Content/Textures/star.png";
	struct Test1
	{
		static const char* path;
	};
	const char* Test1::path = "Content/Textures/texture.jpg";
}

int main()
{
	using ImageMgr = ImageManager<Image::Star, Image::Test1>;
	ImageMgr images;

	std::string vertexShaderPath = "Shaders/vert.spv";
	std::string fragmentShaderPath = "Shaders/frag.spv";
	
	ECS::Manager ecsManager;
	stx::btree_map<Entity, Sprite> spriteComponents;

	VulkanApp app;

	VulkanApp::AppInitData initData;
	initData.width = 900;
	initData.height = 900;
	initData.vertexShaderPath = vertexShaderPath;
	initData.fragmentShaderPath = fragmentShaderPath;
	app.initGLFW_Vulkan(initData);

	// Load Textures Here
	images.initImage<Image::Star>(app);
	images.initImage<Image::Test1>(app);

	// After loading all textures, copy vertex data to device
	app.CopyVertexDataToDevice();

	Texture2D starTexture = images.getTexture<Image::Star>();
	for (auto i = 0.f; i < 100.f; i++)
	{
		auto entity = ecsManager.CreateEntity();
		auto& sprite = spriteComponents[entity];
		sprite.textureIndex = starTexture.index;
		sprite.transform = glm::translate(glm::mat4(1.f), glm::vec3(i, i, 0.f));
	}
	bool running = true;
	while (running)
	{
		//loop
		if (app.beginRender(spriteComponents.size()) == false)
		{
			running = false;
			continue;
		}
		for (const auto& spritePair : spriteComponents)
		{
			app.renderSprite(spritePair.second);
		}
		app.endRender();
	}
	return EXIT_SUCCESS;
}