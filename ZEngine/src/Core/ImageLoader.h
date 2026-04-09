#pragma once

struct ImageData
{
	int width = 0;
	int height = 0;
	int chanel = 0;
	unsigned char* pixels = nullptr;

	ImageData(const std::string& filePath);
	~ImageData();

	ImageData(const ImageData&) = delete;
	ImageData& operator=(const ImageData&) = delete;

	ImageData(ImageData&& other) noexcept;
	ImageData& operator=(ImageData&& other) noexcept;

};