#pragma once
#include <vector>
#include <d3d12.h>
#include <DirectXMath.h>

struct VertexData
{
	XMFLOAT3 position;
	XMFLOAT4 color;
	XMFLOAT2 uv;

	static std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputElementDesc()
	{
		return
		{
			// POSITION (3 float = 12 bytes)
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
			  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

			  // COLOR (4 float = 16 bytes), offset = 12 
			  { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

				// TEXCOORD (2 float = 8 bytes), offset = 28 (12 + 16)
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28,
				  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};
	}
};

struct ConstantBufferData
{
	XMFLOAT4X4 WorldMatrix;
	XMFLOAT4X4 ViewMatrix;
	XMFLOAT4X4 ProjectionMatrix;
};

