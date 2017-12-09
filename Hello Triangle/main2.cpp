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

		for (auto i = 0.f; i < 100.f; i++)
		{
			spriteVector.push_back(Sprite());
			spriteVector.back().texture = app.getTextureByName("Star");
			auto pos = 10.f * i;
			spriteVector.back().setPosition({pos, pos});
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