#undef min
#undef max

#define STB_IMAGE_IMPLEMENTATION
#include "TimeHelper.hpp"
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

#ifdef NO_VALIDATION
constexpr bool ReleaseMode = true;
#endif


#include "SpriteData.hpp"

namespace Font
{
    constexpr auto AeroviasBrasil = entt::HashedString(CONTENTROOT "Content/Fonts/AeroviasBrasilNF.ttf");
}

class ClientApp : public vka::VulkanApp
{
    std::vector<const char*> Layers;
    std::vector<const char*> InstanceExtensions;
    std::vector<const char*> DeviceExtensions;
    
    entt::DefaultRegistry enttRegistry;

public:
    void LoadImages()
    {
        auto sheet1 = vka::loadImageFromFile(std::string(Sprites::SpriteSheet1::ImagePath));
        app.LoadImage2D(Sprites::SpriteSheet1::ImagePath, sheet1);
        app.CreateSprite(Sprites::SpriteSheet1::ImagePath, 
            Sprites::SpriteSheet1::starpng::Name, 
            Sprites::SpriteSheet1::starpng::SpriteQuad);
    }

    void Update(TimePoint_ms updateTime)
    {
        bool inputToProcess = true;
        while (inputToProcess)
        {
            auto messageOptional = inputBuffer.popFirstIf(
                    [timePoint](vka::InputMessage message){ return message.time < timePoint; }
                );
            
            if (messageOptional.has_value())
            {
                auto& message = *messageOptional;
                auto bindHash = bindMap[message.signature];
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
        
        auto physicsView = enttRegistry.persistent<cmp::Position, cmp::Velocity, cmp::PositionMatrix>();

        for (auto entity : physicsView)
        {
            auto& position = physicsView.get<cmp::Position>(entity);
            auto& velocity = physicsView.get<cmp::Velocity>(entity);
            auto& transform = physicsView.get<cmp::PositionMatrix>(entity);

            position.position += velocity.velocity;
            transform.matrix = glm::translate(glm::mat4(1.f), 
            glm::vec3(position.position.x,
                position.position.y,
                0.f));
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

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = nullptr;
    appInfo.pApplicationName = nullptr;
    appInfo.applicationVersion = {};
    appInfo.pEngineName = nullptr;
    appInfo.engineVersion = {};
    appInfo.apiVersion = VK_MAKE_VERSION(1,0,0);

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pNext = nullptr;
    instanceCreateInfo.flags = 0;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledLayerCount = Layers.size();
    instanceCreateInfo.ppEnabledLayerNames = Layers.data();
    instanceCreateInfo.enabledExtensionCount = InstanceExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = InstanceExtensions.data();


    vka::InputBindMap bindMap;
    bindMap[vka::MakeSignature(GLFW_KEY_LEFT, GLFW_PRESS)]  = 	Bindings::Left;
    bindMap[vka::MakeSignature(GLFW_KEY_RIGHT, GLFW_PRESS)] = 	Bindings::Right;
    bindMap[vka::MakeSignature(GLFW_KEY_UP, GLFW_PRESS)]    = 	Bindings::Up;
    bindMap[vka::MakeSignature(GLFW_KEY_DOWN, GLFW_PRESS)]  = 	Bindings::Down;

    vka::InputStateMap inputStateMap;
    inputStateMap[Bindings::Left] =  vka::MakeAction([](){ std::cout << "Left Pressed.\n"; });
    inputStateMap[Bindings::Right] = vka::MakeAction([](){ std::cout << "Right Pressed.\n"; });
    inputStateMap[Bindings::Up] =  vka::MakeAction([](){ std::cout << "Up Pressed.\n"; });
    inputStateMap[Bindings::Down] =  vka::MakeAction([](){ std::cout << "Down Pressed.\n"; });

    auto& inputBuffer = m_InputState.inputBuffer;


    enttRegistry.prepare<cmp::Sprite, cmp::PositionMatrix, cmp::Color>();
    enttRegistry.prepare<cmp::Position, cmp::PositionMatrix, cmp::Velocity>();
    for (auto i = 0.f; i < 3.f; i++)
    {
        auto entity = enttRegistry.create(
            cmp::Sprite(Sprites::SpriteSheet1::starpng::Name), 
            cmp::Position(glm::vec2(i*2.f, i*2.f)),
            cmp::PositionMatrix(),
            cmp::Velocity(glm::vec2()),
            cmp::Color(glm::vec4(1.f)),
            cmp::RectSize());
    }

    vka::InitState initState = {};
    initState.windowTitle = std::string("Vulkan App");
    initState.width = 900;
    initState.height = 900;
    initState.instanceCreateInfo = instanceCreateInfo;
    initState.DeviceExtensions = DeviceExtensions;
    initState.vertexShaderPath = std::string(CONTENTROOT "Shaders/vert.spv");
    initState.fragmentShaderPath = std::string(CONTENTROOT "Shaders/frag.spv");
    
    while (true)
    {
        ClientApp clientApp;
        
        auto runResult = clientApp.Run(initState);
        switch (runResult)
        {
            case vka::LoopState::DeviceLost: break;
            case vka::LoopState::Exit: return 0;
            default: return 0;
        }
    }
}