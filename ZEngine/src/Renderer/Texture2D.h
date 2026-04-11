#pragma once
#include "Texture.h"

class CommandContext;
class DescriptorManager;

enum TextureType
{
	ALBEDO,
};

class Texture2D: public Texture
{
public:
	Texture2D() = default;
	~Texture2D() = default;

	bool Init(ID3D12Device* device, CommandContext* commandContext, DescriptorManager* descriptorManager, 
		const std::string& filePath, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, TextureType textureType = ALBEDO);
private:
	const std::string DEFAULT_TEXTURE_PATH[1] = {
		"Resources/Textures/anime.png"		// Albedo
	};
};
