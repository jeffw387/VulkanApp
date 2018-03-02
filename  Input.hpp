#pragma once
#include <variant>
#include "entt.hpp"
#include "TimeHelper.hpp"

namespace Input
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
        if (index + 1) != len(bindNames)
            cog.outl(",")
	cog.outl(">;")
	]]]*/
