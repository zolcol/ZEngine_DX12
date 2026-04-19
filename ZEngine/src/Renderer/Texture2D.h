#pragma once
#include "Texture.h"

class CommandContext;
class DescriptorManager;

enum TextureType
{
	ALBEDO,
	NORMAL,
	ORM,
	EMISSIVE
};

class Texture2D: public Texture
{
public:
	Texture2D() = default;
	~Texture2D() = default;

	bool Init(ID3D12Device* device, CommandContext* commandContext, DescriptorManager* descriptorManager, 
		const std::string& filePath, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, TextureType textureType = ALBEDO);
private:
	const std::string DEFAULT_TEXTURE_PATH[4] = {
		"Resources/Textures/default_albedo.png",		// Albedo
		"Resources/Textures/default_normal.png",		// Normal
		"Resources/Textures/default_orm.png",		// ORM
		"Resources/Textures/default_emissive.png"		// Emissive
	};
};
