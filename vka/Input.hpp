#pragma once
#include "CircularQueue.hpp"
#include "entt/entt.hpp"
#include "TimeHelper.hpp"
#include <variant>
#include <functional>

namespace vka
{
    using HashType = entt::HashedString::hash_type;
    struct KeySignature
    {
        int code = 0;
        int action = 0;
    };

    static KeySignature MakeSignature(int code, int action)
    {
        KeySignature signature;
        signature.code = code;
        signature.action = action;
        return signature;
    }

    static bool operator <(const KeySignature& a, const KeySignature& b)
    {
        return (a.code < b.code) && (a.action < b.action);
    }

    static bool operator ==(const KeySignature& a, const KeySignature& b)
    {
        return (a.code == b.code) && (a.action == b.action);
    }
}

namespace std
{
    template <>
    struct hash<vka::KeySignature>
    {
        size_t operator()(const vka::KeySignature& msg) const
        {
            return ((hash<int>()(msg.code)
                ^ (hash<int>()(msg.action) << 1)) >> 1);
        }
    };
}

namespace vka
{
    struct InputMessage
    {
        TimePoint_ms time;
        KeySignature signature;
    };

    struct Action
    {
        std::function<void()> func = nullptr;
    };

    static Action MakeAction(std::function<void()> func)
    {
        Action action;
        action.func = func;
        return action;
    }

    struct State
    {
        bool value;
        
        void SetTrue()
        {
            value = true;
        }
        void SetFalse()
        {
            value = false;
        }
    };

    using StateVariant = std::variant<Action, State>;

    struct StateVisitor
    {
        KeySignature signature;
        void operator()(Action& action)
        {
            if (action.func != nullptr)
                action.func();
        }
        void operator()(State& state)
        {
            switch (signature.action)
            {
                case GLFW_PRESS:
                {
                    state.SetTrue();
                }
                case GLFW_RELEASE:
                {
                    state.SetFalse();
                }
            }
        }
    };

    using InputBindMap = std::unordered_map<KeySignature, HashType>;
    using InputStateMap = std::unordered_map<HashType, StateVariant>;

    static void PushBackInput(GLFWwindow* window, InputMessage&& msg);

    static void SetCursorPosition(GLFWwindow* window, double x, double y);

    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        if (action == GLFW_REPEAT)
            return;
        InputMessage msg;
        msg.signature.code = key;
        msg.signature.action = action;
        PushBackInput(window, std::move(msg));
    }

    static void CharacterCallback(GLFWwindow* window, unsigned int codepoint)
    {
    }

    static void CursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
    {
        SetCursorPosition(window, xpos, ypos);
    }

    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
    {
        InputMessage msg;
        msg.signature.code = button;
        msg.signature.action = action;
        PushBackInput(window, std::move(msg));
    }
}

/*[[[cog
import cog
bindNames = ['Up', 'Down', 'Left', 'Right', 'Fire', 'Menu', 'Pause', 'Exit']

# create the binding structs
cog.outl("namespace Bindings")
cog.outl("{")
for bind in bindNames:
    cog.outl("    constexpr auto {0} = entt::HashedString(\"{0}\");".format(bind))
cog.outl("}")
]]]*/
namespace Bindings
{
    constexpr auto Up = entt::HashedString("Up");
    constexpr auto Down = entt::HashedString("Down");
    constexpr auto Left = entt::HashedString("Left");
    constexpr auto Right = entt::HashedString("Right");
    constexpr auto Fire = entt::HashedString("Fire");
    constexpr auto Menu = entt::HashedString("Menu");
    constexpr auto Pause = entt::HashedString("Pause");
    constexpr auto Exit = entt::HashedString("Exit");
}
//[[[end]]]