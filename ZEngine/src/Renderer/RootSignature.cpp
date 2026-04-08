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

	// 2. Lấy toàn bộ danh sách Root Parameters từ DescriptorManager
	const auto& params = descriptorManager->GetRootParams();

	// 3. Khởi tạo Root Signature Desc dựa trên danh sách params
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc{};
	desc.Init_1_1(
		(uint32_t)params.size(), params.data(),
		0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);
	
	ComPtr<ID3DBlob> signatureBlob;
	ComPtr<ID3DBlob> errorBlob;
	
	// 4. Serialize bằng hàm Helper thông minh (tự động hạ cấp xuống 1.0 nếu cần)
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
