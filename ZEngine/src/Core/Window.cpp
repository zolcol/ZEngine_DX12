#include "pch.h"

#include "Window.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Hàm xử lý sự kiện Win32
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
		return true;

	if (uMsg == WM_DESTROY)
	{
		PostQuitMessage(0);
		return 0;
	}
	if (uMsg == WM_KEYDOWN)
	{
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
			return 0;
		}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

Window::Window(int width, int height, std::wstring title) : m_className(L"DX12WindowClass")
{
	m_hInstance = GetModuleHandle(nullptr);

	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = m_hInstance;
	wc.lpszClassName = m_className.c_str();
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);

	RegisterClass(&wc);

	// Tính toán kích thước cửa sổ dựa trên Client Area (phần nội dung hiển thị)
	RECT wr = { 0, 0, (LONG)width, (LONG)height };
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

	m_hwnd = CreateWindowEx(
		0, m_className.c_str(), title.c_str(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		wr.right - wr.left, wr.bottom - wr.top,
		NULL, NULL, m_hInstance, NULL
	);
}

Window::~Window()
{
	UnregisterClass(m_className.c_str(), m_hInstance);
}

void Window::Show()
{
	ShowWindow(m_hwnd, SW_SHOW);
}

bool Window::ProcessMessages()
{
	MSG msg = {};
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT) return false;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return true;
}
