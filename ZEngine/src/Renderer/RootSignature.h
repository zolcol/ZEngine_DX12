#pragma once

class DescriptorManager;

class RootSignature
{
public:
	RootSignature();
	~RootSignature();

	ID3D12RootSignature* Get() const { return m_RootSignature.Get(); }

	bool Init(ID3D12Device* device, DescriptorManager* descriptorManager);
private:
	ComPtr<ID3D12RootSignature> m_RootSignature;
};
