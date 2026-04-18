#pragma once

class Window;
class Renderer;
class Scene;
class Editor;

class Application
{
public:
	Application();
	~Application();

	void Init();
	void Run();
	void ShutDown();

private:
	const int WIDTH = 1200;
	const int HEIGHT = 900;
	const int FRAME_COUNT = 3;

	std::unique_ptr<Window> m_Window;
	std::unique_ptr<Renderer> m_Renderer;
	std::unique_ptr<Editor>	m_Editor;
	std::unique_ptr<Scene> m_Scene;

};
