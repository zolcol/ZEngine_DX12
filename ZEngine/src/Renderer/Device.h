#pragma once

class Device
{
public:
	Device();
	~Device() = default;

	ID3D12Device* GetDevice() const { return m_Device.Get(); }

private:
	ComPtr<ID3D12Device> m_Device;
};
