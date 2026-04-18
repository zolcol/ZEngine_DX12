#pragma once

class Inspector
{
public:
	static void Property(const char* PropName, DirectX::XMFLOAT3& value, float resetValue = 0.0f);
	static void Property(const char* PropName, std::string& value);
	static void Property(const char* PropName, float& value, float speed = 0.1f, float min = 0.0f, float max = 0.0f);
	static void Property(const char* PropName, int& value, float speed = 1.0f, int min = 0, int max = 0);
	static void Property(const char* PropName, bool& value);
private:

};