#include "pch.h"
#include "Time.h"

// Định nghĩa các biến static
std::chrono::time_point<std::chrono::high_resolution_clock> Time::s_StartTime;
std::chrono::time_point<std::chrono::high_resolution_clock> Time::s_LastFrameTime;
float Time::s_DeltaTime = 0.0f;
float Time::s_TotalTime = 0.0f;

void Time::Init()
{
	s_StartTime = std::chrono::high_resolution_clock::now();
	s_LastFrameTime = s_StartTime;
}

void Time::Update()
{
	auto currentTime = std::chrono::high_resolution_clock::now();

	// Ép kiểu thời gian chênh lệch ra số thực (giây)
	std::chrono::duration<float> delta = currentTime - s_LastFrameTime;
	s_DeltaTime = delta.count();

	std::chrono::duration<float> total = currentTime - s_StartTime;
	s_TotalTime = total.count();

	s_LastFrameTime = currentTime;
}