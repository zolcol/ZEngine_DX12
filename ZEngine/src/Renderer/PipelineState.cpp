#include "pch.h"
#include "PipelineState.h"

#include "RootSignature.h"
#include "Shader.h"

bool PipelineState::Init(ID3D12Device* device, const RootSignature& rootSignature, const Shader& vs, const Shader& ps)
{
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		// POSITION (3 float = 12 bytes)
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
		  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

		  // COLOR (4 float = 16 bytes), offset = 12 (sau POSITION)
		  { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// 2. Thiết lập cấu hình PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = rootSignature.Get();

	// Gắn Shader vào
	psoDesc.VS = { vs.GetBufferPointer(), vs.GetBufferSize() };
	psoDesc.PS = { ps.GetBufferPointer(), ps.GetBufferSize() };

	// Các trạng thái mặc định (Dùng helper từ d3dx12.h sẽ nhanh hơn)
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = FALSE; // Tạm thời tắt vì chưa có Depth Buffer
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// Định dạng đầu ra (Phải khớp với SwapChain)
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;

	CHECK(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_Pso)));

	return true;
}
