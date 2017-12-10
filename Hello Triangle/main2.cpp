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
			spriteVector.back().textureIndex = app.getTextureByName("Star").index;
			auto pos = 10.f * i;
			spriteVector.back().m_Position = {pos, pos};
			spriteVector.back().m_Scale = 1.f;
			spriteVector.back().m_Rotation = 0.f;
		}
		bool running = true;
		while (running)
		{
			//loop
			running = app.render(spriteVector);
		}
	}
	catch(const std::runtime_error &e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}