#pragma once

class Device
{
public:
	Device() = default;
	~Device() = default;

	ID3D12Device* GetDevice() const { return m_Device.Get(); }
	IDXGIFactory7* GetFactory() const { return m_Factory.Get(); }

	bool Init();

private:
	ComPtr<ID3D12Device> m_Device;
	ComPtr<IDXGIFactory7> m_Factory;
};
