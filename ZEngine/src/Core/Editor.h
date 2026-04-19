#pragma once
#include "EditorCameraController.h"

class Device;
class Scene;

struct EditorComponentInfo
{
	const char* name;
};

class Editor
{
public:
	Editor() = default;
	~Editor() = default;


	void Init(HWND hwnd, Device* device, int framesInFlight);
	void Shutdown();

	void BeginFrame();
	void Update(Scene* scene, float dt);
	void Render(ID3D12GraphicsCommandList* cmdList);
private:
	void DrawSceneHierarchy(Scene* scene);
	void DrawInspector(Scene* scene);
	void DrawGizmo(Scene* scene);

	ComPtr<ID3D12DescriptorHeap> m_ImGuiHeap;

	entt::entity m_SelectedEntity = entt::null;
	int m_GizmoOperation = 7; // Mặc định là TRANSLATE (thường là 7)

	std::unique_ptr<EditorCameraController> m_CameraController;
};
