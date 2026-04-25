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

void Texture::Transition(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState)
{
	if (m_CurrentState == newState) return;

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_Resource.Get(),
		m_CurrentState,
		newState
	);

	m_CurrentState = newState;
	cmdList->ResourceBarrier(1, &barrier);
}
