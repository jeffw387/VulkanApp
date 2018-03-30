#pragma once
#include "CircularQueue.hpp"
#include "entt.hpp"
#include "TimeHelper.hpp"
#include <variant>
#include <functional>

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

    template <typename T>
    struct Binding
    {
        using bindType = T;
    };

	enum class BindingEvents
	{
		Start,
		End
	};

	template <typename T>
	struct BindingEvent
	{
		BindingEvents event;
		TimePoint_ms eventTime;
	};

    template <typename T>
    struct InputEvent
    {
        using bindType = T;
        using type = InputEvent<bindType>;
        using func = std::function<void(type)>;
        TimePoint_ms eventTime;
        BindingEvents eventType;
    };
/*[[[cog
import cog
bindNames = ['Up', 'Down', 'Left', 'Right', 'Fire', 'Menu', 'Pause', 'Exit']
bindAppend = "Binding"
eventAppend = "Event"
funcAppend = "Func"

# create the binding structs
for bind in bindNames:
    bindType = bind + bindAppend
    cog.outl("struct %s" % bind)
    cog.outl('{};')
    cog.outl("using {0} = Binding<{1}>;".format(bindType, bind))

# create the binding variant
cog.outl()
cog.outl("using BindVariant = std::variant<")
for index, bind in enumerate(bindNames):
    bindType = bind + bindAppend
    cog.out("    {}".format(bindType))
    if (index + 1) != len(bindNames):
        cog.outl(",")
cog.outl(">;")

# create the event variant
cog.outl()
cog.outl("using EventVariant = std::variant<")
for index, bind in enumerate(bindNames):
    eventType = bind + eventAppend
    cog.out("    {}".format(eventType))
    if (index + 1) != len(bindNames):
        cog.outl(",")
cog.outl(">;")

# create the event variant visitor
cog.outl()
cog.outl("struct EventVisitor")
cog.outl("{")
for bind in bindNames:
    eventType = bind + eventAppend
    eventFunc = bind + funcAppend
    cog.outl("    {0}::func {1};".format(eventType, eventFunc))
# void operator()
cog.outl()
for bind in bindNames:
    eventType = bind + eventAppend
    funcName = bind + funcAppend
    cog.outl("    void operator()({0} event)".format(eventType))
    cog.outl("    {")
    cog.outl("        {0}(event);".format(funcName))
    cog.outl("    }")
    cog.outl()
cog.outl("};")

# create the binding variant visitor
cog.outl()
cog.outl("struct BindingVisitor")
cog.outl("{")
# data members
cog.outl("    EventVisitor eventVisitor;")
cog.outl("    TimePoint_ms eventTime;")
cog.outl("    BindingEvents eventType;")
# void operator()
cog.outl()
for bind in bindNames:
    bindType = bind + bindAppend
    eventType = bind + eventAppend
    cog.outl("    void operator()({0} binding)".format(bindType))
    cog.outl("    {")
    cog.outl("        auto event = {0}();".format(eventType))
    cog.outl("        event.eventTime = eventTime;")
    cog.outl("        event.eventType = eventType;")
    cog.outl("        EventVariant eventVariant = event;")
    cog.outl("        std::visit(eventVisitor, eventVariant);")
    cog.outl("    }")
    cog.outl()
cog.outl("};")


]]]*/
struct Up
{};
using UpBinding = Binding<Up>;
struct Down
{};
using DownBinding = Binding<Down>;
struct Left
{};
using LeftBinding = Binding<Left>;
struct Right
{};
using RightBinding = Binding<Right>;
struct Fire
{};
using FireBinding = Binding<Fire>;
struct Menu
{};
using MenuBinding = Binding<Menu>;
struct Pause
{};
using PauseBinding = Binding<Pause>;
struct Exit
{};
using ExitBinding = Binding<Exit>;

using BindVariant = std::variant<
    UpBinding,
    DownBinding,
    LeftBinding,
    RightBinding,
    FireBinding,
    MenuBinding,
    PauseBinding,
    ExitBinding>;

using EventVariant = std::variant<
    UpEvent,
    DownEvent,
    LeftEvent,
    RightEvent,
    FireEvent,
    MenuEvent,
    PauseEvent,
    ExitEvent>;

struct EventVisitor
{
    UpEvent::func UpFunc;
    DownEvent::func DownFunc;
    LeftEvent::func LeftFunc;
    RightEvent::func RightFunc;
    FireEvent::func FireFunc;
    MenuEvent::func MenuFunc;
    PauseEvent::func PauseFunc;
    ExitEvent::func ExitFunc;

    void operator()(UpEvent event)
    {
        UpFunc(event);
    }

    void operator()(DownEvent event)
    {
        DownFunc(event);
    }

    void operator()(LeftEvent event)
    {
        LeftFunc(event);
    }

    void operator()(RightEvent event)
    {
        RightFunc(event);
    }

    void operator()(FireEvent event)
    {
        FireFunc(event);
    }

    void operator()(MenuEvent event)
    {
        MenuFunc(event);
    }

    void operator()(PauseEvent event)
    {
        PauseFunc(event);
    }

    void operator()(ExitEvent event)
    {
        ExitFunc(event);
    }

};

struct BindingVisitor
{
    EventVisitor eventVisitor;
    TimePoint_ms eventTime;
    BindingEvents eventType;

    void operator()(UpBinding binding)
    {
        auto event = UpEvent();
        event.eventTime = eventTime;
        event.eventType = eventType;
        EventVariant eventVariant = event;
        std::visit(eventVisitor, eventVariant);
    }

    void operator()(DownBinding binding)
    {
        auto event = DownEvent();
        event.eventTime = eventTime;
        event.eventType = eventType;
        EventVariant eventVariant = event;
        std::visit(eventVisitor, eventVariant);
    }

    void operator()(LeftBinding binding)
    {
        auto event = LeftEvent();
        event.eventTime = eventTime;
        event.eventType = eventType;
        EventVariant eventVariant = event;
        std::visit(eventVisitor, eventVariant);
    }

    void operator()(RightBinding binding)
    {
        auto event = RightEvent();
        event.eventTime = eventTime;
        event.eventType = eventType;
        EventVariant eventVariant = event;
        std::visit(eventVisitor, eventVariant);
    }

    void operator()(FireBinding binding)
    {
        auto event = FireEvent();
        event.eventTime = eventTime;
        event.eventType = eventType;
        EventVariant eventVariant = event;
        std::visit(eventVisitor, eventVariant);
    }

    void operator()(MenuBinding binding)
    {
        auto event = MenuEvent();
        event.eventTime = eventTime;
        event.eventType = eventType;
        EventVariant eventVariant = event;
        std::visit(eventVisitor, eventVariant);
    }

    void operator()(PauseBinding binding)
    {
        auto event = PauseEvent();
        event.eventTime = eventTime;
        event.eventType = eventType;
        EventVariant eventVariant = event;
        std::visit(eventVisitor, eventVariant);
    }

    void operator()(ExitBinding binding)
    {
        auto event = ExitEvent();
        event.eventTime = eventTime;
        event.eventType = eventType;
        EventVariant eventVariant = event;
        std::visit(eventVisitor, eventVariant);
    }

};
//[[[end]]]

	struct InputState
	{
		CircularQueue<InputMessage, 500> inputBuffer;
		// TODO: possibly convert action map to vector
		std::map<InputMsgVariant, BindVariant> playerEventBindings;
	};
}