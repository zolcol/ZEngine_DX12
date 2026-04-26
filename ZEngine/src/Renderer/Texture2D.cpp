#include "pch.h"
#include "Texture2D.h"
#include <Core/ImageLoader.h>
#include "DescriptorManager.h"
#include "Buffer.h"
#include "CommandContext.h"
bool Texture2D::Init(ID3D12Device* device, CommandContext* commandContext, DescriptorManager* descriptorManager, 
	const std::string& filePath, TextureType textureType)
{
	ImageData imageData(filePath);

	// Kiểm tra tính hợp lệ và đảm bảo không phải là Texture Array/Cube
	if (!imageData.IsValid() || imageData.metaData.arraySize > 1)
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

		if (!imageData.IsValid())
		{
			ENGINE_WARN("Failed to load {} texture: {}. Falling back to default.", typeStr, filePath);
		}
		else
		{
			ENGINE_WARN("Texture2D: {} contains texture array (size {}). Only single slice is supported. Falling back to default.", filePath, (uint32_t)imageData.metaData.arraySize);
		}

		imageData = ImageData(DEFAULT_TEXTURE_PATH[textureType]);

		if (!imageData.IsValid())
		{
			ENGINE_FATAL("CRITICAL ERROR: Default texture missing: {}", DEFAULT_TEXTURE_PATH[textureType]);
			return false;
		}
	}

	// 1. Khởi tạo Resource và Upload dữ liệu dùng helper ở class cha
	if (!CreateResourceAndUpload(device, commandContext, imageData))
	{
		return false;
	}

	// 2. Tạo Descriptor (SRV) chuyên biệt cho Texture 2D
	CD3DX12_SHADER_RESOURCE_VIEW_DESC srvDesc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(m_Resource->GetDesc().Format);
	m_SRVIndex = descriptorManager->CreateSRV(m_Resource.Get(), &srvDesc);

	return true;
}
