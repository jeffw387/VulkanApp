#pragma once

#include "Windows.h"
#include "nowide/convert.hpp"
#include <functional>
#include <iostream>

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
		Windowed = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		Fullscreen = WS_POPUP
	};

	Window() = default;

	Window (std::string WindowClassName, 
		std::string WindowTitle, 
		WindowStyle windowStyle,
		ResizeCallback resizeCallback,
		void* applicationPointer, 
		int width = CW_USEDEFAULT, 
		int height = CW_USEDEFAULT) :
		m_ResizeCallback(resizeCallback),
		m_ApplicationPointer(applicationPointer),
		m_hInstance(GetModuleHandle(0))
	{
		auto className = nowide::widen(WindowClassName).data();
		auto windowTitle = nowide::widen(WindowTitle).data();
		// Register the window class.
		WNDCLASSEX wcex = { };

		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.lpfnWndProc   = WindowProc;
		wcex.hInstance     = m_hInstance;
		wcex.lpszClassName = className;
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);;
		wcex.hIcon = NULL;
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcex.lpszMenuName = NULL;
		wcex.hIconSm = NULL;

		auto registerResult = RegisterClassExW(&wcex);

		//DWORD     dwExStyle		= WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		DWORD     dwStyle 		= WS_OVERLAPPEDWINDOW;
		int       x				= 20;
		int       y				= 20;
		HWND      hWndParent 	= NULL;
		HMENU     hMenu 		= NULL;
		LPVOID    lpParam 		= NULL;
		m_hWnd = HWNDPtr(CreateWindowW(
			className,
			windowTitle,
			WS_OVERLAPPEDWINDOW,
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
				return false;
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

	LRESULT WindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
			case WM_KEYDOWN:
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
		std::cout << "DefWindowProc: " << message << "\n";
		// handle messages
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

private:
	ResizeCallback m_ResizeCallback = nullptr;
	void* m_ApplicationPointer = nullptr;
	HWNDPtr m_hWnd;
	HINSTANCE m_hInstance = nullptr;
};

inline LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Window* window = (Window*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (window == nullptr)
	{
		return true;
	}
	return window->WindowProcedure(hWnd, message, wParam, lParam);
}