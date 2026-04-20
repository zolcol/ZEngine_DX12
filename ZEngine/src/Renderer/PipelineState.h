#pragma once

class RootSignature;
class Shader;

struct PipelineConfig
{
	const Shader* vs = nullptr;
	const Shader* ps = nullptr;

	// Cấu hình Render Target
	UINT numRenderTargets = 1;
	DXGI_FORMAT rtvFormats[8] = { DXGI_FORMAT_UNKNOWN };
	DXGI_FORMAT dsvFormat = DXGI_FORMAT_D32_FLOAT;

	// Cấu hình Rasterizer cho Shadow
	D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_BACK;
	INT depthBias = 0;             // Dành cho Shadow (ví dụ: 100000)
	FLOAT depthBiasClamp = 0.0f;   // Dành cho Shadow
	FLOAT slopeScaledDepthBias = 0.0f; // Dành cho Shadow (ví dụ: 1.5f)

};
class PipelineState
{
public:
	PipelineState() = default;
	~PipelineState() = default;

	ID3D12PipelineState* Get() const { return m_Pso.Get(); }
	bool Init(ID3D12Device* device, const RootSignature& rootSignature, const PipelineConfig& config);

private:
	ComPtr<ID3D12PipelineState> m_Pso;
};
