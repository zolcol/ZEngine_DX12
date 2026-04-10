#include "pch.h"
#include "TextureDepth.h"
#include "DescriptorManager.h"

bool TextureDepth::Init(ID3D12Device* device, CommandContext* commandContext, DescriptorManager* descriptorManager, int width, int height, bool InitSRV /*= false*/, DXGI_FORMAT format /*= DXGI_FORMAT_D32_FLOAT*/, float clearDepth /*= 1*/, float clearStencil /*= 0 */)
{
	// Kiểm tra tính hợp lệ của Format (chỉ chấp nhận các format thuộc nhóm Depth Stencil)
	if (format != DXGI_FORMAT_D32_FLOAT &&
		format != DXGI_FORMAT_R32_TYPELESS &&
		format != DXGI_FORMAT_D24_UNORM_S8_UINT &&
		format != DXGI_FORMAT_R24G8_TYPELESS &&
		format != DXGI_FORMAT_D32_FLOAT_S8X24_UINT &&
		format != DXGI_FORMAT_R32G8X24_TYPELESS)
	{
		ENGINE_ERROR("Invalid Depth Stencil format! Defaulting to DXGI_FORMAT_D32_FLOAT.");
		format = DXGI_FORMAT_D32_FLOAT;
	}

	m_InitSRV = InitSRV;

	// Khởi tạo mặc định
	DXGI_FORMAT dsvFormat = format;
	DXGI_FORMAT srvFormat = DXGI_FORMAT_UNKNOWN;

	if (InitSRV)
	{
		if (format == DXGI_FORMAT_D32_FLOAT || format == DXGI_FORMAT_R32_TYPELESS)
		{
			format = DXGI_FORMAT_R32_TYPELESS;
			srvFormat = DXGI_FORMAT_R32_FLOAT;
			dsvFormat = DXGI_FORMAT_D32_FLOAT;
		}
		else if (format == DXGI_FORMAT_D24_UNORM_S8_UINT || format == DXGI_FORMAT_R24G8_TYPELESS)
		{
			format = DXGI_FORMAT_R24G8_TYPELESS;
			srvFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		}
		else // DXGI_FORMAT_D32_FLOAT_S8X24_UINT hoặc DXGI_FORMAT_R32G8X24_TYPELESS
		{
			format = DXGI_FORMAT_R32G8X24_TYPELESS;
			srvFormat = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
			dsvFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		}
	}

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

	CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		format, width, height,
		1, 0, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
		D3D12_TEXTURE_LAYOUT_UNKNOWN
	);

	CD3DX12_CLEAR_VALUE clearValue(dsvFormat, clearDepth, clearStencil);
	
	CHECK(device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE,
		&resDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		IID_PPV_ARGS(&m_Resource)
	));

	if (InitSRV)
	{
		CD3DX12_SHADER_RESOURCE_VIEW_DESC srvDesc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(srvFormat);

		D3D12_DEPTH_STENCIL_VIEW_DESC dsv{};
		dsv.Format = dsvFormat;
		dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsv.Texture2D.MipSlice = 0;

		m_SRVIndex = descriptorManager->CreateSRV(m_Resource.Get(), &srvDesc);
		m_DSVIndex = descriptorManager->CreateDSV(m_Resource.Get(), &dsv);
	}
	else
	{
		m_DSVIndex = descriptorManager->CreateDSV(m_Resource.Get());
	}

	m_DSVCpuHandle = descriptorManager->GetDSVCPUHandle(m_DSVIndex);

	return true;
}
