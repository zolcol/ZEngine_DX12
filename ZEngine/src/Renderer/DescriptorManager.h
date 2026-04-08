#pragma once
#include "DescriptorAllocator.h"

class Buffer;

class DescriptorManager
{
public:
	DescriptorManager();
	~DescriptorManager();

	bool Init(ID3D12Device* device, uint32_t frameCount);

	// Gọi hàm này sau khi đã đăng ký hết các Root CBV để chốt cấu trúc Root Signature
	void SetupStandardDescriptorTables();

	// Trả về toàn bộ danh sách Parameter (cả Root CBV và Tables) cho Root Signature
	const std::vector<CD3DX12_ROOT_PARAMETER1>& GetRootParams() const { return m_RootParams; }

	// Quản lý Descriptor Heap (Bindless)
	uint32_t CreateCBV(Buffer* buffer);

	// Hàm Bind tổng hợp cho mỗi Frame
	void FrameDescriptorBind(ID3D12GraphicsCommandList* cmdList, int currentFrame);

	// Đăng ký Root CBV
	void CreateRootCBV(Buffer* buffer, UINT baseRegister, UINT space, D3D12_ROOT_DESCRIPTOR_FLAGS flags, D3D12_SHADER_VISIBILITY visibility);
	void CreateRootCBVPerFrame(const std::vector<Buffer*>& buffers, UINT baseRegister, UINT space, D3D12_ROOT_DESCRIPTOR_FLAGS flags, D3D12_SHADER_VISIBILITY visibility);

private:
	void BindDescriptorHeaps(ID3D12GraphicsCommandList* cmdList);

private:
	ID3D12Device* m_Device; 
	std::array<DescriptorAllocator, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> m_Allocators;

	uint32_t m_FrameCount;

	// Layout tĩnh cho Root Signature
	std::vector<CD3DX12_ROOT_PARAMETER1> m_RootParams;
	
	// Lưu trữ Ranges cho 3 bảng Unbound (Bắt buộc phải lưu trữ ở đây để tránh bị hủy khi khởi tạo Root Sig)
	CD3DX12_DESCRIPTOR_RANGE1 m_TableRanges[3];
	
	// Giá trị địa chỉ GPU thay đổi theo frame cho các Root CBV
	// m_RootCBVsAddress[frameIndex][paramIndex]
	std::vector<std::vector<D3D12_GPU_VIRTUAL_ADDRESS>> m_RootCBVsAddress;

	uint32_t m_TableParamStartIndex = 0;
	uint32_t m_RootCBVCount = 0;
};
