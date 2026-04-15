#include "pch.h"

#include "Core/Window.h"
#include <Core/Application.h>

// Agility SDK Exports
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 619; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

int main()
{
	Log::Init();

	Application app;
	app.Init();
	app.Run();
	app.ShutDown();

	return 0;
}	