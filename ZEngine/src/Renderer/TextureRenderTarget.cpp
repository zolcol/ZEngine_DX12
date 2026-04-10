#include "pch.h"
#include "TextureRenderTarget.h"
#include "DescriptorManager.h"

bool TextureRenderTarget::Init(ID3D12Device* device, CommandContext* commandContext, DescriptorManager* descriptorManager, int width, int height, DXGI_FORMAT format, bool InitSRV /*= false*/)
{
	m_InitSRV = InitSRV;

	CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		format, width, height,
		1, 0, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
		D3D12_TEXTURE_LAYOUT_UNKNOWN
	);

	CD3DX12_HEAP_PROPERTIES heapProp(D3D12_HEAP_TYPE_DEFAULT);

	CHECK(device->CreateCommittedResource(
		&heapProp, D3D12_HEAP_FLAG_NONE,
		&texDesc, 
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		nullptr, 
		IID_PPV_ARGS(&m_Resource)
	));

	m_RTVIndex = descriptorManager->CreateRTV(m_Resource.Get());

	if (InitSRV)
	{
		CD3DX12_SHADER_RESOURCE_VIEW_DESC srvDesc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(format);
		m_SRVIndex = descriptorManager->CreateSRV(m_Resource.Get(), &srvDesc);
	}

	m_RTVCpuHandle = descriptorManager->GetRTVCPUHandle(m_RTVIndex);

	return true;
}

bool TextureRenderTarget::Init(DescriptorManager* descriptorManager, IDXGISwapChain4* swapchain, int bufferIndex)
{
	CHECK(swapchain->GetBuffer(bufferIndex, IID_PPV_ARGS(&m_Resource)));

	m_RTVIndex = descriptorManager->CreateRTV(m_Resource.Get());
	m_InitSRV = false;

	m_RTVCpuHandle = descriptorManager->GetRTVCPUHandle(m_RTVIndex);

	return true;
}
