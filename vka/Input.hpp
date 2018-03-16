#pragma once
#include <variant>
#include "CircularQueue.hpp"
#include "entt.hpp"
#include "TimeHelper.hpp"

namespace vka
{
	struct KeyMessage
	{
		int scancode = 0;
		int action = 0;
	};

	bool operator <(const KeyMessage& a, const KeyMessage& b)
	{
		return (a.scancode < b.scancode) && (a.action < b.action);
	}

	struct CharMessage
	{
		unsigned int codepoint = 0;
	};

	bool operator <(const CharMessage& a, const CharMessage& b)
	{
		return a.codepoint < b.codepoint;
	}

	struct CursorPosMessage
	{
		double xpos = 0.0;
		double ypos = 0.0;
	};

	bool operator <(const CursorPosMessage& a, const CursorPosMessage& b)
	{
		return (a.xpos < b.xpos) && (a.ypos < b.ypos);
	}

	struct MouseButtonMessage
	{
		int button = 0;
		int action = 0;
	};
	
	bool operator <(const MouseButtonMessage& a, const MouseButtonMessage& b)
	{
		return (a.button < b.button) && (a.action < b.action);
	}

	using InputMsgVariant = std::variant<
		KeyMessage,
		CharMessage,
		CursorPosMessage,
		MouseButtonMessage>;

	struct InputMessage
	{
		TimePoint_ms time;
		InputMsgVariant variant;
	};

	static void PushBackInput(GLFWwindow* window, InputMessage&& msg);

	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		if (action == GLFW_REPEAT)
			return;
		InputMessage msg;
		KeyMessage keyMsg;
		keyMsg.scancode = scancode;
		keyMsg.action = action;
		msg.variant = keyMsg;
		PushBackInput(window, std::move(msg));
	}

	static void CharacterCallback(GLFWwindow* window, unsigned int codepoint)
	{
		InputMessage msg;
		CharMessage charMsg;
		charMsg.codepoint = codepoint;
		msg.variant = charMsg;
		PushBackInput(window, std::move(msg));
	}

	static void CursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
	{
		InputMessage msg;
		CursorPosMessage cursorPosMsg;
		cursorPosMsg.xpos = xpos;
		cursorPosMsg.ypos = ypos;
		msg.variant = cursorPosMsg;
		PushBackInput(window, std::move(msg));
	}

	static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
	{
		InputMessage msg;
		MouseButtonMessage mouseButtonMsg;
		mouseButtonMsg.button = button;
		mouseButtonMsg.action = action;
		msg.variant = mouseButtonMsg;
		PushBackInput(window, std::move(msg));
	}

/*[[[cog
import cog
bindNames = ['Up', 'Down', 'Left', 'Right', 'Fire', 'Menu', 'Pause', 'Exit']

for bind in bindNames:
    cog.outl("struct Start%sEvent { TimePoint_ms time; };" % bind)
    cog.outl("struct End%sEvent { TimePoint_ms time; };" % bind)
    cog.outl("struct %sBinding {};" % bind)
    cog.outl()

cog.outl()
cog.outl("using PlayerEventVariant = std::variant<")
for index, bind in enumerate(bindNames):
    cog.outl("    Start%sEvent," % bind)
    cog.out("    End%sEvent" % bind)
    if (index + 1) != len(bindNames):
        cog.outl(",")
cog.outl(">;")
]]]*/
struct StartUpEvent { TimePoint_ms time; };
struct EndUpEvent { TimePoint_ms time; };
struct UpBinding {};

struct StartDownEvent { TimePoint_ms time; };
struct EndDownEvent { TimePoint_ms time; };
struct DownBinding {};

struct StartLeftEvent { TimePoint_ms time; };
struct EndLeftEvent { TimePoint_ms time; };
struct LeftBinding {};

struct StartRightEvent { TimePoint_ms time; };
struct EndRightEvent { TimePoint_ms time; };
struct RightBinding {};

struct StartFireEvent { TimePoint_ms time; };
struct EndFireEvent { TimePoint_ms time; };
struct FireBinding {};

struct StartMenuEvent { TimePoint_ms time; };
struct EndMenuEvent { TimePoint_ms time; };
struct MenuBinding {};

struct StartPauseEvent { TimePoint_ms time; };
struct EndPauseEvent { TimePoint_ms time; };
struct PauseBinding {};

struct StartExitEvent { TimePoint_ms time; };
struct EndExitEvent { TimePoint_ms time; };
struct ExitBinding {};


using PlayerEventVariant = std::variant<
    StartUpEvent,
    EndUpEvent,
    StartDownEvent,
    EndDownEvent,
    StartLeftEvent,
    EndLeftEvent,
    StartRightEvent,
    EndRightEvent,
    StartFireEvent,
    EndFireEvent,
    StartMenuEvent,
    EndMenuEvent,
    StartPauseEvent,
    EndPauseEvent,
    StartExitEvent,
    EndExitEvent>;
//[[[end]]]

	struct InputState
	{
		CircularQueue<InputMessage, 500> inputBuffer;
		// TODO: possibly convert action map to vector
		std::map<InputMsgVariant, PlayerEventVariant> playerEventBindings;
	};
}