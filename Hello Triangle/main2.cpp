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

int main()
{
	VulkanApp::AppInitData initData;
	initData.width = 500;
	initData.height = 500;
	initData.texturePaths = texturePaths;
	initData.vertexShaderPath = vertexShaderPath;
	initData.fragmentShaderPath = fragmentShaderPath;
	try
	{
		app.run(initData);
	}
	catch(const std::runtime_error &e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}