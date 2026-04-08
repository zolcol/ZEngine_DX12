#pragma once
#include "DescriptorAllocator.h"

class Buffer;

struct RootCBVData
{
public:
	RootCBVData(D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, UINT baseRegister, UINT space, D3D12_ROOT_DESCRIPTOR_FLAGS flags, D3D12_SHADER_VISIBILITY visibility)
	{
		m_GpuAddress = gpuAddress;
		m_Param.InitAsConstantBufferView(baseRegister, space, flags, visibility);
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() const { return m_GpuAddress; }
	CD3DX12_ROOT_PARAMETER1 GetParam() const { return m_Param; }

private:
	D3D12_GPU_VIRTUAL_ADDRESS m_GpuAddress;
	CD3DX12_ROOT_PARAMETER1 m_Param;
};

class DescriptorManager
{
public:
	DescriptorManager();
	~DescriptorManager();

	bool Init(ID3D12Device* device, uint32_t frameCount);

	std::vector<CD3DX12_ROOT_PARAMETER1> GetRootParams() const { return m_RootParams; }

	uint32_t CreateCBV(Buffer* buffer);

	void FrameDescriptorBind(ID3D12GraphicsCommandList* cmdList, int currentFrame);

	void CreateRootCBV(Buffer* buffer, UINT baseRegister, UINT space, D3D12_ROOT_DESCRIPTOR_FLAGS flags, D3D12_SHADER_VISIBILITY visibility);
	void CreateRootCBVPerFrame(const std::vector<Buffer*>& buffers, UINT baseRegister, UINT space, D3D12_ROOT_DESCRIPTOR_FLAGS flags, D3D12_SHADER_VISIBILITY visibility);


private:
	ID3D12Device* m_Device; 
	std::array<DescriptorAllocator, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> m_Allocators;

	uint32_t m_FrameCount;

	std::vector<std::vector<RootCBVData>> m_RootCBVsData;
	std::vector<CD3DX12_ROOT_PARAMETER1> m_RootParams;
	void BindDescriptorHeap(int startParamIndex, ID3D12GraphicsCommandList* cmdList);

};
