#include "VulkanApp2.hpp"
#include <algorithm>
#include <iostream>
#include <map>

#include "stx/btree_map.h"
//#include "profiler.hpp"
#include "ECS.hpp"
#include "text.hpp"

namespace Image
{
	enum 
	{
		Star,
		Test1,
		Count
	};

	std::array<const char*, static_cast<size_t>(Image::Count)> Paths =
	{
		"Content/Textures/star.png",
		"Content/Textures/texture.jpg"
	};

	std::map<size_t, size_t> ImageIDToTextureID;

	void LoadHelper(VulkanApp& app, size_t imageID)
	{
		//ImageIDToTextureID[imageID] = app.initImage(Paths[imageID]);
	}
}

namespace Font
{
enum
{
	AeroviasBrasil,
	Count
};

std::array<const char*, static_cast<size_t>(Font::Count)> Paths = 
{
	"Content/Fonts/AeroviasBrasilNF.ttf"
};

void LoadHelper(VulkanApp& app, size_t fontID, Text::FontSize fontSize, size_t DPI)
{
	//app.LoadFont(fontID, fontSize, DPI, Paths[fontID]);
}
}

void LoadTextures(VulkanApp& app)
{
	// Load Textures Here
	Image::LoadHelper(app, Image::Star);
	Image::LoadHelper(app, Image::Test1);

	Font::LoadHelper(app, Font::AeroviasBrasil, 12, 166);
}

int main()
{
	//std::string vertexShaderPath = "Shaders/vert.spv";
	//std::string fragmentShaderPath = "Shaders/frag.spv";
	//
	//ECS::Manager entityManager;
	//SpriteMap spriteComponents;

	//VulkanApp app;
	//VulkanApp::AppInitData initData;
	//initData.width = 900;
	//initData.height = 900;
	//initData.vertexShaderPath = vertexShaderPath;
	//initData.fragmentShaderPath = fragmentShaderPath;
	//initData.loadImagesCallback = LoadTextures;
	//// Init application
	//app.initApp(initData);

	//VulkanApp::TextInitInfo initInfo;
	//initInfo.baseline_x = 0;
	//initInfo.baseline_y = 0;
	//initInfo.depth = -0.5f;
	//initInfo.fontID = Font::AeroviasBrasil;
	//initInfo.fontSize = 12;
	//initInfo.text = "Test Text that's a little longer!";
	//initInfo.textColor = glm::vec4(1.f,0.f,0.f,1.f);
	//TextureIndex starTexture = Image::ImageIDToTextureID[Image::Star];
	//for (auto i = 0.f; i < 3.f; i++)
	//{
	//	auto entity = entityManager.CreateEntity();
	//	auto& sprite = spriteComponents[entity];
	//	sprite.textureIndex = starTexture;
	//	sprite.color = glm::vec4(1.f);
	//	auto position = glm::vec3(i*2, i*2, -1.f);
	//	auto transform = glm::translate(glm::mat4(1.f), position);
	//	sprite.transform = transform;
	//}
	//auto TextGroup = app.createTextGroup(entityManager, spriteComponents, initInfo);
	//profiler::Describe<0>("Main Loop Timer ");
	//profiler::startTimer<0>();

	//bool running = true;
	//while (running)
	//{
	//	profiler::endTimer<0>();
	//	auto avg = profiler::toMicroseconds(profiler::getRollingAverage<0>(1));
	//	std::cout << profiler::getDescription<0>() << avg.count() << "ms\n";
	//	//loop
	//	if (app.beginRender(spriteComponents.size()) == false)
	//	{
	//		running = false;
	//		continue;
	//	}
	//	for (const auto& [entity, sprite] : spriteComponents)
	//	{
	//		app.renderSprite(sprite);
	//	}
	//	app.endRender();
	//	profiler::startTimer<0>();

	//}
	//app.cleanup();
	//std::cout << "Remaining allocations: " << g_counter << ". \n";
	//return EXIT_SUCCESS;
	return 0;
}