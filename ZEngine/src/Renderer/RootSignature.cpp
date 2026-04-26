#include "pch.h"
#include "RootSignature.h"
#include "DescriptorManager.h"

RootSignature::RootSignature() = default;
RootSignature::~RootSignature() = default;

bool RootSignature::Init(ID3D12Device* device, DescriptorManager* descriptorManager)
{
	// 1. Kiểm tra phiên bản Root Signature cao nhất máy hỗ trợ
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData{};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// 2. Định nghĩa Layout cố định (Contract)
	CD3DX12_ROOT_PARAMETER1 params[RootSlot::Count];

	// Slot 0: Root Constants (b0, space1)
	params[Slot_RootConstants].InitAsConstants(3, 0, 1);

	// Root Descriptors (Ưu tiên dùng space2 để tránh xung đột bindless ở space0)
	params[Slot_GlobalCBV].InitAsConstantBufferView(0, 2);
	params[Slot_ShadowCBV].InitAsConstantBufferView(1, 2);
	params[Slot_MaterialSRV].InitAsShaderResourceView(0, 2);
	params[Slot_ObjectSRV].InitAsShaderResourceView(1, 2);
	params[Slot_LightSRV].InitAsShaderResourceView(2, 2);

	// Descriptor Tables (Bindless - space0)
	static CD3DX12_DESCRIPTOR_RANGE1 rangeCBV, rangeSRV[2], rangeUAV;
	rangeCBV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, UINT_MAX, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
	
	// Range 0: Cho Texture2D (space0)
	rangeSRV[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
	// Range 1: Cho TextureCube (space3) - Ép offset về 0 để dùng chung Index với space0
	rangeSRV[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, 0, 3, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, 0);

	rangeUAV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, UINT_MAX, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

	params[Slot_CBVTable].InitAsDescriptorTable(1, &rangeCBV);
	params[Slot_SRVTable].InitAsDescriptorTable(2, rangeSRV);
	params[Slot_UAVTable].InitAsDescriptorTable(1, &rangeUAV);

	// 3. Static Samplers
	const auto& staticSamplers = descriptorManager->GetStaticSamplers();

	// 4. Khởi tạo Root Signature Desc
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc{};
	desc.Init_1_1(
		RootSlot::Count, params,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);
	
	ComPtr<ID3DBlob> signatureBlob;
	ComPtr<ID3DBlob> errorBlob;
	
	HRESULT hr = D3DX12SerializeVersionedRootSignature(&desc, featureData.HighestVersion, &signatureBlob, &errorBlob);

	if (errorBlob)
	{
		std::string errorMsg = (char*)errorBlob->GetBufferPointer();
		if (FAILED(hr))
		{
			ENGINE_FATAL("Root Signature Serialize Error: {}", errorMsg);
			return false;
		}
		else
		{
			ENGINE_WARN("Root Signature Serialize Warn: {}", errorMsg);
		}
	}

	// 5. Tạo Root Signature thực tế
	CHECK(device->CreateRootSignature(
		0, 
		signatureBlob->GetBufferPointer(),
		signatureBlob->GetBufferSize(), 
		IID_PPV_ARGS(&m_RootSignature)
	));

	return true;
}
