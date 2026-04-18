#pragma once

class Input
{
public:
	static void Init();
	static void Update();

	// Keyboard
	static bool IsKeyPressed(int keyCode);

	// Mouse
	static bool IsMouseButtonPressed(int button);
	static DirectX::XMFLOAT2 GetMousePosition() { return s_MousePos; }
	static DirectX::XMFLOAT2 GetMouseDelta() { return s_MouseDelta; }
	static float GetMouseWheelDelta() { return s_MouseWheelDelta; }

	// Được gọi từ WindowProc
	static void ProcessRawInput(LPARAM lParam);
	static void SetMouseWheelDelta(float delta) { s_MouseWheelDelta = delta; }

private:
	static DirectX::XMFLOAT2 s_MousePos;
	static DirectX::XMFLOAT2 s_MouseDelta;
	static float s_MouseWheelDelta;

	static bool s_CurrentKeys[256];
};
