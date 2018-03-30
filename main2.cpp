#undef min
#undef max

#define VMA_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "vka/VulkanApp2.hpp"
#include <algorithm>
#include <iostream>
#include <map>

#include "ECSComponents.hpp"
#include "entt.hpp"
#undef min
#undef max

#ifndef CONTENTROOT
#define CONTENTROOT
#endif

#include "SpriteData.hpp"

namespace Image
{
	constexpr auto Star = entt::HashedString(CONTENTROOT "Content/Textures/star.png");
	constexpr auto Sheet1 = entt::HashedString(CONTENTROOT "Content/SpriteSheets/spritesheet.png");
}

namespace SpriteSheet
{
	constexpr auto Sheet1 = entt::HashedString(CONTENTROOT "Content/SpriteSheets/spritesheet.json");
}

namespace Font
{
	constexpr auto AeroviasBrasil = entt::HashedString(CONTENTROOT "Content/Fonts/AeroviasBrasilNF.ttf");
}

class ClientApp
{
	std::vector<const char*> Layers;
	std::vector<const char*> InstanceExtensions;
	vk::ApplicationInfo appInfo;
	vk::InstanceCreateInfo instanceCreateInfo;
	std::vector<const char*> deviceExtensions;
	vka::VulkanApp app;
	entt::DefaultRegistry enttRegistry;

public:
	struct PlayerInputBindingVisitor
	{
		ClientApp* app;

		explicit PlayerInputBindingVisitor(ClientApp* app) : app(app) {}

		void operator()(vka::KeyMessage msg)
		{
		}

		void operator()(vka::CharMessage msg)
		{

		}

		void operator()(vka::MouseButtonMessage msg)
		{

		}

		void operator()(vka::CursorPosMessage msg)
		{

		}
	};


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

		auto loadImageCallback = [this]()
		{
			auto sheet1 = vka::loadImageFromFile(std::string(Image::Sheet1));
			app.LoadImage2D(Image::Sheet1, sheet1);

		};

		auto updateCallback = [this](TimePoint_ms timePoint)
		{
			auto& bindings = app.m_InputState.playerEventBindings;
			bool inputToProcess = true;
			while (inputToProcess)
			{
				auto messageOptional = app.m_InputState.inputBuffer.popFirstIf(
						[timePoint](vka::InputMessage msg){ return msg.time < timePoint; }
					);
				
				if (messageOptional.has_value())
				{
					auto bindingVisitor = vka::BindingVisitor();
					bindingVisitor.eventTime = messageOptional->time;
					bindingVisitor.eventType = messageOptional->
					vka::InputMsgVariant& variant = messageOptional.value().variant;
					auto bindingIterator = bindings.find(variant);
					if (bindingIterator != bindings.end())
					{
						auto& bindingVariant = *bindingIterator;
						std::visit(visitor, bindingVariant);
					}
				}
				else
				{
					inputToProcess = false;
				}
			}
			
			auto physicsView = enttRegistry.persistent<cmp::Position, cmp::Velocity, cmp::PositionMatrix>();

			for (auto entity : physicsView)
			{
				auto& position = physicsView.get<cmp::Position>(entity);
				auto& velocity = physicsView.get<cmp::Velocity>(entity);
				auto& transform = physicsView.get<cmp::PositionMatrix>(entity);

				position.position += velocity.velocity;
			}
			auto renderView = enttRegistry.persistent<cmp::Sprite, cmp::PositionMatrix, cmp::Color>();
			return renderView.size();
		};

		auto drawCallback = [this]()
		{
			auto view = enttRegistry.persistent<cmp::Sprite, cmp::PositionMatrix, cmp::Color>();
			for (auto entity : view)
			{
				auto& sprite = view.get<cmp::Sprite>(entity);
				auto& transform = view.get<cmp::PositionMatrix>(entity);
				auto& color = view.get<cmp::Color>(entity);
				if (transform.matrix.has_value())
				{
					app.RenderSpriteInstance(sprite.index, transform.matrix.value(), color.rgba);
				}
			}
		};
		app.init(std::string("Vulkan App"),
		900,
		900,
		instanceCreateInfo,
		deviceExtensions,
		std::string(CONTENTROOT "Shaders/vert.spv"),
		std::string(CONTENTROOT "Shaders/frag.spv"),
		loadImageCallback,
		updateCallback,
		drawCallback);
		enttRegistry.prepare<cmp::Sprite, cmp::PositionMatrix, cmp::Color>();
		enttRegistry.prepare<cmp::Position, cmp::PositionMatrix, cmp::Velocity>();
		
		for (auto i = 0.f; i < 3.f; i++)
		{
			auto entity = enttRegistry.create(
				cmp::Sprite(Image::Star), 
				cmp::Position(glm::vec2(i*2.f, i*2.f)),
				cmp::PositionMatrix(),
				cmp::Velocity(glm::vec2()),
				cmp::Color(glm::vec4(1.f)),
				cmp::RectSize());
		}

		app.Run();

		return 0;
	}
};

int main()
{
	ClientApp clientApp;
	return clientApp.run();
}