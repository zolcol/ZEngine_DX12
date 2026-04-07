#pragma once
#include "DescriptorAllocator.h"

class Buffer;

class DescriptorManager
{
public:
	DescriptorManager();
	~DescriptorManager();

	bool Init(ID3D12Device* device);
	uint32_t CreateCBV(Buffer* buffer);
	void BindDescriptorHeap(ID3D12GraphicsCommandList* cmdList);

private:
	std::array<DescriptorAllocator, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> m_Allocators;
	ID3D12Device* m_Device;
};
