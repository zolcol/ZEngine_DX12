#include "pch.h"
#include "Application.h"

#include "Window.h"
#include <Renderer/Device.h>

Application::Application() = default;

Application::~Application() = default;

void Application::Init()
{
	m_Window = std::make_unique<Window>(WIDTH, HEIGHT, L"ZEngine");
	m_Device = std::make_unique<Device>();
}

void Application::Run()
{
	
}

void Application::ShutDown()
{

}
