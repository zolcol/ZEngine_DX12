#pragma once

#include "TextureRenderTarget.h"
class DescriptorManager;

struct SwapChainConfig
{
	HWND windowHandle = nullptr;
	IDXGIFactory4* factory = nullptr;
	ID3D12CommandQueue* commandQueue = nullptr;
	uint32_t width = 800;
	uint32_t height = 600;
	uint32_t frameCount = 3;
};

// =====================================================================
// Lớp quản lý hiển thị (Display/Presentation) và Back Buffers
// Sử dụng DXGI để giao tiếp trực tiếp với Windows/Màn hình
// =====================================================================
class Swapchain
{
public:
	Swapchain();
	~Swapchain();

	// Khởi tạo Swapchain và đăng ký các RTV vào DescriptorManager
	void Init(const SwapChainConfig& config, ID3D12Device* device, DescriptorManager* descriptorManager);

	// ==========================================
	// Getters
	// ==========================================
	IDXGISwapChain4* GetSwapchain() const { return m_Swapchain.Get(); }
	
	// Trả về số thứ tự của Back Buffer đang được dùng cho frame này (thường là 0, 1, 2)
	uint32_t GetCurrentBackBufferIndex() const { return m_Swapchain->GetCurrentBackBufferIndex(); }
	
	// Lấy tài nguyên (Texture2D) của một frame cụ thể
	ID3D12Resource* GetBackBuffer(int currentFrame) const { return m_BackBuffers[currentFrame]->GetResource(); }
	
	// Lấy địa chỉ CPU của Render Target để sử dụng cho hàm OMSetRenderTargets
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetCurrentRTVCpuHandle() const { return m_BackBuffers[GetCurrentBackBufferIndex()]->GetRTVCpuHanlde(); }

private:
	SwapChainConfig m_SwapchainConfig;
	ComPtr<IDXGISwapChain4> m_Swapchain;

	std::vector<std::unique_ptr<TextureRenderTarget>> m_BackBuffers;
};
