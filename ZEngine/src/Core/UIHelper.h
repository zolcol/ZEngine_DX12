#pragma once

class UIHelper
{
public:
	static void DrawVec3Control(const std::string& label, DirectX::XMFLOAT3& values, float resetValue = 0.0f, float columnWidth = 75.0f);
	static void DrawString(const std::string& label, std::string& value, float columnWidth = 75.0f);

	template <typename Component>
	static void DrawComponent(const std::string& componentName, Component& component)
	{
		if (ImGui::TreeNodeEx(componentName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			component.Inspect();
			ImGui::TreePop();
		}
	}
};
