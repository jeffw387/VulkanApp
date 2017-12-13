#include "VulkanApp.h"
#include "stx/btree_map.h"
#include <algorithm>

using SpriteTree = stx::btree_map<Entity, Sprite>;

VulkanApp app;

using Entity = size_t;

// Name, Path
std::vector<StringPair> texturePaths =
{
	{ "Test1", "Content/Textures/texture.jpg" },
	{ "Star", "Content/Textures/star.png" }
};

std::string vertexShaderPath = "Shaders/vert.spv";
std::string fragmentShaderPath = "Shaders/frag.spv";

std::vector<Entity> entities;
std::vector<int> isActive;
std::vector<Entity> inactiveEntities;
stx::btree_map<Entity, Sprite> spriteTree;

Entity CreateEntity()
{
	Entity newEntity;
	if (inactiveEntities.size() > 0)
	{
		newEntity = inactiveEntities.back();
		inactiveEntities.pop_back();
		isActive[newEntity] = true;
	}
	else
	{
		newEntity = entities.size();
		entities.push_back(newEntity);
		isActive.push_back(1);
	}
	return newEntity;
}

int main()
{
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
			auto entity = CreateEntity();
			auto& sprite = spriteTree[entity];
			sprite.textureIndex = app.getTextureByName("Star").index;
			sprite.transform = glm::translate(glm::mat4(1.f), glm::vec3(i, i, 0.f));
		}
		bool running = true;
		while (running)
		{
			//loop
			if (app.beginRender(spriteTree.size()) == false)
			{
				running = false;
				continue;
			}
			for (auto& spritePair : spriteTree)
			{
				app.renderSprite(spritePair.second);
			}
			app.endRender();
		}
	}
	catch(const std::runtime_error &e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}