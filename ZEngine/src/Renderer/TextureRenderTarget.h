#pragma once
#include "Texture.h"

class CommandContext;
class DescriptorManager;

class TextureRenderTarget: public Texture
{
public:
	TextureRenderTarget() = default;
	~TextureRenderTarget() = default;

	uint32_t GetRTVIndex() const { return m_RTVIndex; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetRTVCpuHanlde() const { return m_RTVCpuHandle; }

	bool Init(ID3D12Device* device, CommandContext* commandContext, DescriptorManager* descriptorManager, int width, int height, DXGI_FORMAT format, bool InitSRV = false);
	bool Init(DescriptorManager* descriptorManager, IDXGISwapChain4* swapchain, int bufferIndex);
private:
	uint32_t m_RTVIndex;
	D3D12_CPU_DESCRIPTOR_HANDLE m_RTVCpuHandle;
};
