#include "pch.h"
#include "RootSignature.h"
#include "DescriptorManager.h"

RootSignature::RootSignature() = default;

RootSignature::~RootSignature() = default;

bool RootSignature::Init(ID3D12Device* device, DescriptorManager* descriptorManager)
{
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData{};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// CBV
	CD3DX12_DESCRIPTOR_RANGE1 rangeCBV;
	rangeCBV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, UINT_MAX, 0, 0, 
		D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE | D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

	CD3DX12_ROOT_PARAMETER1 paramCBV;
	paramCBV.InitAsDescriptorTable(1, &rangeCBV, D3D12_SHADER_VISIBILITY_ALL);


	// SRV
	CD3DX12_DESCRIPTOR_RANGE1 rangeSRV;
	rangeSRV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, 0, 0,
		D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

	CD3DX12_ROOT_PARAMETER1 paramSRV;
	paramSRV.InitAsDescriptorTable(1, &rangeSRV, D3D12_SHADER_VISIBILITY_ALL);


	// UAV
	CD3DX12_DESCRIPTOR_RANGE1 rangeUAV;
	rangeUAV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, UINT_MAX, 0, 0,
		D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);


	CD3DX12_ROOT_PARAMETER1 paramUAV;
	paramUAV.InitAsDescriptorTable(1, &rangeUAV, D3D12_SHADER_VISIBILITY_ALL);

	std::vector<CD3DX12_ROOT_PARAMETER1> params = descriptorManager->GetRootParams();
	params.push_back(paramCBV);
	params.push_back(paramSRV);
	params.push_back(paramUAV);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc{};
	desc.Init_1_1(
		params.size(), params.data(),
		0, nullptr,
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

	CHECK(device->CreateRootSignature(
		0, 
		signatureBlob->GetBufferPointer(),
		signatureBlob->GetBufferSize(), 
		IID_PPV_ARGS(&m_RootSignature)
	));

	CHECK(hr);
	return true;
}
