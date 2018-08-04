#pragma once
#include <functional>
#include <variant>
#include "CircularQueue.hpp"
#include "TimeHelper.hpp"
#include "entt/entt.hpp"

namespace vka {
using HashType = entt::HashedString::hash_type;
struct KeySignature {
  int code = 0;
  int action = 0;
};

static KeySignature MakeSignature(int code, int action) {
  KeySignature signature;
  signature.code = code;
  signature.action = action;
  return signature;
}

static bool operator<(const KeySignature& a, const KeySignature& b) {
  return (a.code < b.code) && (a.action < b.action);
}

static bool operator==(const KeySignature& a, const KeySignature& b) {
  return (a.code == b.code) && (a.action == b.action);
}
}  // namespace vka

namespace std {
template <>
struct hash<vka::KeySignature> {
  size_t operator()(const vka::KeySignature& msg) const {
    return ((hash<int>()(msg.code) ^ (hash<int>()(msg.action) << 1)) >> 1);
  }
};
}  // namespace std

namespace vka {
struct InputMessage {
  TimePoint_ms time;
  KeySignature signature;
};

struct Action {
  std::function<void()> func = nullptr;
};

static Action MakeAction(std::function<void()> func) {
  Action action;
  action.func = func;
  return action;
}

struct State {
  bool value;

  void SetTrue() { value = true; }
  void SetFalse() { value = false; }
};

using StateVariant = std::variant<Action, State>;

struct StateVisitor {
  KeySignature signature;
  void operator()(Action& action) {
    if (action.func != nullptr) action.func();
  }
  void operator()(State& state) {
    switch (signature.action) {
      case GLFW_PRESS: {
        state.SetTrue();
      }
      case GLFW_RELEASE: {
        state.SetFalse();
      }
    }
  }
};

using InputBindMap = std::unordered_map<KeySignature, HashType>;
using InputStateMap = std::unordered_map<HashType, StateVariant>;

struct InputState {
  InputBindMap inputBindMap;
  InputStateMap inputStateMap;
  CircularQueue<InputMessage, 500> inputBuffer;
  double cursorX;
  double cursorY;
};
}  // namespace vka

/*[[[cog
import cog
bindNames = ['Up', 'Down', 'Left', 'Right', 'Fire', 'Menu', 'Pause', 'Exit']

# create the binding structs
cog.outl("namespace Bindings")
cog.outl("{")
for bind in bindNames:
    cog.outl("    constexpr auto {0} =
entt::HashedString(\"{0}\");".format(bind)) cog.outl("}")
]]]*/
namespace Bindings {
constexpr auto Up = entt::HashedString("Up");
constexpr auto Down = entt::HashedString("Down");
constexpr auto Left = entt::HashedString("Left");
constexpr auto Right = entt::HashedString("Right");
constexpr auto Fire = entt::HashedString("Fire");
constexpr auto Menu = entt::HashedString("Menu");
constexpr auto Pause = entt::HashedString("Pause");
constexpr auto Exit = entt::HashedString("Exit");
}  // namespace Bindings
   //[[[end]]]