#include "pch.h"
#include "MipmapManager.h"
#include "Renderer/Shader.h"
#include "Renderer/CommandContext.h"
#include <Renderer/DescriptorManager.h>

MipmapManager::~MipmapManager() = default;

void MipmapManager::Init(ID3D12Device* device, DescriptorManager* descriptorManager)
{
	m_DescriptorManager = descriptorManager;
	InitRootSignature(device);
	
	m_ComputeShader = std::make_unique<Shader>();
	m_ComputeShader->Init(L"src/Renderer/Shaders/generateMipmap.hlsl", "CSMain", "cs_5_1");

	D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineDesc{};
	pipelineDesc.pRootSignature = m_RootSignature.Get();
	pipelineDesc.CS = { m_ComputeShader->GetBufferPointer(), m_ComputeShader->GetBufferSize() };
	pipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	
	CHECK(device->CreateComputePipelineState(&pipelineDesc, IID_PPV_ARGS(&m_PSO)));
}

void MipmapManager::InitMipmap(ID3D12Resource* resource, TextureType textureType, D3D12_RESOURCE_STATES beginState, uint32_t mipLevels, CommandContext* commandContext)
{
	D3D12_RESOURCE_DESC resDesc = resource->GetDesc();

	ID3D12GraphicsCommandList* cmdList = commandContext->BeginImmediateCommand();

	CD3DX12_RESOURCE_BARRIER barriers[2];

	if (beginState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
	{
		barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
			resource,
			beginState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS
		);

		cmdList->ResourceBarrier(1, &barriers[0]);
	}

	cmdList->SetComputeRootSignature(m_RootSignature.Get());
	cmdList->SetPipelineState(m_PSO.Get());

	ID3D12DescriptorHeap* heaps[1] = { m_DescriptorManager->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
	cmdList->SetDescriptorHeaps(1, heaps);
	cmdList->SetComputeRootDescriptorTable(0, m_DescriptorManager->GetDescriptorHeapGpuBase(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	cmdList->SetComputeRootDescriptorTable(1, m_DescriptorManager->GetDescriptorHeapGpuBase(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

	for (size_t i = 0; i < mipLevels - 1; i++)
	{
		uint32_t srvIndex, uavIndex;

		barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
			resource,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			i
		);
		cmdList->ResourceBarrier(1, &barriers[0]);

		srvIndex = InitSRV(resource, i);
		uavIndex = InitUAV(resource, i + 1);

		cmdList->SetComputeRoot32BitConstant(2, srvIndex, 0);
		cmdList->SetComputeRoot32BitConstant(2, uavIndex, 1);
		cmdList->SetComputeRoot32BitConstant(2, (uint32_t)textureType, 2);


		// Compute
		uint32_t srcWidth =		std::max(1u, (uint32_t)resDesc.Width	>> i);
		uint32_t srcHeight =	std::max(1u, (uint32_t)resDesc.Height	>> i);
		uint32_t destWidth =	std::max(1u, (uint32_t)resDesc.Width	>> (i + 1));
		uint32_t destHeight =	std::max(1u, (uint32_t)resDesc.Height	>> (i + 1));

		uint32_t groupCountX = (destWidth + 7) / 8;
		uint32_t groupCountY = (destHeight + 7) / 8;

		cmdList->Dispatch(groupCountX, groupCountY, 1);

		barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
			resource,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			i
		);
		cmdList->ResourceBarrier(1, &barriers[1]);
	}

	barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
		resource,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,	D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		mipLevels - 1
	);

	cmdList->ResourceBarrier(1, &barriers[0]);
	
	commandContext->EndImmediateCommand();
}

void MipmapManager::InitRootSignature(ID3D12Device* device)
{
	// 1. Kiểm tra phiên bản Root Signature cao nhất máy hỗ trợ
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData{};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	CD3DX12_DESCRIPTOR_RANGE1 rangeSRV, rangeUAV;
	rangeSRV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
	rangeUAV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, UINT_MAX, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

	CD3DX12_ROOT_PARAMETER1 params[3];
	params[0].InitAsDescriptorTable(1, &rangeSRV);
	params[1].InitAsDescriptorTable(1, &rangeUAV);
	params[2].InitAsConstants(3, 0, 0);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignDesc{};
	rootSignDesc.Init_1_1(
		_countof(params), params,
		0, nullptr
	);

	ComPtr<ID3DBlob> signatureBlob;
	ComPtr<ID3DBlob> errorBlob;

	HRESULT hr = D3DX12SerializeVersionedRootSignature(&rootSignDesc, featureData.HighestVersion, &signatureBlob, &errorBlob);

	if (errorBlob)
	{
		std::string errorMsg = (char*)errorBlob->GetBufferPointer();
		if (FAILED(hr))
		{
			ENGINE_FATAL("Mipmap Root Signature Serialize Error: {}", errorMsg);
			return;
		}
		else
		{
			ENGINE_WARN("Mimap Root Signature Serialize Warn: {}", errorMsg);
		}
	}

	// 5. Tạo Root Signature thực tế
	CHECK(device->CreateRootSignature(
		0,
		signatureBlob->GetBufferPointer(),
		signatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&m_RootSignature)
	));
}

uint32_t MipmapManager::InitSRV(ID3D12Resource* resource, uint32_t mipSelected)
{
	D3D12_RESOURCE_DESC resDesc = resource->GetDesc();

	CD3DX12_SHADER_RESOURCE_VIEW_DESC desc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(
		resDesc.Format,
		//resDesc.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB ? DXGI_FORMAT_R8G8B8A8_UNORM : resDesc.Format,
		1, mipSelected
	);

	uint32_t srvIndex = m_DescriptorManager->CreateSRV(resource, &desc);
	
	return srvIndex;
}

uint32_t MipmapManager::InitUAV(ID3D12Resource* resource, uint32_t mipSelected)
{
	D3D12_RESOURCE_DESC resDesc = resource->GetDesc();
	
	CD3DX12_UNORDERED_ACCESS_VIEW_DESC desc = CD3DX12_UNORDERED_ACCESS_VIEW_DESC::Tex2D(
		resDesc.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB ? DXGI_FORMAT_R8G8B8A8_UNORM : resDesc.Format ,
		mipSelected
	);

	uint32_t uavIndex = m_DescriptorManager->CreateUAV(resource, &desc);

	return uavIndex;
}
