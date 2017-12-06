#pragma once

#include <map>
#include <vector>
#include <GLFW/glfw3.h>
using Action = uint32_t;
using KeyCode = int;

enum class InputState
{
	Pressed = GLFW_PRESS,
	Released = GLFW_RELEASE
};

enum class InputType
{
	Keyboard,
	Mouse,
	Touch
};

struct InputBinding
{
	int inputCode;
	InputType inputType;
	InputState current;
	InputState previous;

	bool operator <(InputBinding rhs) const
	{
		if (inputType < rhs.inputType)
			return true;
		else if (inputType == rhs.inputType)
			return inputCode < rhs.inputCode;
		return false;
	}
};

class InputManager
{
public:
	void MapInput(InputBinding&& inputBinding, Action action)
	{
		m_ActionInputMap[action] = inputBinding;
		m_InputActionMap[inputBinding] = action;
	}

	void keyCallback(int key, int scancode, int state, int mods)
	{
		InputBinding lookup;
		lookup.inputType = InputType::Keyboard;
		lookup.inputCode = key;

		auto action = m_InputActionMap[lookup];
		auto& binding = m_ActionInputMap[state];
		binding.previous = binding.current;
		binding.current = static_cast<InputState>(state);
	}

	void cursorPositionCallback(double xpos, double ypos)
	{
		m_MouseX = xpos;
		m_MouseY = ypos;
	}

	void mouseButtonCallback(int button, int state, int mods)
	{
		InputBinding lookup;
		lookup.inputType = InputType::Mouse;
		lookup.inputCode = button;

		auto action = m_InputActionMap[lookup];
		auto& binding = m_ActionInputMap[state];
		binding.previous = binding.current;
		binding.current = static_cast<InputState>(state);
	}

	const InputBinding& getBindingForAction(const Action& action)
	{
		return m_ActionInputMap[action];
	}

	std::tuple<double, double> getMousePosition() { return std::make_tuple(m_MouseX, m_MouseY); }

private:
	std::map<Action, InputBinding> m_ActionInputMap;
	std::map<InputBinding, Action> m_InputActionMap;
	double m_MouseX;
	double m_MouseY;
};