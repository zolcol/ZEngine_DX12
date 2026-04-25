#pragma once
#include "Texture.h"

class CommandContext;
class DescriptorManager;

class Texture2D: public Texture
{
public:
	Texture2D() = default;
	~Texture2D() = default;

	bool Init(ID3D12Device* device, CommandContext* commandContext, DescriptorManager* descriptorManager,
		const std::string& filePath, TextureType textureType = ALBEDO);
private:
	const std::string DEFAULT_TEXTURE_PATH[4] = {
		"Resources/Textures/default_albedo.dds",		// Albedo
		"Resources/Textures/default_normal.dds",		// Normal
		"Resources/Textures/default_orm.dds",		// ORM
		"Resources/Textures/default_emissive.dds"		// Emissive
	};

	uint32_t m_MipLevels = 1;
};
