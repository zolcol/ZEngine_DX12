#pragma once

class RootSignature
{
public:
	RootSignature() = default;
	~RootSignature() = default;

	ID3D12RootSignature* Get() const { return m_RootSignature.Get(); }

	bool Init(ID3D12Device* device);
private:
	ComPtr<ID3D12RootSignature> m_RootSignature;
};
