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
	XMVECTOR frustumCenterWS = XMVectorZero();
	for (int i = 0; i < 8; ++i)
	{
		XMVECTOR corner = XMLoadFloat4(&frustumCornersNDC[i]);
		corner = XMVector4Transform(corner, invViewProj);
		corner /= XMVectorGetW(corner);
		frustumCornersWS[i] = corner;
		frustumCenterWS += corner;
	}
	frustumCenterWS /= 8.0f;

	// 4. Thiết lập hướng đèn và View Matrix chính thức
	XMVECTOR lightDir = XMVector3Normalize(XMLoadFloat3(&lightDirection));
	XMVECTOR up = (fabs(XMVectorGetY(lightDir)) > 0.99f) ? XMVectorSet(0, 0, 1, 0) : XMVectorSet(0, 1, 0, 0);

	// Đẩy đèn ra xa để bắt được các vật thể đứng sau camera nhưng đổ bóng vào vùng nhìn
	XMVECTOR lightPos = frustumCenterWS - lightDir * shadowDistanceOffset;
	outLightView = XMMatrixLookAtLH(lightPos, frustumCenterWS, up);

	// 5. Tính toán giới hạn (AABB) của Frustum trong không gian của Đèn (Light Space)
	float minX = FLT_MAX, maxX = -FLT_MAX;
	float minY = FLT_MAX, maxY = -FLT_MAX;
	float minZ = FLT_MAX, maxZ = -FLT_MAX;

	for (int i = 0; i < 8; ++i)
	{
		XMVECTOR cornerLS = XMVector4Transform(frustumCornersWS[i], outLightView);
		XMFLOAT3 c;
		XMStoreFloat3(&c, cornerLS);

		minX = (std::min)(minX, c.x);
		maxX = (std::max)(maxX, c.x);
		minY = (std::min)(minY, c.y);
		maxY = (std::max)(maxY, c.y);
		minZ = (std::min)(minZ, c.z);
		maxZ = (std::max)(maxZ, c.z);
	}

	// --- TEXEL SNAPPING CHO TIGHT AABB ---
	float worldUnitsPerTexelX = (maxX - minX) / shadowMapResolution;
	float worldUnitsPerTexelY = (maxY - minY) / shadowMapResolution;

	minX = floor(minX / worldUnitsPerTexelX) * worldUnitsPerTexelX;
	maxX = ceil(maxX / worldUnitsPerTexelX) * worldUnitsPerTexelX;
	minY = floor(minY / worldUnitsPerTexelY) * worldUnitsPerTexelY;
	maxY = ceil(maxY / worldUnitsPerTexelY) * worldUnitsPerTexelY;

	// 6. Tạo ma trận Ortho (Reversed-Z: maxZ -> Near(1), minZ -> Far(0))
	outLightProj = XMMatrixOrthographicOffCenterLH(
		minX, maxX,
		minY, maxY,
		maxZ + shadowDistanceOffset, // Near (nới rộng để bắt shadow caster)
		0.1f						 // Far
	);
}
