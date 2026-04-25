#include "pch.h"
#include "Texture2D.h"
#include <Core/ImageLoader.h>
#include "DescriptorManager.h"
#include "Buffer.h"
#include "CommandContext.h"
#include "Core/MipmapManager.h"

bool Texture2D::Init(ID3D12Device* device, CommandContext* commandContext, DescriptorManager* descriptorManager, MipmapManager* mipmapManager, 
	const std::string& filePath, DXGI_FORMAT format /*= DXGI_FORMAT_R8G8B8A8_UNORM_SRGB*/, TextureType textureType /*= ALBEDO*/, bool InitMipMap /*= true*/ )
{
	ImageData image(filePath);

	if (!image.pixels)
	{
		std::string typeStr;
		switch (textureType)
		{
		case ALBEDO:   typeStr = "ALBEDO"; break;
		case NORMAL:   typeStr = "NORMAL"; break;
		case ORM:      typeStr = "ORM"; break;
		case EMISSIVE: typeStr = "EMISSIVE"; break;
		default:       typeStr = "UNKNOWN"; break;
		}

		if (filePath.empty())
		{
			ENGINE_INFO("No {} texture provided in model. Loading default.", typeStr);
		}
		else
		{
			ENGINE_ERROR("Failed to find {} texture file at: {}. Loading default.", typeStr, filePath);
		}
		
		image = ImageData(DEFAULT_TEXTURE_PATH[textureType]);

		if (!image.pixels)
		{
			ENGINE_FATAL("CRITICAL ERROR: Failed to load DEFAULT {} texture from: {}. Please ensure your 'Resources' folder is correctly set up!", typeStr, DEFAULT_TEXTURE_PATH[textureType]);
			return false;
		}
	}

	if (InitMipMap)
	{
		m_MipLevels = (uint32_t)std::floor(std::log2(std::max(image.width, image.height))) + 1;
	}

	// Create Texture Resource
	CD3DX12_RESOURCE_DESC texDesc;
	texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		format, image.width, image.height,
		1, m_MipLevels,
		1, 0,
		m_MipLevels == 1 ? D3D12_RESOURCE_FLAG_NONE : D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_TEXTURE_LAYOUT_UNKNOWN
	);

	CD3DX12_HEAP_PROPERTIES heapProp(D3D12_HEAP_TYPE_DEFAULT);

	CHECK(device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_Resource)
	));

	m_CurrentState = D3D12_RESOURCE_STATE_COPY_DEST;

	UINT64 stagingBufferSize = GetRequiredIntermediateSize(m_Resource.Get(), 0, 1);

	Buffer stagingBuffer;
	stagingBuffer.Init(device, stagingBufferSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);

	ID3D12GraphicsCommandList* cmdList = stagingBuffer.UpdateDataToTexture(commandContext, image.pixels, m_Resource.Get(), image.width, image.height);

	this->Transition(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	commandContext->EndImmediateCommand();

	if (m_MipLevels > 1)
	{
		mipmapManager->InitMipmap(m_Resource.Get(), textureType, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, m_MipLevels, commandContext);
	}

	CD3DX12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(texDesc.Format);

	m_SRVIndex = descriptorManager->CreateSRV(m_Resource.Get(), &srvDesc);

	return true;
}
