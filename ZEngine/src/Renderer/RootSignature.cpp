#include "pch.h"
#include "RootSignature.h"

bool RootSignature::Init(ID3D12Device* device)
{
	// CBV
	CD3DX12_DESCRIPTOR_RANGE rangeCBV;
	rangeCBV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, UINT_MAX, 0);

	CD3DX12_ROOT_PARAMETER paramCBV;
	paramCBV.InitAsDescriptorTable(1, &rangeCBV, D3D12_SHADER_VISIBILITY_ALL);


	// SRV
	CD3DX12_DESCRIPTOR_RANGE rangeSRV;
	rangeSRV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, 0);

	CD3DX12_ROOT_PARAMETER paramSRV;
	paramSRV.InitAsDescriptorTable(1, &rangeSRV, D3D12_SHADER_VISIBILITY_ALL);


	// UAV
	CD3DX12_DESCRIPTOR_RANGE rangeUAV;
	rangeUAV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, UINT_MAX, 0);

	CD3DX12_ROOT_PARAMETER paramUAV;
	paramUAV.InitAsDescriptorTable(1, &rangeUAV, D3D12_SHADER_VISIBILITY_ALL);

	// Root Signature Desc
	CD3DX12_ROOT_PARAMETER params[3] = {
	paramCBV,
	paramSRV,
	paramUAV
	};

	CD3DX12_ROOT_SIGNATURE_DESC desc{};
	desc.Init(
		_countof(params), params,
		0, nullptr, 
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	
	ComPtr<ID3DBlob> signatureBlob;
	ComPtr<ID3DBlob> errorBlob;
	
	HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);

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
