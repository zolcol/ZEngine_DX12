#pragma once
#include <vector>
#include <d3d12.h>
#include <DirectXMath.h>

struct VertexData
{
	XMFLOAT3 position;   // 12 bytes
	XMFLOAT3 normal;     // 12 bytes
	XMFLOAT2 uv;         // 8 bytes
	XMFLOAT4 tangent;    // 16 bytes

	static std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputElementDesc()
	{
		return
		{
			// POSITION (offset = 0)
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
			  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

			  // NORMAL (offset = 12)
			  { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

				// TEXCOORD (offset = 24)
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24,
				  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

				  // TANGENT (offset = 32)
				  { "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32,
					D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};
	}
};

struct alignas(16) ConstantBufferData
{
	XMFLOAT4X4 ViewMatrix;
	XMFLOAT4X4 ProjectionMatrix;
	XMFLOAT3   CameraPos;
	float padding;
};

struct alignas(16) ShadowConstantBufferData
{
	XMFLOAT4X4 LightViewMatrix;
	XMFLOAT4X4 LightProjectionMatrix;
	XMFLOAT3   PaddingPos;
	float padding;
};



struct ObjectData
{
	XMMATRIX WorldTransform;
};

// ==========================================
// ROOT SIGNATURE SLOTS
// Định nghĩa cố định vị trí các tham số trong Root Signature
// ==========================================
enum RootSlot
{
	Slot_RootConstants = 0, // b0, space1 (MaterialID, ObjectIndex, LightCount)
	
	// Root Descriptors (CBV/SRV/UAV trực tiếp - Tốc độ cao)
	Slot_GlobalCBV     = 1, // b0, space2 (Camera, Time)
	Slot_ShadowCBV     = 2, // b1, space2 (Light View/Proj)
	Slot_MaterialSRV   = 3, // t0, space2 (StructuredBuffer Materials)
	Slot_ObjectSRV     = 4, // t1, space2 (StructuredBuffer ObjectData)
	Slot_LightSRV      = 5, // t2, space2 (StructuredBuffer Lights)

	// Descriptor Tables (Bindless - Cho số lượng lớn)
	Slot_CBVTable      = 6, // space0 (Chưa dùng nhiều)
	Slot_SRVTable      = 7, // space0 (Textures bindless)
	Slot_UAVTable      = 8, // space0
	
	Count
};