#include "pch.h"
#include "Application.h"

#include "Window.h"
#include <Renderer/Renderer.h>
#include "Time.h"

Application::Application() = default;

Application::~Application() = default;

void Application::Init()
{
	m_Window = std::make_unique<Window>(WIDTH, HEIGHT, L"ZEngine");
	m_Window->Show();
	
	m_Renderer = std::make_unique<Renderer>();
	if (!m_Renderer->Init(m_Window->GetHWND(), WIDTH, HEIGHT, FRAME_COUNT))
	{
		ENGINE_FATAL("Failed to initialize Renderer!");
		return;
	}

	Time::Init();

	ENGINE_INFO("Application Initialized Successfully.");
}

void Application::Run()
{
	while (m_Window->ProcessMessages())
	{
		Time::Update();

		m_Renderer->BeginFrame();
		m_Renderer->EndFrame();
	}
}

void Application::ShutDown()
{
	m_Renderer->ShutDown();
}
