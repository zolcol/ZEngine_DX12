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
