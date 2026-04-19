#include "pch.h"
#include "UIHelper.h"

void UIHelper::DrawVec3Control(const std::string& label, DirectX::XMFLOAT3& values, float resetValue, float columnWidth)
{
	ImGui::PushID(label.c_str());

	if (ImGui::BeginTable("##vec3_table", 2, ImGuiTableFlags_None))
	{
		ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, columnWidth);
		ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		// Căn giữa label
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", label.c_str());

		ImGui::TableSetColumnIndex(1);

		// [ĐÃ SỬA] Dùng public API lấy Font Size
		float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		// [ĐÃ SỬA] Tự tính toán độ rộng chia đều cho 3 ô DragFloat
		float spacingBetweenGroups = 4.0f; // Khoảng cách nhỏ giữa nhóm X, Y, Z
		float totalWidth = ImGui::GetContentRegionAvail().x;
		// Trừ đi kích thước 3 nút và 2 khoảng cách ở giữa, sau đó chia 3
		float itemWidth = (totalWidth - (buttonSize.x * 3.0f) - (spacingBetweenGroups * 2.0f)) / 3.0f;

		// Xóa khoảng cách mặc định để Nút và Ô input dính liền vào nhau
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		// --- TRỤC X ---
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::Button("X", buttonSize);
		if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
			values.x = resetValue;
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::PushItemWidth(itemWidth);
		ImGui::DragFloat("##X", &values.x, 0.01f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::SameLine(0, spacingBetweenGroups); // Phục hồi 1 chút khoảng cách trước khi vẽ Y

		// --- TRỤC Y ---
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::Button("Y", buttonSize);
		if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
			values.y = resetValue;
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::PushItemWidth(itemWidth);
		ImGui::DragFloat("##Y", &values.y, 0.01f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::SameLine(0, spacingBetweenGroups);

		// --- TRỤC Z ---
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::Button("Z", buttonSize);
		if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
			values.z = resetValue;
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::PushItemWidth(itemWidth);
		ImGui::DragFloat("##Z", &values.z, 0.01f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar(); // Pop cái ItemSpacing = 0
		ImGui::EndTable();
	}

	ImGui::PopID();

	ImGui::Separator();

}

void UIHelper::DrawString(const std::string& label, std::string& value, float columnWidth)
{
	ImGui::PushID(label.c_str());

	if (ImGui::BeginTable("##string_table", 2, ImGuiTableFlags_None))
	{
		ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, columnWidth);
		ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", label.c_str());

		ImGui::TableSetColumnIndex(1);

		char buffer[256];
		memset(buffer, 0, sizeof(buffer));
		strcpy_s(buffer, sizeof(buffer), value.c_str());

		ImGui::PushItemWidth(-FLT_MIN); // Chiếm toàn bộ chiều rộng còn lại của cột
		if (ImGui::InputText("##string_input", buffer, sizeof(buffer)))
		{
			value = std::string(buffer);
		}
		ImGui::PopItemWidth();

		ImGui::EndTable();
	}

	ImGui::PopID();

	ImGui::Separator();

}

void UIHelper::DrawFloat(const std::string& label, float& value, float speed, float min, float max, float columnWidth)
{
	ImGui::PushID(label.c_str());

	if (ImGui::BeginTable("##float_table", 2, ImGuiTableFlags_None))
	{
		ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, columnWidth);
		ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", label.c_str());

		ImGui::TableSetColumnIndex(1);

		ImGui::PushItemWidth(-FLT_MIN);
		ImGui::DragFloat("##value", &value, speed, min, max, "%.2f");
		ImGui::PopItemWidth();

		ImGui::EndTable();
	}

	ImGui::PopID();
	ImGui::Separator();
}

void UIHelper::DrawInt(const std::string& label, int& value, float speed, int min, int max, float columnWidth)
{
	ImGui::PushID(label.c_str());

	if (ImGui::BeginTable("##int_table", 2, ImGuiTableFlags_None))
	{
		ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, columnWidth);
		ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", label.c_str());

		ImGui::TableSetColumnIndex(1);

		ImGui::PushItemWidth(-FLT_MIN);
		ImGui::DragInt("##value", &value, speed, min, max);
		ImGui::PopItemWidth();

		ImGui::EndTable();
	}

	ImGui::PopID();
	ImGui::Separator();
}

void UIHelper::DrawBool(const std::string& label, bool& value, float columnWidth)
{
	ImGui::PushID(label.c_str());

	if (ImGui::BeginTable("##bool_table", 2, ImGuiTableFlags_None))
	{
		ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, columnWidth);
		ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", label.c_str());

		ImGui::TableSetColumnIndex(1);

		ImGui::Checkbox("##value", &value);

		ImGui::EndTable();
	}

	ImGui::PopID();
	ImGui::Separator();
}

void UIHelper::DrawDropdown(const std::string& label, const char** options, int optionCount, int& selectedIndex, float columnWidth)
{
	ImGui::PushID(label.c_str());

	if (ImGui::BeginTable("##dropdown_table", 2, ImGuiTableFlags_None))
	{
		ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, columnWidth);
		ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", label.c_str());

		ImGui::TableSetColumnIndex(1);

		ImGui::PushItemWidth(-FLT_MIN);
		const char* currentItem = options[selectedIndex];
		if (ImGui::BeginCombo("##combo", currentItem))
		{
			for (int i = 0; i < optionCount; i++)
			{
				bool isSelected = (currentItem == options[i]);
				if (ImGui::Selectable(options[i], isSelected))
				{
					selectedIndex = i;
				}
				if (isSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();

		ImGui::EndTable();
	}

	ImGui::PopID();
	ImGui::Separator();
}

void UIHelper::DrawColorControl(const std::string& label, DirectX::XMFLOAT3& value, float columnWidth)
{
	ImGui::PushID(label.c_str());

	if (ImGui::BeginTable("##color_table", 2, ImGuiTableFlags_None))
	{
		ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, columnWidth);
		ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", label.c_str());

		ImGui::TableSetColumnIndex(1);

		ImGui::PushItemWidth(-FLT_MIN);
		ImGui::ColorEdit3("##color", &value.x, ImGuiColorEditFlags_Float);
		ImGui::PopItemWidth();

		ImGui::EndTable();
	}

	ImGui::PopID();
	ImGui::Separator();
}
