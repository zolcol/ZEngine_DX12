#pragma once

class Window
{
public:
	Window(int width, int height, std::wstring title);
	~Window();

	HWND GetHWND() const { return m_hwnd; }
	bool ProcessMessages(); // Vòng lặp tin nhắn (Message Loop)
	void Show();

private:
	HWND m_hwnd;
	HINSTANCE m_hInstance;
	std::wstring m_className;
};
