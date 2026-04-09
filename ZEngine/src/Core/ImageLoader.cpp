#include "pch.h"
#include "ImageLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

ImageData::ImageData(const std::string& filePath)
{
	pixels = stbi_load(filePath.c_str(), &width, &height, &chanel, 4);

	if (!pixels)
	{
		ENGINE_ERROR("Failed To Load Image At: {}", filePath);
	}
}

ImageData::~ImageData()
{
	if (pixels)
	{
		stbi_image_free(pixels);
		pixels = nullptr;
	}
}

ImageData::ImageData(ImageData&& other) noexcept:
	width(other.width), height(other.height), chanel(other.chanel), pixels(other.pixels)
{
	other.pixels = nullptr;
}

ImageData& ImageData::operator=(ImageData&& other) noexcept
{
	if (&other != this)
	{
		stbi_image_free(pixels);

		pixels = other.pixels;
		width = other.width;
		height = other.height;
		chanel = other.chanel;

		other.pixels = nullptr;
	}

	return *this;
}
