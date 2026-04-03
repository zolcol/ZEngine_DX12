#include "pch.h"
#include "Device.h"

bool Device::Init()
{
	// Enable Debug Layer
#ifdef _DEBUG
	ComPtr<ID3D12Debug> debugControl;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugControl))))
	{
		debugControl->EnableDebugLayer();
		ENGINE_INFO("ENABLED DEBUG LAYER");
	}
	else
	{
		ENGINE_ERROR("FAILED TO ENABLE DEBUG LAYER");
	}
#endif // _DEBUG

	CHECK(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&m_Factory)));

	ComPtr<IDXGIAdapter4> adapter;
	for (int i = 0; m_Factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND; i++)
	{
		DXGI_ADAPTER_DESC1 desc{};
		adapter->GetDesc1(&desc);
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
		ENGINE_INFO("SELECTED DEVICE: {}", WStringToString(desc.Description));
		break;
	}

	CHECK(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_Device)));

	return true;
}
