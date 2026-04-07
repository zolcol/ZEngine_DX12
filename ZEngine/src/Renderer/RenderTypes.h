#pragma once
#include <vector>
#include <d3d12.h>
#include <DirectXMath.h>

struct VertexData
{
	XMFLOAT3 position;
	XMFLOAT4 color;

	static std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputElementDesc()
	{
		return
		{
			// POSITION (3 float = 12 bytes)
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
			  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

			  // COLOR (4 float = 16 bytes), offset = 12 (sau POSITION)
			  { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};
	}
};

struct ConstantBufferData
{
	float colorMul;
};