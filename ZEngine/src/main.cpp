#include "pch.h"

#include "Core/Window.h"
#include <Core/Application.h>

int main()
{
	Log::Init();

	Application app;
	app.Init();
	app.Run();
	app.ShutDown();

	return 0;
}	