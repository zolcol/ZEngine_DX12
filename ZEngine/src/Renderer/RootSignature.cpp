#include "pch.h"
#include "RootSignature.h"

bool RootSignature::Init(ID3D12Device* device)
{
	CD3DX12_ROOT_SIGNATURE_DESC desc{};

	desc.Init(
		0, nullptr, 
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
