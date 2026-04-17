#include "pch.h"
#include "Swapchain.h"
#include "DescriptorManager.h"

Swapchain::Swapchain() = default;
Swapchain::~Swapchain()
{
	CloseHandle(m_WaitableObject);
};

void Swapchain::Init(const SwapChainConfig& config, ID3D12Device* device, DescriptorManager* descriptorManager)
{
	m_SwapchainConfig = config;

	// 1. Cấu hình Swapchain (Màn hình hiển thị)
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.BufferCount = config.frameCount;
	swapChainDesc.Width = config.width;
	swapChainDesc.Height = config.height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // Bắt buộc cho DX12 để lật frame mượt mà
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

	// DXGI Factory tạo Swapchain
	ComPtr<IDXGISwapChain1> tempSwapchain;
	CHECK(config.factory->CreateSwapChainForHwnd(
		config.commandQueue, 
		config.windowHandle, 
		&swapChainDesc, 
		nullptr, nullptr, 
		&tempSwapchain
	));

	// Nâng cấp interface lên bản 4 để có thêm tính năng
	tempSwapchain.As(&m_Swapchain);

	m_Swapchain->SetMaximumFrameLatency(config.frameCount - 1);
	m_WaitableObject = m_Swapchain->GetFrameLatencyWaitableObject();

	//// 2. Tạo BackBuffer chứa RTV
	m_BackBuffers.resize(config.frameCount);
	for (size_t i = 0; i < config.frameCount; i++)
	{
		m_BackBuffers[i] = std::make_unique<TextureRenderTarget>();
		m_BackBuffers[i]->Init(descriptorManager, m_Swapchain.Get(), i);
	}

	ENGINE_INFO("Swapchain Initialized: {}x{}, Buffers: {}", config.width, config.height, config.frameCount);
}

void Swapchain::WaitForLatencyWaitableObject()
{
	WaitForSingleObject(m_WaitableObject, 1000);
}
