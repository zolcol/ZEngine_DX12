#pragma once

class Window;
class Renderer;
class Scene;

class Application
{
public:
	Application();
	~Application();

	void Init();
	void Run();
	void ShutDown();

private:
	const int WIDTH = 800;
	const int HEIGHT = 600;
	const int FRAME_COUNT = 3;

	std::unique_ptr<Window> m_Window;
	std::unique_ptr<Renderer> m_Renderer;
	std::unique_ptr<Scene> m_Scene;
};
