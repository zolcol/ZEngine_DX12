#include "pch.h"
#include "Input.h"

DirectX::XMFLOAT2 Input::s_MousePos = { 0, 0 };
DirectX::XMFLOAT2 Input::s_MouseDelta = { 0, 0 };
float Input::s_MouseWheelDelta = 0;
bool Input::s_CurrentKeys[256] = { false };

void Input::Init()
{
	// Đăng ký Raw Input cho Chuột
	RAWINPUTDEVICE rid;
	rid.usUsagePage = 0x01; // Generic Desktop Controls
	rid.usUsage = 0x02;     // Mouse
	rid.dwFlags = 0;
	rid.hwndTarget = 0;

	if (!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
	{
		ENGINE_ERROR("Failed to register Raw Input devices!");
	}
}

void Input::Update()
{
	// 1. Cập nhật Mouse Position (Screen Space)
	POINT p;
	if (GetCursorPos(&p))
	{
		s_MousePos.x = static_cast<float>(p.x);
		s_MousePos.y = static_cast<float>(p.y);
	}

	// 2. Reset Mouse Delta & Wheel sau khi dùng (sẽ được cập nhật lại bởi WindowProc)
	s_MouseDelta = { 0, 0 };
	s_MouseWheelDelta = 0;

	// 3. Cập nhật Bàn phím bằng GetAsyncKeyState
	// (Có thể tối ưu bằng cách chỉ check các phím cần thiết nếu muốn)
	for (int i = 0; i < 256; i++)
	{
		s_CurrentKeys[i] = (GetAsyncKeyState(i) & 0x8000) != 0;
	}
}

bool Input::IsKeyPressed(int keyCode)
{
	if (keyCode < 0 || keyCode >= 256) return false;
	return s_CurrentKeys[keyCode];
}

bool Input::IsMouseButtonPressed(int button)
{
	// 0: Left, 1: Right, 2: Middle
	int vKey = (button == 0) ? VK_LBUTTON : (button == 1 ? VK_RBUTTON : VK_MBUTTON);
	return (GetAsyncKeyState(vKey) & 0x8000) != 0;
}

void Input::ProcessRawInput(LPARAM lParam)
{
	UINT dwSize = sizeof(RAWINPUT);
	static BYTE lpb[sizeof(RAWINPUT)];

	GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));

	RAWINPUT* raw = (RAWINPUT*)lpb;

	if (raw->header.dwType == RIM_TYPEMOUSE)
	{
		s_MouseDelta.x += static_cast<float>(raw->data.mouse.lLastX);
		s_MouseDelta.y += static_cast<float>(raw->data.mouse.lLastY);
	}
}
