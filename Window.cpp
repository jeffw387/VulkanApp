#include "Window.hpp"

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Window* window = (Window*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (window == nullptr)
	{
		return true;
	}
	return window->WindowProcedure(hWnd, message, wParam, lParam);
}

void Window::SetResizeCallback(ResizeCallback resizeCallback)
{
	m_ResizeCallback = resizeCallback;
}
