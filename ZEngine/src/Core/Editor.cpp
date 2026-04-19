#include "pch.h"
#include "Editor.h"
#include "Renderer/Device.h"
#include "Scene.h"
#include "CoreComponent.h"
#include "UIHelper.h"
#include "Input.h"

void Editor::Init(HWND hwnd, Device* device, int framesInFlight)
{
	m_CameraController = std::make_unique<EditorCameraController>();

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = 1;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	CHECK(device->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_ImGuiHeap)));

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX12_Init(
		device->GetDevice(),
		framesInFlight,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		m_ImGuiHeap.Get(),
		m_ImGuiHeap->GetCPUDescriptorHandleForHeapStart(),
		m_ImGuiHeap->GetGPUDescriptorHandleForHeapStart()
	);

	io.Fonts->Build();
}

void Editor::Shutdown()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void Editor::BeginFrame()
{
	// Khởi tạo ImGui frame mới ở đầu logic frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();

	// TẠO DOCKSPACE TOÀN MÀN HÌNH
	ImGuiDockNodeFlags dockSpaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;
	ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), dockSpaceFlags);
}

void Editor::Update(Scene* scene, float dt)
{
	m_CameraController->Update(scene, dt);

	// Phím tắt cho Gizmo: W (Dịch), E (Xoay), R (Thu phóng)
	if (!Input::IsMouseButtonPressed(1)) // Không đổi mode Gizmo nếu đang giữ chuột phải để lái Camera
	{
		if (Input::IsKeyPressed('W')) m_GizmoOperation = ImGuizmo::TRANSLATE;
		if (Input::IsKeyPressed('E')) m_GizmoOperation = ImGuizmo::ROTATE;
		if (Input::IsKeyPressed('R')) m_GizmoOperation = ImGuizmo::SCALE;
	}

	DrawSceneHierarchy(scene);
	DrawInspector(scene);
	DrawGizmo(scene);
}

void Editor::DrawGizmo(Scene* scene)
{
	if (m_SelectedEntity == entt::null) return;

	auto& registry = scene->GetRegistry();
	auto* transform = registry.try_get<TransformComponent>(m_SelectedEntity);
	if (!transform) return;

	// 1. Lấy ma trận View và Projection
	DirectX::XMMATRIX viewMatrix, projMatrix;
	bool hasCamera = false;
	float aspectRatio = ImGui::GetIO().DisplaySize.x / ImGui::GetIO().DisplaySize.y;

	registry.view<TransformComponent, CameraComponent>().each([&](auto& camTransform, auto& camera) {
		if (camera.IsPrimary && !hasCamera) {
			viewMatrix = camera.GetViewMatrix(camTransform);
			projMatrix = camera.GetProjectionMatrix(aspectRatio);
			hasCamera = true;
		}
	});

	if (!hasCamera) return;

	// 2. Cấu hình ImGuizmo để vẽ lên Foreground (đè lên toàn bộ màn hình)
	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
	
	// Đảm bảo vùng vẽ bao phủ toàn bộ cửa sổ ứng dụng
	float windowWidth = ImGui::GetIO().DisplaySize.x;
	float windowHeight = ImGui::GetIO().DisplaySize.y;
	ImGuizmo::SetRect(0, 0, windowWidth, windowHeight);

	// 3. Chuẩn bị ma trận
	float view[16], proj[16], model[16];
	DirectX::XMStoreFloat4x4((DirectX::XMFLOAT4X4*)view, viewMatrix);
	DirectX::XMMATRIX perspectiveProj = projMatrix;
	DirectX::XMStoreFloat4x4((DirectX::XMFLOAT4X4*)proj, perspectiveProj);

	// Lấy ma trận Model trực tiếp từ Transform
	DirectX::XMMATRIX modelMatrix = transform->GetWorldMatrix();
	DirectX::XMStoreFloat4x4((DirectX::XMFLOAT4X4*)model, modelMatrix);

	// 4. Vẽ Gizmo
	ImGuizmo::Manipulate(view, proj, (ImGuizmo::OPERATION)m_GizmoOperation, ImGuizmo::LOCAL, model);

	if (ImGuizmo::IsUsing())
	{
		DirectX::XMMATRIX manipulatedMatrix = DirectX::XMLoadFloat4x4((DirectX::XMFLOAT4X4*)model);
		DirectX::XMVECTOR scale, rotQuat, trans;
		DirectX::XMMatrixDecompose(&scale, &rotQuat, &trans, manipulatedMatrix);
		
		DirectX::XMStoreFloat3(&transform->Position, trans);
		DirectX::XMStoreFloat4(&transform->Rotation, rotQuat);
		DirectX::XMStoreFloat3(&transform->Scale, scale);
	}
}

void Editor::DrawSceneHierarchy(Scene* scene)
{
	ImGui::Begin("Scene Hierarchy");

	// Duyệt qua tất cả các Entity có TagComponent để hiển thị danh sách
	auto view = scene->GetRegistry().view<TagComponent>();
	for (auto entityID : view)
	{
		auto& tag = view.get<TagComponent>(entityID);
		bool isSelected = (m_SelectedEntity == entityID);

		// Hiển thị tên Entity dưới dạng Selectable
		if (ImGui::Selectable(tag.name.c_str(), isSelected))
		{
			m_SelectedEntity = entityID;
		}
	}

	// Bỏ chọn nếu click vào vùng trống trong Window
	if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
	{
		m_SelectedEntity = entt::null;
	}

	ImGui::End();
}

void Editor::DrawInspector(Scene* scene)
{
	ImGui::Begin("Inspector");

	if (m_SelectedEntity != entt::null)
	{
		auto& registry = scene->GetRegistry();

		// DUYỆT TỰ ĐỘNG CÁC COMPONENT (AUTOMATIC COMPONENT INSPECTION)
		// Chúng ta duyệt qua tất cả các Storage (kho lưu trữ) của Registry.
		// Mỗi Storage đại diện cho một loại Component.
		for (auto [id, storage] : registry.storage())
		{
			// Kiểm tra xem Entity hiện tại có sở hữu loại Component này không
			if (storage.contains(m_SelectedEntity))
			{
				// Sử dụng EnTT Meta để tìm thông tin kiểu dữ liệu (Reflection)
				auto meta_type = entt::resolve(id);
				if (meta_type)
				{
					// Chỉ xử lý những Component đã đăng ký hàm "Inspect"
					auto inspectFunc = meta_type.func("Inspect"_hs);
					if (inspectFunc)
					{
						// Lấy tên hiển thị từ Metadata custom (đã đăng ký trong RegisterMetaData)
						std::string componentName = "Unknown Component";
						if (EditorComponentInfo* info = meta_type.custom())
						{
							componentName = info->name;
						}

						// Vẽ tiêu đề Component (TreeNode)
						if (ImGui::TreeNodeEx(componentName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
						{
							// Lấy con trỏ thực tế tới Component và bọc nó trong meta_any (instance)
							auto instance = meta_type.from_void(storage.value(m_SelectedEntity));
							
							// Thực thi hàm Inspect() của Component đó
							inspectFunc.invoke(instance);

							ImGui::TreePop();
						}
					}
				}
			}
		}
	}
	else
	{
		ImGui::Text("Select an entity to view details.");
	}

	ImGui::End();
}

void Editor::Render(ID3D12GraphicsCommandList* cmdList)
{
	ImGui::Render();
	ID3D12DescriptorHeap* imguiHeaps[] = { m_ImGuiHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(imguiHeaps), imguiHeaps);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
}
