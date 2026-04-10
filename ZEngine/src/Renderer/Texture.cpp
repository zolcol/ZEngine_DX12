#include "pch.h"
#include "Texture.h"

uint32_t Texture::GetSRVIndex() const
{
	if (m_InitSRV)
	{
		return m_SRVIndex;
	}
	else
	{
		ENGINE_ERROR("Cannot Get SRV Index of texture have InitSRV = FALSE");
		return UINT32_MAX;
	}
}
