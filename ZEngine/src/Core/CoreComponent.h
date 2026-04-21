#pragma once
#include <string>
#include <DirectXMath.h>
#include "Model.h"
#include "Inspector.h"

struct TagComponent
{
	std::string name;

	void Inspect()
	{
		Inspector::Property("Name", name);
	}
};

struct TransformComponent
{
	DirectX::XMFLOAT3 Position{ 0, 0, 0 };
	DirectX::XMFLOAT4 Rotation{ 0, 0, 0, 1 }; // Quaternion (X, Y, Z, W)
	DirectX::XMFLOAT3 Scale{ 1, 1, 1 };

	[[nodiscard]] DirectX::XMMATRIX GetRotationMatrix() const
	{
		return DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&Rotation));
	}

	[[nodiscard]] DirectX::XMFLOAT3 GetEulerAnglesRadians() const
	{
		using namespace DirectX;
		XMMATRIX mat = GetRotationMatrix();
		XMFLOAT4X4 m;
		XMStoreFloat4x4(&m, mat);

		float x, y, z;
		if (m._32 < 1.0f)
		{
			if (m._32 > -1.0f)
			{
				x = asin(-m._32);
				y = atan2(m._31, m._33);
				z = atan2(m._12, m._22);
			}
			else
			{
				x = XM_PIDIV2;
				y = -atan2(-m._13, m._11);
				z = 0.0f;
			}
		}
		else
		{
			x = -XM_PIDIV2;
			y = atan2(-m._13, m._11);
			z = 0.0f;
		}
		return { x, y, z };
	}

	[[nodiscard]] DirectX::XMFLOAT3 GetEulerAnglesDegrees() const
	{
		using namespace DirectX;
		auto rad = GetEulerAnglesRadians();
		return { XMConvertToDegrees(rad.x), XMConvertToDegrees(rad.y), XMConvertToDegrees(rad.z) };
	}

	void SetEulerAnglesRadians(const DirectX::XMFLOAT3& euler)
	{
		using namespace DirectX;
		XMVECTOR q = XMQuaternionRotationRollPitchYaw(euler.x, euler.y, euler.z);
		XMStoreFloat4(&Rotation, q);
	}

	void SetEulerAnglesDegrees(const DirectX::XMFLOAT3& euler)
	{
		using namespace DirectX;
		SetEulerAnglesRadians({ XMConvertToRadians(euler.x), XMConvertToRadians(euler.y), XMConvertToRadians(euler.z) });
	}

	void Rotate(const DirectX::XMFLOAT3& axis, float angleDegree)
	{
		using namespace DirectX;
		XMVECTOR currentRot = XMLoadFloat4(&Rotation);
		XMVECTOR axisVec = XMLoadFloat3(&axis);
		XMVECTOR deltaRot = XMQuaternionRotationAxis(axisVec, XMConvertToRadians(angleDegree));
		XMVECTOR newRot = XMQuaternionMultiply(currentRot, deltaRot);
		XMStoreFloat4(&Rotation, newRot);
	}

	[[nodiscard]] DirectX::XMMATRIX GetWorldMatrix() const
	{
		using namespace DirectX;
		// 1. Ma trận Scale
		XMMATRIX matScale = XMMatrixScaling(Scale.x, Scale.y, Scale.z);

		// 2. Ma trận Rotation
		XMMATRIX matRot = GetRotationMatrix();

		// 3. Ma trận Translation (Vị trí)
		XMMATRIX matTrans = XMMatrixTranslation(Position.x, Position.y, Position.z);

		// 4. Nhân các ma trận theo thứ tự S * R * T
		return matScale * matRot * matTrans;
	}

	void Inspect()
	{
		Inspector::Property("Position", Position);
		
		auto rotDegree = GetEulerAnglesDegrees();
		Inspector::Property("Rotation", rotDegree);
		SetEulerAnglesDegrees(rotDegree);
		
		Inspector::Property("Scale", Scale);
	}
};


struct CameraComponent
{
	float FOV = 45.0f;
	float NearPlane = 0.1f;
	float FarPlane = 1000.0f;

	bool IsPrimary = true;

	[[nodiscard]] DirectX::XMMATRIX GetViewMatrix(const TransformComponent& transform) const
	{
		using namespace DirectX;
		// Ma trận View là nghịch đảo của ma trận Camera Transform (không tính Scale)
		XMMATRIX matTrans = XMMatrixTranslation(transform.Position.x, transform.Position.y, transform.Position.z);
		XMMATRIX matRot = transform.GetRotationMatrix();

		XMMATRIX cameraWorld = matRot * matTrans;
		return XMMatrixInverse(nullptr, cameraWorld);
	}

	[[nodiscard]] DirectX::XMMATRIX GetProjectionMatrix(float aspectRatio) const
	{
		// Reverse-Z Perspective Projection
		// Standard: m33 = f/(f-n), m43 = -fn/(f-n)
		// Reverse:  m33 = n/(n-f), m43 = -fn/(n-f) = fn/(f-n)
		
		DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(FOV), aspectRatio, NearPlane, FarPlane);
		
		proj.r[2].m128_f32[2] = NearPlane / (NearPlane - FarPlane);
		proj.r[3].m128_f32[2] = (FarPlane * NearPlane) / (FarPlane - NearPlane);
		
		return proj;
	}

	void Inspect()
	{
		Inspector::Property("FOV", FOV, 0.1f, 1.0f, 120.0f);
		Inspector::Property("Near Plane", NearPlane, 0.01f, 0.001f, 10.0f);
		Inspector::Property("Far Plane", FarPlane, 1.0f, 10.0f, 10000.0f);

		Inspector::Property("Is Primary", IsPrimary);
	}
};


enum class LightType : int
{
	Directional = 0,
	Point		= 1,
	Spot		= 2
};

struct LightComponent
{
	LightType Type = LightType::Directional;
	XMFLOAT3 Color = { 1, 1, 1 };
	float Intensity = 5;

	// Point, Spot
	float Range = 10;

	// Spot
	float InnerAngle = 15.0f;
	float OuterAngle = 30.0f;

	bool CastShadow = false;
	int shadowMapIndex = -1;

	[[nodiscard]] DirectX::XMFLOAT4X4 GetViewMatrix(const TransformComponent& transform) const
	{
		using namespace DirectX;
		// Hướng đèn là Forward của transform. Ma trận View là nghịch đảo của ma trận World đèn.
		XMMATRIX matTrans = XMMatrixTranslation(transform.Position.x, transform.Position.y, transform.Position.z);
		XMMATRIX matRot = transform.GetRotationMatrix();

		XMMATRIX lightWorld = matRot * matTrans;
		XMMATRIX view = XMMatrixInverse(nullptr, lightWorld);

		XMFLOAT4X4 res;
		XMStoreFloat4x4(&res, view);
		return res;
	}

	[[nodiscard]] DirectX::XMFLOAT4X4 GetProjectionMatrix() const
	{
		using namespace DirectX;
		XMMATRIX proj;
		if (Type == LightType::Directional)
		{
			// Reverse Z for Orthographic
			// Standard: m33 = 1/(f-n), m43 = -n/(f-n)
			// Reverse: m33 = -1/(f-n), m43 = f/(f-n)
			float nearP = 0.1f;
			float farP = 100.0f;
			proj = XMMatrixOrthographicLH(10.0f, 10.0f, nearP, farP);
			proj.r[2].m128_f32[2] = -1.0f / (farP - nearP);
			proj.r[3].m128_f32[2] = farP / (farP - nearP);
		}
		else if (Type == LightType::Spot)
		{
			float nearP = 0.1f;
			float farP = Range;
			proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(OuterAngle * 2.0f), 1.0f, nearP, farP);
			proj.r[2].m128_f32[2] = nearP / (nearP - farP);
			proj.r[3].m128_f32[2] = -(farP * nearP) / (nearP - farP);
		}
		else
		{
			proj = XMMatrixIdentity();
		}

		XMFLOAT4X4 res;
		XMStoreFloat4x4(&res, proj);
		return res;
	}
	
	void Inspect()
	{
		const char* lightTypeOptions[] = { "Directional", "Point", "Spot" };
		int selectedIndex = (int)Type;
		Inspector::Property("Light Type", lightTypeOptions, 3, selectedIndex);
		Type = (LightType)selectedIndex;

		Inspector::ColorProperty("Color", Color);
		Inspector::Property("Intensity", Intensity, 0.1f, 0.0f, 1000.0f);

		if (Type != LightType::Directional)
		{
			Inspector::Property("Range", Range, 0.1f, 0.0f, 1000.0f);
		}

		if (Type == LightType::Spot)
		{
			Inspector::Property("Inner Angle", InnerAngle, 0.1f, 0.0f, 89.0f);
			Inspector::Property("Outer Angle", OuterAngle, 0.1f, 0.0f, 90.0f);
		}

		Inspector::Property("Cast Shadow", CastShadow);
	}
};

struct GPULightData
{
	XMFLOAT3 Color;
	float    Intensity;
	XMFLOAT3 Position;
	float    Range;
	XMFLOAT3 Direction;
	int      Type; // 0: Dir, 1: Point, 2: Spot                                                             
	float    InnerAngle;
	float    OuterAngle;
	
	int		 ShadowMapIndex;
	float	 padding;
	XMFLOAT4X4	lightViewProj;

	GPULightData() = default;

	GPULightData(const LightComponent& light, const TransformComponent& transform, int shadowMapIndex = -1)
	{
		Color = light.Color;
		Intensity = light.Intensity;

		Position = transform.Position;
		Range = light.Range;
		Type = (int)light.Type;

		// 👉 Tính direction từ quaternion rotation
		XMMATRIX rotMatrix = transform.GetRotationMatrix();

		// Forward mặc định là (0,0,1)
		XMVECTOR forward = XMVector3TransformNormal(
			XMVectorSet(0, 0, 1, 0),
			rotMatrix
		);

		forward = XMVector3Normalize(forward);
		XMStoreFloat3(&Direction, forward);

		// 👉 Spot light: dùng cos(angle)
		InnerAngle = cosf(DirectX::XMConvertToRadians(light.InnerAngle));
		OuterAngle = cosf(DirectX::XMConvertToRadians(light.OuterAngle));

		// Shadow
		ShadowMapIndex = shadowMapIndex;

		// Calculate Light View Projection Matrix
		XMFLOAT4X4 viewF = light.GetViewMatrix(transform);
		XMFLOAT4X4 projF = light.GetProjectionMatrix();
		XMMATRIX view = XMLoadFloat4x4(&viewF);
		XMMATRIX proj = XMLoadFloat4x4(&projF);
		
		XMStoreFloat4x4(&lightViewProj, XMMatrixTranspose(view * proj));
	}
};