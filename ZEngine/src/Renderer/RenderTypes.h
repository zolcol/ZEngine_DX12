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

struct ConstantBufferData
{
	XMFLOAT4X4 WorldMatrix;
	XMFLOAT4X4 ViewMatrix;
	XMFLOAT4X4 ProjectionMatrix;
};

