#pragma once

class CommandContext
{
public:
	CommandContext();
	~CommandContext();

private:
	ComPtr<ID3D12CommandQueue> m_CommandQueue;

};
