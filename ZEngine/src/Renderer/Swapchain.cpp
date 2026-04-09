#include "pch.h"
#include "Swapchain.h"
#include "DescriptorManager.h"

Swapchain::Swapchain() = default;
Swapchain::~Swapchain() = default;

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

	// 2. Tạo Render Target Views (RTV) cho các Back Buffers
	m_BackBuffers.resize(config.frameCount);
	m_BackBuffersRTV.resize(config.frameCount);

	for (size_t i = 0; i < config.frameCount; i++)
	{
		// Lấy resource thực tế từ Swapchain
		CHECK(m_Swapchain->GetBuffer(i, IID_PPV_ARGS(&m_BackBuffers[i])));
		
		// Đăng ký resource này với DescriptorManager để lấy RTV Index
		int index = descriptorManager->CreateRTV(m_BackBuffers[i].Get());
		
		// Lưu lại CPU Handle để dùng siêu nhanh trong BeginFrame
		m_BackBuffersRTV[i] = descriptorManager->GetRTVCPUHandle(index);
	}

	ENGINE_INFO("Swapchain Initialized: {}x{}, Buffers: {}", config.width, config.height, config.frameCount);
}
