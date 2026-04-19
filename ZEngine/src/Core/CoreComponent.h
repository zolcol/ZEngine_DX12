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
	DirectX::XMFLOAT3 Rotation{ 0, 0, 0 }; // Cần đảm bảo các giá trị này sử dụng đơn vị Radian
	DirectX::XMFLOAT3 Scale{ 1, 1, 1 };

	[[nodiscard]] DirectX::XMMATRIX GetWorldMatrix() const
	{
		using namespace DirectX;
		// 3. Ma trận Translation (Vị trí)
		XMMATRIX matTrans = XMMatrixTranslation(Position.x, Position.y, Position.z);
		// 1. Ma trận Scale
		XMMATRIX matScale = XMMatrixScaling(Scale.x, Scale.y, Scale.z);

		// 2. Ma trận Rotation 
		// Hàm này sử dụng thứ tự Pitch (trục X), Yaw (trục Y), Roll (trục Z)
		XMMATRIX matRot = XMMatrixRotationRollPitchYaw(Rotation.x, Rotation.y, Rotation.z);

		// 4. Nhân các ma trận theo thứ tự S * R * T
		return matScale * matRot * matTrans;
	}

	void Inspect()
	{
		Inspector::Property("Position", Position);
		auto rotDegree = DirectX::XMFLOAT3(
			DirectX::XMConvertToDegrees(Rotation.x), 
			DirectX::XMConvertToDegrees(Rotation.y),
			DirectX::XMConvertToDegrees(Rotation.z)
		);
		Inspector::Property("Rotation", rotDegree);
		Rotation = DirectX::XMFLOAT3(
			DirectX::XMConvertToRadians(rotDegree.x),
			DirectX::XMConvertToRadians(rotDegree.y),
			DirectX::XMConvertToRadians(rotDegree.z)
		);
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
		XMMATRIX matRot = XMMatrixRotationRollPitchYaw(transform.Rotation.x, transform.Rotation.y, transform.Rotation.z);

		XMMATRIX cameraWorld = matRot * matTrans;
		return XMMatrixInverse(nullptr, cameraWorld);
	}

	[[nodiscard]] DirectX::XMMATRIX GetProjectionMatrix(float aspectRatio) const
	{
		// Chuyển FOV từ độ sang Radian để dùng với XMMatrixPerspectiveFovLH
		return DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(FOV), aspectRatio, NearPlane, FarPlane);
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
	float Intensity = 1;

	// Point, Spot
	float Range = 10;

	// Spot
	float InnerAngle = 15.0f;
	float OuterAngle = 30.0f;

	bool CastShadow = false;
	
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
	XMFLOAT2 Padding;

	GPULightData(const LightComponent& light, const TransformComponent& transform)
	{
		Color = light.Color;
		Intensity = light.Intensity;

		Position = transform.Position;
		Range = light.Range;
		Type = (int)light.Type;

		// 👉 Tính direction từ Euler rotation (radian)
		XMMATRIX rotMatrix = XMMatrixRotationRollPitchYaw(
			transform.Rotation.x,
			transform.Rotation.y,
			transform.Rotation.z
		);

		// Forward mặc định là (0,0,1)
		XMVECTOR forward = XMVector3TransformNormal(
			XMVectorSet(0, 0, 1, 0),
			rotMatrix
		);

		forward = XMVector3Normalize(forward);
		XMStoreFloat3(&Direction, forward);

		// 👉 Spot light: dùng cos(angle)
		InnerAngle = cosf(light.InnerAngle);
		OuterAngle = cosf(light.OuterAngle);

		Padding = { 0.0f, 0.0f };
	}
};