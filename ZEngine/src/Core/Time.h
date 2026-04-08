#pragma once
#include <chrono>

class Time
{
public:
	static void Init();
	static void Update();

	// Trả về thời gian trôi qua giữa 2 frame (tính bằng giây)
	static float GetDeltaTime() { return s_DeltaTime; }

	// Trả về tổng thời gian từ lúc Engine bắt đầu chạy (tính bằng giây)
	static float GetTotalTime() { return s_TotalTime; }

private:
	static std::chrono::time_point<std::chrono::high_resolution_clock> s_StartTime;
	static std::chrono::time_point<std::chrono::high_resolution_clock> s_LastFrameTime;

	static float s_DeltaTime;
	static float s_TotalTime;
};