#undef min
#undef max

#define VMA_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "vka/VulkanApp2.hpp"
#include <algorithm>
#include <iostream>
#include <map>

// #include "ECSComponents.hpp"
#include "entt.hpp"
#undef min
#undef max

#ifndef CONTENTROOT
#define CONTENTROOT
#endif

// namespace Image
// {
// 	std::array<std::string, static_cast<size_t>(Image::COUNT)> Paths =
// 	{
// 		CONTENTROOT "Content/Textures/star.png",
// 		CONTENTROOT "Content/Textures/texture.jpg",
// 		CONTENTROOT "Content/Textures/dungeon_sheet.png"
// 	};
// }

// namespace Font
// {
// std::array<const char*, static_cast<size_t>(Font::COUNT)> Paths = 
// {
// 	CONTENTROOT "Content/Fonts/AeroviasBrasilNF.ttf"
// };
// }

// void LoadTextures(vka::VulkanApp* app)
// {
// 	// Load Textures Here
// 	Image::LoadHelper(app, Image::Star);
// 	Image::LoadHelper(app, Image::Test1);
// 	Image::LoadHelper(app, Image::DungeonSheet);
// }

class ClientApp
{
	// std::vector<const char*> Layers;
	// std::vector<const char*> InstanceExtensions;
	// vk::ApplicationInfo appInfo;
	// vk::InstanceCreateInfo instanceCreateInfo;
	// std::vector<const char*> deviceExtensions;
	// std::function<void(vka::VulkanApp*)> imageLoadCallback;
	// vka::InitData initData;
	// vka::VulkanApp app;
	// entt::DefaultRegistry enttRegistry;

public:
	int run()
	{
	// 	Layers = 
	// 	{
	// 	#ifndef NO_VALIDATION
	// 		"VK_LAYER_LUNARG_standard_validation",
	// 		//"VK_LAYER_LUNARG_api_dump",
	// 	#endif
	// 	};

	// 	InstanceExtensions = 
	// 	{
	// 		VK_KHR_SURFACE_EXTENSION_NAME,
	// 		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
	// 	#ifndef NO_VALIDATION
	// 		VK_EXT_DEBUG_REPORT_EXTENSION_NAME
	// 	#endif
	// 	};

	// 	appInfo = vk::ApplicationInfo();
	// 	instanceCreateInfo = vk::InstanceCreateInfo(
	// 		vk::InstanceCreateFlags(),
	// 		&appInfo,
	// 		static_cast<uint32_t>(Layers.size()),
	// 		Layers.data(),
	// 		static_cast<uint32_t>(InstanceExtensions.size()),
	// 		InstanceExtensions.data()
	// 	);
	// 	deviceExtensions = 
	// 	{
	// 		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	// 	};
	// 	imageLoadCallback = LoadTextures;
	// 	initData = 
	// 	{
	// 		std::string("Vulkan App"),
	// 		900,
	// 		900,
	// 		instanceCreateInfo,
	// 		deviceExtensions,
	// 		std::string(CONTENTROOT "Shaders/vert.spv"),
	// 		std::string(CONTENTROOT "Shaders/frag.spv"),
	// 		imageLoadCallback
	// 	};
	// 	app.init(initData);
	// 	enttRegistry.prepare<cmp::TextureID, cmp::PositionMatrix, cmp::Color>();
	// 	enttRegistry.prepare<cmp::Position, cmp::PositionMatrix, cmp::Velocity>();
		
	// 	ImageIndex starTexture = static_cast<ImageIndex>(Image::ImageIDToTextureID[Image::Star]);
	// 	for (auto i = 0.f; i < 3.f; i++)
	// 	{
	// 		auto entity = enttRegistry.create(
	// 			cmp::TextureID(starTexture), 
	// 			cmp::Position(glm::vec3(i*2.f, i*2.f, 0.f)),
	// 			cmp::PositionMatrix(),
	// 			cmp::Velocity(glm::vec2()),
	// 			cmp::Color(glm::vec4(1.f)),
	// 			cmp::RectSize());
	// 	}

	// 	struct InputVisitor
	// 	{
	// 		ClientApp* app;

	// 		explicit InputVisitor(ClientApp* app) : app(app) {}

	// 		void operator()(Input::KeyMessage msg)
	// 		{
	// 		}

	// 		void operator()(Input::CharMessage msg)
	// 		{

	// 		}

	// 		void operator()(Input::MouseButtonMessage msg)
	// 		{

	// 		}

	// 		void operator()(Input::CursorPosMessage msg)
	// 		{

	// 		}
			
	// 	};

	// 	vka::LoopCallbacks callbacks;
	// 	callbacks.UpdateCallback = [this](TimePoint_ms timePoint) -> vka::SpriteCount
	// 	{
	// 		bool inputToProcess = true;
	// 		while (inputToProcess)
	// 		{
	// 			auto messageOptional = app.m_InputBuffer.popFirstIf([timePoint](Input::InputMessage msg){ return msg.time < timePoint; });
	// 			auto visitor = InputVisitor(this);
	// 			if (messageOptional.has_value())
	// 			{
	// 				Input::InputMsgVariant& variant = messageOptional.value().variant;
	// 				std::visit(visitor, variant);
	// 			}
	// 		}
			
	// 		auto physicsView = enttRegistry.persistent<cmp::Position, cmp::Velocity, cmp::PositionMatrix>();

	// 		for (auto entity : physicsView)
	// 		{
	// 			auto position = physicsView.get<cmp::Position>(entity);
	// 			auto velocity = physicsView.get<cmp::Velocity>(entity);
	// 			auto transform = physicsView.get<cmp::PositionMatrix>(entity);
	// 		}
	// 		auto renderView = enttRegistry.persistent<cmp::TextureID, cmp::PositionMatrix, cmp::Color>();
	// 		return renderView.size();
	// 	};
	// 	callbacks.RenderCallback = [this]()
	// 	{
	// 		auto view = enttRegistry.persistent<cmp::TextureID, cmp::PositionMatrix, cmp::Color>();
	// 		for (auto entity : view)
	// 		{
	// 			auto textureID = view.get<cmp::TextureID>(entity);
	// 			auto transform = view.get<cmp::PositionMatrix>(entity);
	// 			auto color = view.get<cmp::Color>(entity);
	// 			if (transform.matrix.has_value())
	// 			{
	// 				app.RenderSprite(textureID.index, transform.matrix.value(), color.rgba);
	// 			}
	// 		}
	// 	};
	// 	app.Run(callbacks);

		return 0;
	}
};

int main()
{
	ClientApp clientApp;
	return clientApp.run();
}