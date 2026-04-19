#pragma once

class UIHelper
{
public:
	static void DrawVec3Control(const std::string& label, DirectX::XMFLOAT3& values, float resetValue = 0.0f, float columnWidth = 75.0f);
	static void DrawString(const std::string& label, std::string& value, float columnWidth = 75.0f);
	static void DrawFloat(const std::string& label, float& value, float speed = 0.1f, float min = 0.0f, float max = 0.0f, float columnWidth = 75.0f);
	static void DrawInt(const std::string& label, int& value, float speed = 1.0f, int min = 0, int max = 0, float columnWidth = 75.0f);
	static void DrawBool(const std::string& label, bool& value, float columnWidth = 75.0f);
	static void DrawDropdown(const std::string& label, const char** options, int optionCount, int& selectedIndex, float columnWidth = 75.0f);
	static void DrawColorControl(const std::string& label, DirectX::XMFLOAT3& value, float columnWidth = 75.0f);
	

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
