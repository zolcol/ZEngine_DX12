#include <system_error>

#include <DirectXMath.h>
#include <algorithm>
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


// Calculate view and projection matrices for directional light frustum fitting
inline void CalculateDirectionalLightMatrices(
	XMMATRIX& outLightView,
	XMMATRIX& outLightProj,
	const XMMATRIX& cameraViewProj,
	const XMFLOAT3& lightDirection,
	float shadowDistanceOffset = 200.0f)
{
	// 1. Get inverse camera view-projection matrix
	XMVECTOR det;
	XMMATRIX invViewProj = XMMatrixInverse(&det, cameraViewProj);

	// 2. Frustum corners in NDC space (DirectX: Z from 0 to 1)
	XMFLOAT4 frustumCornersNDC[8] = {
		{ -1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, -1.0f, 0.0f, 1.0f },
		{ -1.0f,  1.0f, 0.0f, 1.0f }, { 1.0f,  1.0f, 0.0f, 1.0f },
		{ -1.0f, -1.0f, 1.0f, 1.0f }, { 1.0f, -1.0f, 1.0f, 1.0f },
		{ -1.0f,  1.0f, 1.0f, 1.0f }, { 1.0f,  1.0f, 1.0f, 1.0f }
	};

	// 3. Transform to world space and calculate center
	XMVECTOR frustumCornersWorld[8];
	XMVECTOR frustumCenter = XMVectorZero();

	for (int i = 0; i < 8; ++i)
	{
		XMVECTOR corner = XMLoadFloat4(&frustumCornersNDC[i]);
		corner = XMVector4Transform(corner, invViewProj);

		// Perspective divide
		corner = XMVectorScale(corner, 1.0f / XMVectorGetW(corner));

		frustumCornersWorld[i] = corner;
		frustumCenter = XMVectorAdd(frustumCenter, corner);
	}

	// Average to get the center
	frustumCenter = XMVectorScale(frustumCenter, 1.0f / 8.0f);

	// 4. Calculate Light View Matrix
	XMVECTOR lightDir = XMLoadFloat3(&lightDirection);
	lightDir = XMVector3Normalize(lightDir);

	// Position light backwards from center to catch shadow casters outside view
	XMVECTOR lightPos = XMVectorSubtract(frustumCenter, XMVectorScale(lightDir, shadowDistanceOffset));

	// Default up vector
	XMVECTOR upDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	// Edge case: If light direction is perfectly vertical, adjust the up vector
	if (fabs(XMVectorGetY(lightDir)) > 0.999f)
	{
		upDir = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	}

	outLightView = XMMatrixLookAtLH(lightPos, frustumCenter, upDir);

	// 5. Transform world corners to light view space and find bounding box
	float minX = FLT_MAX, maxX = -FLT_MAX;
	float minY = FLT_MAX, maxY = -FLT_MAX;
	float minZ = FLT_MAX, maxZ = -FLT_MAX;

	for (int i = 0; i < 8; ++i)
	{
		XMVECTOR cornerLightSpace = XMVector4Transform(frustumCornersWorld[i], outLightView);
		XMFLOAT4 pt;
		XMStoreFloat4(&pt, cornerLightSpace);

		// Using extra parentheses to prevent macro conflicts with <windows.h> min/max
		minX = (std::min)(minX, pt.x);
		maxX = (std::max)(maxX, pt.x);
		minY = (std::min)(minY, pt.y);
		maxY = (std::max)(maxY, pt.y);
		minZ = (std::min)(minZ, pt.z);
		maxZ = (std::max)(maxZ, pt.z);
	}

	// 6. Calculate Light Projection Matrix
	// minZ and maxZ are used as Near and Far planes
	outLightProj = XMMatrixOrthographicOffCenterLH(minX, maxX, minY, maxY, minZ, maxZ);
}

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

