#pragma once

class RootSignature;
class Shader;

class PipelineState
{
public:
	PipelineState() = default;
	~PipelineState() = default;

	ID3D12PipelineState* Get() const { return m_Pso.Get(); }
	bool Init(ID3D12Device* device, const RootSignature& rootSignature, const Shader& vs, const Shader& ps);

private:
	ComPtr<ID3D12PipelineState> m_Pso;
};
