#include "VulkanApp.h"
#include "stx/btree_map.h"
#include <algorithm>
#include "profiler.hpp"
#include <iostream>
#include "text.hpp"
#include "ECS.hpp"

int main()
{
	// Name, Path
	std::vector<StringPair> texturePaths =
	{
		{ "Test1", "Content/Textures/texture.jpg" },
		{ "Star", "Content/Textures/star.png" }
	};

	std::string vertexShaderPath = "Shaders/vert.spv";
	std::string fragmentShaderPath = "Shaders/frag.spv";
	
	ECS::Manager ecsManager;
	stx::btree_map<Entity, Sprite> spriteComponents;

	VulkanApp app;

	VulkanApp::AppInitData initData;
	initData.width = 900;
	initData.height = 900;
	initData.texturePaths = texturePaths;
	initData.vertexShaderPath = vertexShaderPath;
	initData.fragmentShaderPath = fragmentShaderPath;
	try
	{
		app.run(initData);

		for (auto i = 0.f; i < 100.f; i++)
		{
			auto entity = ecsManager.CreateEntity();
			auto& sprite = spriteComponents[entity];
			sprite.textureIndex = app.getTextureByName("Star").index;
			sprite.transform = glm::translate(glm::mat4(1.f), glm::vec3(i, i, 0.f));
		}
		bool running = true;
		while (running)
		{
			static profiler::Describe<0> desc("Main Loop");
			profiler::startTimer<0>();
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
			profiler::endTimer<0>();
			// auto avg5 = profiler::toMicroseconds(profiler::getRollingAverage<0>(5)).count();
			// std::cout << avg5 << " microseconds per update for the last 5 frames.\r\n";
		}
		auto avg = profiler::getAverageTime<0>();
		std::cout << profiler::toMicroseconds(avg).count() << " microseconds per update.\r\n";
		std::cout << modulo(-8, 5) << " should be 2.\r\n";
		std::cout << modulo(-3, 5) << " should be 2.\r\n";
		std::cout << modulo(5, 5) << " should be 0.\r\n";
		std::cout << modulo(6, 5) << " should be 1.\r\n";
	}
	catch(const std::runtime_error &e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}