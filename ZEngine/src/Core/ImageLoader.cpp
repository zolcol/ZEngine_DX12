#include "pch.h"
#include "ImageLoader.h"

ImageData::ImageData(const std::string& filePath)
{
	if (filePath.empty()) return;

	scratchImage = std::make_unique<DirectX::ScratchImage>();
	std::wstring wPath = StringToWString(filePath);

	// Chỉ hỗ trợ nạp file DDS
	HRESULT hr = DirectX::LoadFromDDSFile(wPath.c_str(), DirectX::DDS_FLAGS_NONE, &metaData, *scratchImage);

	if (FAILED(hr))
	{
		// Nếu nạp thất bại, giải phóng vùng nhớ. Texture2D sẽ lo việc fallback.
		scratchImage.reset();
		return;
	}

	if (scratchImage->GetImageCount() == 0)
	{
		ENGINE_ERROR("DDS file contains no data: {}", filePath);
		scratchImage.reset();
	}
}

ImageData::~ImageData() = default;
