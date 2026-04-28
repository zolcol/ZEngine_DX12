#include <system_error>
#include <DirectXMath.h>
#include <algorithm>
#include <d3d12.h>
#include <iostream>
using namespace DirectX;

#define CHECK(x) \
    do { \
        HRESULT hr__ = (x); \
        if (FAILED(hr__)) { \
            /* Dùng system_category để dịch mã lỗi C++ chuẩn */ \
            std::string errMsg = HRToString(hr__); \
            ENGINE_FATAL("DirectX 12 Error!"); \
            ENGINE_FATAL("Function: {}", #x); \
            ENGINE_FATAL("Code: 0x{:08X} - {}", (unsigned int)hr__, errMsg); \
            __debugbreak(); \
        } \
    } while(0)

// Hàm chuyển đổi wchar_t* sang char* (UTF-8)
inline std::string WStringToString(const std::wstring & wstr)
{
	if (wstr.empty()) return std::string();

	// Tính toán kích thước cần thiết
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);

	// Đổ dữ liệu sang chuỗi std::string thông thường
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

// Hàm chuyển đổi char* (UTF-8) sang wchar_t*
inline std::wstring StringToWString(const std::string& str)
{
	if (str.empty()) return std::wstring();

	// Tính toán kích thước cần thiết
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);

	// Đổ dữ liệu sang chuỗi std::wstring
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

// Hàm chuyển đổi HR sang tiếng người
inline std::string HRToString(const HRESULT& hr)
{
    return std::system_category().message(hr);
}

void CalculateDirectionalLightMatrices(XMMATRIX& outLightView, XMMATRIX& outLightProj, const XMMATRIX& cameraViewProj, const XMFLOAT3& lightDirection, float splitNear, float splitFar, float camNear, float camFar, float shadowMapResolution, float shadowDistanceOffset = 200);

// Calculate forward direction from a quaternion rotation
inline DirectX::XMFLOAT3 GetDirectionFromRotation(const DirectX::XMFLOAT4& rotation)
{
	DirectX::XMVECTOR rotQuat = DirectX::XMLoadFloat4(&rotation);
	DirectX::XMMATRIX rotMatrix = DirectX::XMMatrixRotationQuaternion(rotQuat);
	// Forward vector in Left-Handed coordinate system is (0, 0, 1)
	DirectX::XMVECTOR forward = DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotMatrix);
	forward = DirectX::XMVector3Normalize(forward);
	
	DirectX::XMFLOAT3 direction;
	DirectX::XMStoreFloat3(&direction, forward);
	return direction;
}

