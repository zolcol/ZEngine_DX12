#include "pch.h"
#include "TextureCube.h"
#include <Core/ImageLoader.h>
#include "DescriptorManager.h"
#include "Buffer.h"
#include "CommandContext.h"

bool TextureCube::Init(ID3D12Device* device, CommandContext* commandContext, DescriptorManager* descriptorManager, const std::string& filePath)
{
	ImageData imageData(filePath);

	// Kiểm tra tính hợp lệ và đảm bảo là Cubemap (ArraySize thường là 6)
	if (!imageData.IsValid() || imageData.metaData.arraySize != 6)
	{
		if (!imageData.IsValid())
		{
			ENGINE_ERROR("TextureCube: Failed to load texture: {}", filePath);
		}
		else
		{
			ENGINE_ERROR("TextureCube: {} is not a valid Cubemap (ArraySize: {}). Expected 6.", filePath, (uint32_t)imageData.metaData.arraySize);
		}
		return false;
	}

	// 1. Khởi tạo Resource và Upload dữ liệu dùng helper ở class cha
	if (!CreateResourceAndUpload(device, commandContext, imageData))
	{
		return false;
	}

	// 2. Tạo Descriptor (SRV) chuyên biệt cho Texture Cube
	CD3DX12_SHADER_RESOURCE_VIEW_DESC srvDesc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::TexCube(
		m_Resource->GetDesc().Format,
		m_MipLevels, 0
	);

	m_SRVIndex = descriptorManager->CreateSRV(m_Resource.Get(), &srvDesc);
	m_Resource->SetName(L"conaca");
	return true;
}
