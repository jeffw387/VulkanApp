#include "VulkanApp.h"

VulkanApp app;

// Name, Path
std::vector<StringPair> texturePaths =
{
	{ "Test1", "Content/Textures/texture.jpg" },
	{ "Star", "Content/Textures/star.png" }
};

std::string vertexShaderPath = "Shaders/vert.spv";
std::string fragmentShaderPath = "Shaders/frag.spv";

std::vector<Sprite> spriteVector;

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

		for (auto i = 0U; i < 10U; i++)
		{
			spriteVector.push_back(Sprite());
			spriteVector.back().texture = app.getTextureByName("Star");
			spriteVector.back().setPosition({static_cast<float>(10U * i), static_cast<float>(10U * i)});
			spriteVector.back().setScale(1.f);
		}
		while (app.render(spriteVector))
		{
			//loop
		}
	}
	catch(const std::runtime_error &e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}