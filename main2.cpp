#define VMA_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "VulkanApp2.hpp"
#include <algorithm>
#include <iostream>
#include <map>

// #include "stx/btree_map.h"
// #include "ECS.hpp"
#include "ECSComponents.hpp"
#include "entt.hpp"

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
	std::vector<const char*> Layers;
	std::vector<const char*> InstanceExtensions;
	vk::ApplicationInfo appInfo;
	vk::InstanceCreateInfo instanceCreateInfo;
	std::vector<const char*> deviceExtensions;
	vka::ShaderData shaderData;
	std::function<void(vka::VulkanApp*)> imageLoadCallback;
	vka::InitData initData;
	vka::VulkanApp app;
	entt::DefaultRegistry enttRegistry;

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
			900,
			900,
			instanceCreateInfo,
			deviceExtensions,
			shaderData,
			imageLoadCallback,
			this
		};
		app.init(initData);
		enttRegistry.prepare<cmp::TextureID, cmp::PositionMatrix, cmp::Color>();
		enttRegistry.prepare<cmp::Position, cmp::PositionMatrix, cmp::Velocity>();
		
		TextureIndex starTexture = static_cast<TextureIndex>(Image::ImageIDToTextureID[Image::Star]);
		for (auto i = 0.f; i < 3.f; i++)
		{
			auto entity = enttRegistry.create(
				cmp::TextureID(starTexture), 
				cmp::Position(glm::vec3(i*2.f, i*2.f, 0.f)),
				cmp::PositionMatrix(),
				cmp::Velocity(glm::vec2()),
				cmp::Color(glm::vec4(1.f)),
				cmp::RectSize());
		}

		struct InputVisitor
		{
			vka::VulkanApp* app;

			InputVisitor(vka::VulkanApp* app) : app(app) {}

			operator()(vka::KeyMessage msg)
			{
				auto bindingIterator = app->m_MaintainStateMap.find(msg.scancode);
				(*bindingIterator).second
			}

			operator()(vka::CharMessage msg)
			{}

			operator()(vka::MouseButtonMessage msg)
			{}

			operator()(vka::CursorPosMessage msg)
			{}
			
		};

		vka::LoopCallbacks callbacks;
		callbacks.UpdateCallback = [this](vka::TimePoint_ms timePoint) -> vka::SpriteCount
		{
			bool inputToProcess = true;
			while (inputToProcess)
			{
				auto msgOpt = app.m_InputBuffer.popFirstIf([timePoint](vka::InputMessage msg){ return msg.time < timePoint; });
				if (msgOpt.has_value())
				{
					switch (msgOpt.value().variant.index)
				}
			}
			
			auto physicsView = enttRegistry.persistent<cmp::Position, cmp::Velocity, cmp::PositionMatrix>();

			physicsView.each([](auto entity, auto& position, auto& velocity, auto& transform){

			});
			auto renderView = enttRegistry.persistent<cmp::TextureID, cmp::PositionMatrix, cmp::Color>();
			return view.size();
		};
		callbacks.RenderCallback = [this]()
		{
			auto view = enttRegistry.persistent<cmp::TextureID, cmp::PositionMatrix, cmp::Color>();
			view.each([this](auto entity, const auto& textureIndex, const auto& transform, const auto& color)
			{
				if (!transform.matrix.has_value())
				{
					continue;
				}
				app.RenderSprite(textureIndex.index, transform.matrix, color.rgba);
			});
		};
		app.Run(callbacks);

		return 0;
	}
};

int main()
{
	ClientApp clientApp;
	return clientApp.run();
}