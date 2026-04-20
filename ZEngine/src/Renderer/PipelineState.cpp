#include "pch.h"
#include "PipelineState.h"

#include "RootSignature.h"
#include "Shader.h"
#include "RenderTypes.h"

//bool PipelineState::Init(ID3D12Device* device, const RootSignature& rootSignature, const Shader& vs, const Shader& ps)
//{
//	std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDesc = VertexData::GetInputElementDesc();
//
//	// 2. Thiết lập cấu hình PSO
//	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
//	psoDesc.InputLayout = { inputElementDesc.data(), static_cast<UINT>(inputElementDesc.size())};
//	psoDesc.pRootSignature = rootSignature.Get();
//
//	// Gắn Shader vào
//	psoDesc.VS = { vs.GetBufferPointer(), vs.GetBufferSize() };
//	psoDesc.PS = { ps.GetBufferPointer(), ps.GetBufferSize() };
//
//	// Các trạng thái mặc định (Dùng helper từ d3dx12.h sẽ nhanh hơn)
//	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
//	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
//	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
//	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
//	psoDesc.DepthStencilState.DepthEnable = true; 
//	psoDesc.DepthStencilState.StencilEnable = FALSE;
//	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
//	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
//	psoDesc.SampleMask = UINT_MAX;
//	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
//
//	// Định dạng đầu ra (Phải khớp với SwapChain)
//	psoDesc.NumRenderTargets = 1;
//	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
//	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
//	psoDesc.SampleDesc.Count = 1;
//
//	CHECK(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_Pso)));
//
//	return true;
//}

bool PipelineState::Init(ID3D12Device* device, const RootSignature& rootSignature, const PipelineConfig& config)
{
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDesc = VertexData::GetInputElementDesc();

	// 2. Thiết lập cấu hình PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDesc.data(), static_cast<UINT>(inputElementDesc.size()) };
	psoDesc.pRootSignature = rootSignature.Get();

	// Gắn Shader vào
	if (config.vs)
	{
		psoDesc.VS = { config.vs->GetBufferPointer(), config.vs->GetBufferSize() };
	}
	if (config.ps)
	{
		psoDesc.PS = { config.ps->GetBufferPointer(), config.ps->GetBufferSize() };
	}

	// Các trạng thái mặc định (Dùng helper từ d3dx12.h sẽ nhanh hơn)
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.CullMode = config.cullMode;
	psoDesc.RasterizerState.DepthBias = config.depthBias;
	psoDesc.RasterizerState.DepthBiasClamp = config.depthBiasClamp;
	psoDesc.RasterizerState.SlopeScaledDepthBias = config.slopeScaledDepthBias;

	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = true;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// Định dạng đầu ra (Phải khớp với SwapChain)
	psoDesc.NumRenderTargets = config.numRenderTargets;
	
	for (size_t i = 0; i < config.numRenderTargets; i++)
	{
		psoDesc.RTVFormats[i] = config.rtvFormats[i];
	}

	psoDesc.DSVFormat = config.dsvFormat;
	psoDesc.SampleDesc.Count = 1;

	CHECK(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_Pso)));

	return true;
}
