#pragma once
#include "DescriptorAllocator.h"
#include "RenderTypes.h"

class Buffer;

// =====================================================================
// Bộ não quản lý toàn bộ hệ thống Binding Tài Nguyên của Engine
// Xử lý cả Root Parameters (Layout) và Descriptor Heaps (Values)
// =====================================================================
class DescriptorManager
{
public:
	DescriptorManager();
	~DescriptorManager();

	// ==========================================
	// Khởi tạo & Cấu hình Root Signature
	// ==========================================
	bool Init(ID3D12Device* device, uint32_t frameCount);

	// Gán tài nguyên vào Slot (Descriptor Manager sẽ lưu lại để Bind mỗi frame)
	void SetRootCBV(RootSlot slot, D3D12_GPU_VIRTUAL_ADDRESS address);
	void SetRootCBVPerFrame(RootSlot slot, uint32_t frameIndex, D3D12_GPU_VIRTUAL_ADDRESS address);
	
	void SetRootSRV(RootSlot slot, D3D12_GPU_VIRTUAL_ADDRESS address);
	void SetRootSRVPerFrame(RootSlot slot, uint32_t frameIndex, D3D12_GPU_VIRTUAL_ADDRESS address);

	// Cung cấp Static Sampler cho RootSignature (Vẫn giữ nguyên vì sampler là tĩnh)
	const std::vector<CD3DX12_STATIC_SAMPLER_DESC>& GetStaticSamplers() const { return m_StaticSamplers; }

	// ==========================================
	// Cấp phát Tài Nguyên (Resource Allocation)
	// ==========================================
	// Xin slot cho View. Các hàm này trả về một Index dùng để Shader truy cập (Bindless).
	uint32_t CreateCBV(Buffer* buffer);
	uint32_t CreateSRV(ID3D12Resource* texture, const CD3DX12_SHADER_RESOURCE_VIEW_DESC* srvDesc);
	uint32_t CreateRTV(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC* rtvDesc = nullptr);
	uint32_t CreateDSV(ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC* dsvDesc = nullptr);
	

	// ==========================================
	// Binding & Rendering
	// ==========================================
	// Bind tất cả tài nguyên (Root CBV và Heaps) của frame hiện tại vào Command List
	void BindDescriptors(ID3D12GraphicsCommandList* cmdList, int currentFrame);

	// Lấy địa chỉ CPU an toàn của RTV/DSV để dùng cho hàm OMSetRenderTargets
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCPUHandle(uint32_t index);
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCPUHandle(uint32_t index);

private:
	void InitStaticSamplers();
	void BindDescriptorHeaps(ID3D12GraphicsCommandList* cmdList);

private:
	ID3D12Device* m_Device = nullptr; 
	uint32_t m_FrameCount = 0;

	// ------------------------------------------
	// Heaps (The Pools of Descriptors)
	// ------------------------------------------
	std::array<DescriptorAllocator, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> m_Allocators;

	// ------------------------------------------
	// Root Signature Data
	// ------------------------------------------
	// Static Sampler
	std::vector<CD3DX12_STATIC_SAMPLER_DESC> m_StaticSamplers;
	
	// ------------------------------------------
	// Dynamic Data (Per-Frame Values)
	// ------------------------------------------
	// Lưu trữ địa chỉ GPU cho từng Slot, cho từng Frame
	// m_SlotAddresses[frameIndex][slotIndex]
	std::vector<std::array<D3D12_GPU_VIRTUAL_ADDRESS, RootSlot::Count>> m_SlotAddresses;
};
