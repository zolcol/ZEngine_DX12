#pragma once
#include "DX12Utils.h"
#include <Core/Log.h>

void CalculateDirectionalLightMatrices(
	XMMATRIX& outLightView,
	XMMATRIX& outLightProj,
	const XMMATRIX& cameraViewProj,
	const XMFLOAT3& lightDirection,

	float splitNear,
	float splitFar,
	float camNear,
	float camFar,

	float shadowMapResolution, 
	float shadowDistanceOffset)
{

	// 1. Nghịch đảo ViewProj
	XMVECTOR det;
	XMMATRIX invViewProj = XMMatrixInverse(&det, cameraViewProj);

	// 2. Tính NDC Z (Reversed-Z)
	float splitNearNDC = (camNear * (camFar - splitNear)) / (splitNear * (camFar - camNear));
	float splitFarNDC = (camNear * (camFar - splitFar)) / (splitFar * (camFar - camNear));

	// 3. Lấy 8 đỉnh trong World Space
	XMFLOAT4 frustumCornersNDC[8] = {
		{ -1, -1, splitNearNDC, 1 }, {  1, -1, splitNearNDC, 1 },
		{ -1,  1, splitNearNDC, 1 }, {  1,  1, splitNearNDC, 1 },
		{ -1, -1, splitFarNDC, 1 },  {  1, -1, splitFarNDC, 1 },
		{ -1,  1, splitFarNDC, 1 },  {  1,  1, splitFarNDC, 1 }
	};

	XMVECTOR frustumCornersWS[8];
	XMVECTOR frustumCenter = XMVectorZero();

	for (int i = 0; i < 8; ++i)
	{
		XMVECTOR corner = XMLoadFloat4(&frustumCornersNDC[i]);
		corner = XMVector4Transform(corner, invViewProj);
		corner /= XMVectorGetW(corner);
		frustumCornersWS[i] = corner;
		frustumCenter += corner;
	}
	frustumCenter /= 8.0f;

	// --- BƯỚC MỚI: TÍNH BÁN KÍNH BAO QUANH (STABILITY) ---
	// Tính khoảng cách xa nhất từ tâm đến các đỉnh để cố định kích thước Box
	float sphereRadius = 0.0f;
	for (int i = 0; i < 8; ++i)
	{
		float dist = XMVectorGetX(XMVector3Length(frustumCornersWS[i] - frustumCenter));
		sphereRadius = (std::max)(sphereRadius, dist);
	}
	// Làm tròn bán kính để tránh sai số nhỏ
	sphereRadius = (std::ceil)(sphereRadius * 16.0f) / 16.0f;

	// 4. Thiết lập Light View
	XMVECTOR lightDir = XMVector3Normalize(XMLoadFloat3(&lightDirection));
	XMVECTOR lightPos = frustumCenter - lightDir * shadowDistanceOffset;
	XMVECTOR up = (fabs(XMVectorGetY(lightDir)) > 0.99f) ? XMVectorSet(0, 0, 1, 0) : XMVectorSet(0, 1, 0, 0);

	outLightView = XMMatrixLookAtLH(lightPos, frustumCenter, up);

	// 5. Xác định giới hạn Ortho dựa trên bán kính (Tạo thành hình vuông)
	float minX = -sphereRadius;
	float maxX = sphereRadius;
	float minY = -sphereRadius;
	float maxY = sphereRadius;

	// Tính toán Z tạm thời từ các đỉnh để bắt shadow caster
	float minZ = FLT_MAX, maxZ = -FLT_MAX;
	for (int i = 0; i < 8; ++i)
	{
		XMVECTOR v = XMVector4Transform(frustumCornersWS[i], outLightView);
		float z = XMVectorGetZ(v);
		minZ = (std::min)(minZ, z);
		maxZ = (std::max)(maxZ, z);
	}

	// --- BƯỚC MỚI: TEXEL SNAPPING ---
	float worldUnitsPerTexel = (sphereRadius * 2.0f) / shadowMapResolution;

	// Làm tròn minX, minY về bội số của kích thước 1 texel
	minX = (std::floor)(minX / worldUnitsPerTexel) * worldUnitsPerTexel;
	maxX = (std::floor)(maxX / worldUnitsPerTexel) * worldUnitsPerTexel;
	minY = (std::floor)(minY / worldUnitsPerTexel) * worldUnitsPerTexel;
	maxY = (std::floor)(maxY / worldUnitsPerTexel) * worldUnitsPerTexel;
	/*ENGINE_FATAL("sphereRadius = {}", sphereRadius);
	ENGINE_FATAL("worldUnitsPerTexel = {}", worldUnitsPerTexel);*/
	// 6. Ortho projection (Reversed-Z)
	minZ = 0.1f;
	// Lưu ý: minZ/maxZ cũng nên được mở rộng một chút để an toàn
	outLightProj = XMMatrixOrthographicOffCenterLH(
		minX, maxX,
		minY, maxY,
		maxZ, // Near
		minZ  // Far
	);
}