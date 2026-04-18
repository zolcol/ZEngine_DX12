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
