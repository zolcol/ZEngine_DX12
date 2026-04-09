#pragma once
#include "DescriptorAllocator.h"

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

	// Đăng ký Root CBV (Tốc độ cao nhất, dùng cho Global Data như Camera/Time)
	void CreateRootCBV(Buffer* buffer, UINT baseRegister, UINT space, D3D12_ROOT_DESCRIPTOR_FLAGS flags, D3D12_SHADER_VISIBILITY visibility);
	void CreateRootCBVPerFrame(const std::vector<Buffer*>& buffers, UINT baseRegister, UINT space, D3D12_ROOT_DESCRIPTOR_FLAGS flags, D3D12_SHADER_VISIBILITY visibility);

	// Gọi hàm này sau cùng để đóng gói 3 bảng Unbound (CBV, SRV, UAV) vào Root Signature
	void SetupStandardDescriptorTables();

	// Cung cấp Layout cho lớp RootSignature
	const std::vector<CD3DX12_ROOT_PARAMETER1>& GetRootParams() const { return m_RootParams; }


	// ==========================================
	// Cấp phát Tài Nguyên (Resource Allocation)
	// ==========================================
	// Xin slot cho View. Các hàm này trả về một Index dùng để Shader truy cập (Bindless).
	uint32_t CreateCBV(Buffer* buffer);
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
	// Layout tĩnh cho Root Signature
	std::vector<CD3DX12_ROOT_PARAMETER1> m_RootParams;
	
	// Các Ranges cho 3 bảng Unbound khổng lồ (Bindless Architecture)
	CD3DX12_DESCRIPTOR_RANGE1 m_TableRanges[3];
	uint32_t m_TableParamStartIndex = 0;
	
	// ------------------------------------------
	// Dynamic Data (Per-Frame Values)
	// ------------------------------------------
	// m_RootCBVsAddress[frameIndex][paramIndex]
	std::vector<std::vector<D3D12_GPU_VIRTUAL_ADDRESS>> m_RootCBVsAddress;
};
