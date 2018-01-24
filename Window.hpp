#pragma once

#include "Windows.h"
#include <functional>

struct ClientSize
{
	int width;
	int height;
};
using ResizeCallback = std::function<void(void*, ClientSize)>;

struct HWNDDeleter
{
	using pointer = HWND;
	void operator()(HWND p) const 
	{
		if (p != NULL)
			DestroyWindow(p);
	};
};
using HWNDPtr = std::unique_ptr<HWND, HWNDDeleter>;

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

class Window
{
public:
	enum WindowStyle
	{
		Windowed = WS_OVERLAPPEDWINDOW,
		Fullscreen = WS_POPUP
	};

	Window() = default;

	Window (char const* WindowClassName, char const* WindowTitle, WindowStyle windowStyle, void* applicationPointer, int width = CW_USEDEFAULT, int height = CW_USEDEFAULT)
	{
		m_ApplicationPointer = applicationPointer;
		m_hInstance = GetModuleHandle( nullptr );
		// Register the window class.
		WNDCLASSEX wcex = { };

		wcex.lpfnWndProc   = WindowProc;
		wcex.hInstance     = m_hInstance;
		wcex.lpszClassName = LPCTSTR(WindowClassName);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcex.cbSize = sizeof(WNDCLASSEX);

		auto registerResult = RegisterClassEx(&wcex);

		DWORD     dwExStyle		= 0;
		LPCTSTR   lpClassName 	= LPCTSTR(WindowClassName);
		LPCTSTR   lpWindowName 	= LPCTSTR(WindowTitle);
		DWORD     dwStyle 		= windowStyle;
		int       x				= CW_USEDEFAULT;
		int       y				= CW_USEDEFAULT;
		HWND      hWndParent 	= NULL;
		HMENU     hMenu 		= NULL;
		LPVOID    lpParam 		= NULL;
		m_hWnd = HWNDPtr(CreateWindowEx(
			dwExStyle,
			lpClassName,
			lpWindowName,
			dwStyle,
			x,
			y,
			width,
			height,
			hWndParent,
			hMenu,
			m_hInstance,
			lpParam
		));

		SetWindowLongPtr(m_hWnd.get(), GWLP_USERDATA, (LONG_PTR)this);

		ShowWindow(m_hWnd.get(), SW_SHOW);
	}

	HINSTANCE getHINSTANCE() const
	{
		return m_hInstance;
	}

	HWND getHWND() const
	{
		return m_hWnd.get();
	}

	// returns false if app should close
	bool PollEvents()
	{
		MSG msg;
		bool loop = true;
		while (loop)
		{
			auto result = PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE);
			if (result == 0)
			{
				return true;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
			{
				loop = false;
				return loop;
			}
			loop = result;
		}
		return true;
	}

	ClientSize GetClientSize() const
	{
		RECT rect;
		GetClientRect(m_hWnd.get(), &rect);

		return {rect.right, rect.bottom};
	}

	void SetResizeCallback(ResizeCallback resizeCallback);

	LRESULT WindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
			case WM_DESTROY:
			{
				PostQuitMessage(0);
				break;
			}
			case WM_SIZE:
			{
				if (m_ResizeCallback != nullptr)
					m_ResizeCallback(m_ApplicationPointer, GetClientSize());
				break;
			}
		}
		// handle messages
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

private:
	ResizeCallback m_ResizeCallback = nullptr;
	void* m_ApplicationPointer = nullptr;
	HWNDPtr m_hWnd;
	HINSTANCE m_hInstance = nullptr;
};