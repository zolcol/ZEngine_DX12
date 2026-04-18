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
