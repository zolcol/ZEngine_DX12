#pragma once

class CommandContext;
class Shader;
class DescriptorManager;

class MipmapManager
{
public:
	MipmapManager() = default;
	~MipmapManager();
	
	void Init(ID3D12Device* device, DescriptorManager* descriptorManager);
	void InitMipmap(ID3D12Resource* resource, TextureType textureType, D3D12_RESOURCE_STATES beginState, uint32_t mipLevels, CommandContext* commandContext);

private:
	void InitRootSignature(ID3D12Device* device);

	uint32_t InitSRV(ID3D12Resource* resource, uint32_t mipSelected);
	uint32_t InitUAV(ID3D12Resource* resource, uint32_t mipSelected);

	DescriptorManager* m_DescriptorManager;

	ComPtr<ID3D12RootSignature> m_RootSignature;
	ComPtr<ID3D12PipelineState> m_PSO;
	std::unique_ptr<Shader> m_ComputeShader;
};
