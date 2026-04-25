#pragma once

// ImageData bọc ScratchImage của DirectXTex để quản lý dữ liệu ảnh trên RAM
struct ImageData
{
	std::unique_ptr<DirectX::ScratchImage> scratchImage;
	DirectX::TexMetadata metaData{};

	ImageData(const std::string& filePath);
	~ImageData();

	// Không cho phép copy để tránh xung đột vùng nhớ
	ImageData(const ImageData&) = delete;
	ImageData& operator=(const ImageData&) = delete;

	// Hỗ trợ Move Semantics để chuyển quyền sở hữu tài nguyên
	ImageData(ImageData&& other) noexcept
		: scratchImage(std::move(other.scratchImage)), metaData(other.metaData)
	{
		other.metaData = {};
	}

	ImageData& operator=(ImageData&& other) noexcept
	{
		if (this != &other)
		{
			scratchImage = std::move(other.scratchImage);
			metaData = other.metaData;
			other.metaData = {};
		}
		return *this;
	}

	bool IsValid() const { return scratchImage != nullptr && scratchImage->GetImageCount() > 0; }
};
