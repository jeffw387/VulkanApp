#undef min
#undef max

#define STB_IMAGE_IMPLEMENTATION
#include "TimeHelper.hpp"
#include "vka/VulkanApp.hpp"
#include <algorithm>
#include <iostream>
#include <map>
#include <optional>

#include "ECSComponents.hpp"
#include "entt.hpp"
#undef min
#undef max

#ifndef CONTENTROOT
#define CONTENTROOT
#endif

#include "SpriteData.hpp"
const char* ConfigPath = CONTENTROOT "VulkanInitInfo.json";
const char* VertexShaderPath = CONTENTROOT "Shaders/vert.spv";
const char* FragmentShaderPath = CONTENTROOT "Shaders/frag.spv";

namespace Font
{
    constexpr auto AeroviasBrasil = entt::HashedString(CONTENTROOT "Content/Fonts/AeroviasBrasilNF.ttf");
}

class ClientApp : public vka::VulkanApp
{
public:
    entt::DefaultRegistry enttRegistry;

    void LoadImages()
    {
        auto sheet1 = vka::loadImageFromFile(std::string(Sprites::SpriteSheet1::ImagePath));
        LoadImage2D(Sprites::SpriteSheet1::ImagePath, sheet1);
        CreateSprite(Sprites::SpriteSheet1::ImagePath, 
            Sprites::SpriteSheet1::starpng::Name, 
            Sprites::SpriteSheet1::starpng::SpriteQuad);
    }

    void Update(TimePoint_ms updateTime)
    {
        bool inputToProcess = true;
        while (inputToProcess)
        {
            auto messageOptional = inputBuffer.popFirstIf(
                [updateTime](vka::InputMessage message){ return message.time < updateTime; });
            
            if (messageOptional.has_value())
            {
                auto& message = *messageOptional;
                auto bindHash = inputBindMap[message.signature];
                auto stateVariant = inputStateMap[bindHash];
                vka::StateVisitor visitor;
                visitor.signature = message.signature;
                std::visit(visitor, stateVariant);
            }
            else
            {
                inputToProcess = false;
            }
        }
        
        auto physicsView = enttRegistry.persistent<cmp::Engine, cmp::Velocity, cmp::Position, cmp::PositionMatrix>();

        for (auto entity : physicsView)
        {
            auto& engine = physics.get<cmp::Engine>(entity);
            auto& velocity = physicsView.get<cmp::Velocity>(entity);
            auto& position = physicsView.get<cmp::Position>(entity);
            auto& transform = physicsView.get<cmp::PositionMatrix>(entity);

            
        }
    }

    void Draw()
    {
        auto view = enttRegistry.persistent<cmp::Sprite, cmp::PositionMatrix, cmp::Color>();
        for (auto entity : view)
        {
            auto& sprite = view.get<cmp::Sprite>(entity);
            auto& transform = view.get<cmp::PositionMatrix>(entity);
            auto& color = view.get<cmp::Color>(entity);
            if (transform.matrix.has_value())
            {
                RenderSpriteInstance(sprite.index, transform.matrix.value(), color.rgba);
            }
        }
    }
};

int main()
{
    ClientApp app;
    
    app.inputBindMap[vka::MakeSignature(GLFW_KEY_LEFT, GLFW_PRESS)]  = 	Bindings::Left;
    app.inputBindMap[vka::MakeSignature(GLFW_KEY_RIGHT, GLFW_PRESS)] = 	Bindings::Right;
    app.inputBindMap[vka::MakeSignature(GLFW_KEY_UP, GLFW_PRESS)]    = 	Bindings::Up;
    app.inputBindMap[vka::MakeSignature(GLFW_KEY_DOWN, GLFW_PRESS)]  = 	Bindings::Down;

    app.inputStateMap[Bindings::Left] =  vka::MakeAction([](){ std::cout << "Left Pressed.\n"; });
    app.inputStateMap[Bindings::Right] = vka::MakeAction([](){ std::cout << "Right Pressed.\n"; });
    app.inputStateMap[Bindings::Up] =  vka::MakeAction([](){ std::cout << "Up Pressed.\n"; });
    app.inputStateMap[Bindings::Down] =  vka::MakeAction([](){ std::cout << "Down Pressed.\n"; });

    // render view
    app.enttRegistry.prepare<cmp::Sprite, cmp::PositionMatrix, cmp::Color>();
    // physics view
    app.enttRegistry.prepare<cmp::Engine, cmp::Velocity, cmp::Position, cmp::PositionMatrix>();
    for (auto i = 0.f; i < 3.f; i++)
    {
        auto entity = app.enttRegistry.create(
            cmp::Sprite(Sprites::SpriteSheet1::starpng::Name), 
            cmp::Position(glm::vec2(i*20.f, i*20.f)),
            cmp::PositionMatrix(),
            cmp::Velocity(glm::vec2()),
            cmp::Color(glm::vec4(1.f)),
            cmp::RectSize());
    }

    app.Run(std::string(ConfigPath), std::string(VertexShaderPath), std::string(FragmentShaderPath));
}