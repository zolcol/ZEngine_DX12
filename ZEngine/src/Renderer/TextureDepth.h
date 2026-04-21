#pragma once
#include "Texture.h"

class CommandContext;
class DescriptorManager;

class TextureDepth: public Texture
{
public:
	TextureDepth() = default;
	~TextureDepth() = default;

	const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSVCpuHandle() const { return m_DSVCpuHandle; }

	bool Init(ID3D12Device* device, CommandContext* commandContext, DescriptorManager* descriptorManager,
		int width, int height, bool InitSRV = false, 
		DXGI_FORMAT format = DXGI_FORMAT_D32_FLOAT,
		float clearDepth = 0, float clearStencil = 0
	);
private:
	uint32_t m_DSVIndex;
	D3D12_CPU_DESCRIPTOR_HANDLE m_DSVCpuHandle;
};
