#include "pch.h"
#include "Inspector.h"
#include "UIHelper.h"

void Inspector::Property(const char* PropName, DirectX::XMFLOAT3& value, float resetValue /*= 0.0f*/)
{
	UIHelper::DrawVec3Control(PropName, value, resetValue);
}

void Inspector::Property(const char* PropName, std::string& value)
{
	UIHelper::DrawString(PropName, value);
}

void Inspector::Property(const char* PropName, float& value, float speed, float min, float max)
{
	UIHelper::DrawFloat(PropName, value, speed, min, max);
}

void Inspector::Property(const char* PropName, int& value, float speed, int min, int max)
{
	UIHelper::DrawInt(PropName, value, speed, min, max);
}

void Inspector::Property(const char* PropName, bool& value)
{
	UIHelper::DrawBool(PropName, value);
}

void Inspector::Property(const char* PropName, const char** options, int optionCount, int& selectedIndex)
{
	UIHelper::DrawDropdown(PropName, options, optionCount, selectedIndex);
}

void Inspector::ColorProperty(const char* PropName, DirectX::XMFLOAT3& value)
{
	UIHelper::DrawColorControl(PropName, value);
}
