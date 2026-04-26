#include "pch.h"
#include "Application.h"

#include "Window.h"
#include <Renderer/Renderer.h>
#include "Scene.h"
#include "Time.h"
#include "Editor.h"
#include "CoreComponent.h"
#include "RegisterEnttMeta.h"
#include "Input.h"
#include "Renderer/Device.h"

Application::Application() = default;

Application::~Application() = default;

void Application::Init()
{
	// Graphic
	m_Window = std::make_unique<Window>(WIDTH, HEIGHT, L"ZEngine");
	m_Window->Show();
	
	// Lấy kích thước thực tế của Client Area sau khi Windows tạo cửa sổ
	// (Vì OS có thể bóp nhỏ cửa sổ lại nếu nó lớn hơn màn hình hoặc vướng Taskbar)
	RECT clientRect;
	GetClientRect(m_Window->GetHWND(), &clientRect);
	int actualWidth = clientRect.right - clientRect.left;
	int actualHeight = clientRect.bottom - clientRect.top;

	m_Renderer = std::make_unique<Renderer>();
	if (!m_Renderer->Init(m_Window->GetHWND(), actualWidth, actualHeight, FRAME_COUNT))
	{
		ENGINE_FATAL("Failed to initialize Renderer!");
		return;
	}

	Input::Init();

	m_Editor = std::make_unique<Editor>();
	m_Editor->Init(m_Window->GetHWND(), m_Renderer->GetDevice(), FRAME_COUNT);

	RegisterMetaData();

	// Scene
	m_Scene = std::make_unique<Scene>(m_Renderer.get());
	m_Scene->InitModel();
	m_Scene->InitEnvironment(m_Renderer->GetDevice()->GetDevice(), m_Renderer->GetCommandContext(), m_Renderer->GetDescriptorManager());

	// Time
	Time::Init();

	ENGINE_INFO("Application Initialized Successfully.");
}

void Application::Run()
{
	while (true)
	{
		Input::Update(); // 1. Reset Mouse Delta & Update Keys trạng thái của frame cũ

		if (!m_Window->ProcessMessages()) break; // 2. Collect Mouse Delta mới cho frame này

		Time::Update();

		m_Scene->Update(Time::GetDeltaTime());

		m_Editor->BeginFrame();
		m_Editor->Update(m_Scene.get(), Time::GetDeltaTime());

		m_Renderer->BeginFrame(m_Scene.get());
		m_Renderer->EndFrame(m_Scene.get(), m_Editor.get());
	}
}

void Application::ShutDown()
{
	m_Renderer->ShutDown();
	m_Editor->Shutdown();
}

