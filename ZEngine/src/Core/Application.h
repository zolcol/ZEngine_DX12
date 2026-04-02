#pragma once

class Window;
class Device;

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
	const int FRAME_IN_FLIGHT = 2;

	std::unique_ptr<Window> m_Window;
	std::unique_ptr<Device> m_Device;
};
