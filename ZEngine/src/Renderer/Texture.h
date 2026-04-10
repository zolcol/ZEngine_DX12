#pragma once

class Texture
{
public:
	Texture() = default;
	virtual ~Texture() = default;

	ID3D12Resource* GetResource() const { return m_Resource.Get(); }
	uint32_t GetSRVIndex() const;

protected:
	ComPtr<ID3D12Resource> m_Resource;
	
	uint32_t m_SRVIndex;
	bool m_InitSRV = true;
};