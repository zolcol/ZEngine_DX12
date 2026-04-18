#pragma once

class Inspector
{
public:
	static void Property(const char* PropName, DirectX::XMFLOAT3& value, float resetValue = 0.0f);

	static void Property(const char* PropName, std::string& value);
private:

};