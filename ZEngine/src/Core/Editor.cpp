#include "pch.h"
#include "Editor.h"
#include "Renderer/Device.h"
#include "Scene.h"
#include "CoreComponent.h"
#include "UIHelper.h"

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

	// TẠO DOCKSPACE TOÀN MÀN HÌNH
	ImGuiDockNodeFlags dockSpaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;
	ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), dockSpaceFlags);
}

void Editor::Update(Scene* scene, float dt)
{
	m_CameraController->Update(scene, dt);

	DrawSceneHierarchy(scene);
	DrawInspector(scene);
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
