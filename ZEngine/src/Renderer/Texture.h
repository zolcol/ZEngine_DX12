#pragma once

class CommandContext;
struct ImageData;

class Texture
{
public:
	Texture() = default;
	virtual ~Texture() = default;

	ID3D12Resource* GetResource() const { return m_Resource.Get(); }
	uint32_t GetSRVIndex() const;

	void Transition(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState);

protected:
	ComPtr<ID3D12Resource> m_Resource;
	
	uint32_t m_SRVIndex;
	bool m_InitSRV = true;
	uint32_t m_MipLevels = 1;

	D3D12_RESOURCE_STATES m_CurrentState = D3D12_RESOURCE_STATE_COMMON;

	// Shared logic for creating GPU resource and uploading DDS data
	bool CreateResourceAndUpload(ID3D12Device* device, CommandContext* commandContext, const ImageData& imageData);
};