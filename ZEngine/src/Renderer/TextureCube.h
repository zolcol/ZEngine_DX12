#pragma once
#include "Texture.h"

class CommandContext;
class DescriptorManager;

class TextureCube : public Texture
{
public:
	TextureCube() = default;
	~TextureCube() = default;

	bool Init(ID3D12Device* device, CommandContext* commandContext, DescriptorManager* descriptorManager,
		const std::string& filePath);
};
