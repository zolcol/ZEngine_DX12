#include "pch.h"
#include "Swapchain.h"
#include "DescriptorManager.h"

Swapchain::Swapchain() = default;

Swapchain::~Swapchain() = default;

void Swapchain::Init(const SwapChainConfig& config, ID3D12Device* device, DescriptorManager* descriptorManager)
{
	m_SwapchainConfig = config;

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.BufferCount = config.frameCount;
	swapChainDesc.Width = config.width;
	swapChainDesc.Height = config.height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> tempSwapchain;
	CHECK(config.factory->CreateSwapChainForHwnd(config.commandQueue, config.windowHandle, &swapChainDesc, nullptr, nullptr, &tempSwapchain));

	tempSwapchain.As(&m_Swapchain);

	// Create RTV Descriptor
	m_BackBuffers.resize(config.frameCount);
	m_BackBuffersRTV.resize(config.frameCount);

	for (size_t i = 0; i < config.frameCount; i++)
	{
		CHECK(m_Swapchain->GetBuffer(i, IID_PPV_ARGS(&m_BackBuffers[i])));
		
		int index = descriptorManager->CreateRTV(m_BackBuffers[i].Get());
		m_BackBuffersRTV[i] = descriptorManager->GetRTVCPUHandle(index);
	}
}


