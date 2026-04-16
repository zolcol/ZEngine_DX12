#include "pch.h"
#include "Application.h"

#include "Window.h"
#include <Renderer/Renderer.h>
#include "Scene.h"
#include "Time.h"

Application::Application() = default;

Application::~Application() = default;

void Application::Init()
{
	// Graphic
	m_Window = std::make_unique<Window>(WIDTH, HEIGHT, L"ZEngine");
	m_Window->Show();
	
	m_Renderer = std::make_unique<Renderer>();
	if (!m_Renderer->Init(m_Window->GetHWND(), WIDTH, HEIGHT, FRAME_COUNT))
	{
		ENGINE_FATAL("Failed to initialize Renderer!");
		return;
	}

	// Scene
	m_Scene = std::make_unique<Scene>(m_Renderer->GetModelManager());
	m_Scene->InitModel();
	// Time
	Time::Init();

	ENGINE_INFO("Application Initialized Successfully.");


}

void Application::Run()
{
	while (m_Window->ProcessMessages())
	{
		Time::Update();

		m_Scene->Update(Time::GetDeltaTime());

		m_Renderer->BeginFrame(m_Scene.get());
		m_Renderer->EndFrame(m_Scene.get());
	}
}

void Application::ShutDown()
{
	m_Renderer->ShutDown();
}
