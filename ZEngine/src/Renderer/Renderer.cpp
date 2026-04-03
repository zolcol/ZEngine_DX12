#include "pch.h"
#include "Renderer.h"

bool Renderer::Init(HWND hwnd, int width, int height, uint32_t frameCount)
{
	m_FrameCount = frameCount;
	m_FramesInFlight = frameCount;

	m_Device = std::make_unique<Device>();
	if (!m_Device->Init()) return false;

	m_CommandContext = std::make_unique<CommandContext>();
	m_CommandContext->Init(m_Device->GetDevice(), m_FramesInFlight);

	m_Fence = std::make_unique<Fence>();
	if (!m_Fence->Init(m_Device->GetDevice())) return false;

	SwapChainConfig config{};
	config.windowHandle = hwnd;
	config.factory = m_Device->GetFactory();
	config.commandQueue = m_CommandContext->GetCommandQueue();
	config.width = width;
	config.height = height;
	config.frameCount = m_FrameCount;

	m_Swapchain = std::make_unique<Swapchain>();
	m_Swapchain->Init(config, m_Device->GetDevice());

	return true;
}

void Renderer::BeginFrame()
{
	m_CurrentBufferIndex = m_Swapchain->GetCurrentBackBufferIndex();
	auto& frameRes = m_CommandContext->GetFrameCommandResource(m_CurrentFrame);
	auto commandList = frameRes.commandList.Get();

 	m_Fence->Wait(frameRes.fenceValue);

	frameRes.commandAllocator->Reset();
	commandList->Reset(frameRes.commandAllocator.Get(), nullptr);

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_Swapchain->GetBackBuffer(m_CurrentBufferIndex),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	commandList->ResourceBarrier(1, &barrier);

	D3D12_CPU_DESCRIPTOR_HANDLE currentRTV = m_Swapchain->GetCurrentRTV();
	const float clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
	commandList->OMSetRenderTargets(1, &currentRTV, false, nullptr);
	commandList->ClearRenderTargetView(currentRTV, clearColor, 0, nullptr);
}

void Renderer::EndFrame()
{
	auto& frameRes = m_CommandContext->GetFrameCommandResource(m_CurrentFrame);
	auto commandList = frameRes.commandList.Get();
	
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_Swapchain->GetBackBuffer(m_CurrentBufferIndex),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);
	commandList->ResourceBarrier(1, &barrier);

	commandList->Close();

	ID3D12CommandList* ppCommandLists[] = { commandList };

	m_CommandContext->GetCommandQueue()->ExecuteCommandLists(1, ppCommandLists);

	CHECK(m_Swapchain->GetSwapchain()->Present(1, 0));

	//m_Fence->Signal(m_CommandContext->GetCommandQueue(), ++frameRes.fenceValue);
	m_Fence->Signal(m_CommandContext->GetCommandQueue(), ++m_FenceValue);
	frameRes.fenceValue = m_FenceValue;
	
	m_CurrentFrame = (m_CurrentFrame + 1) % m_FramesInFlight;
}

void Renderer::ShutDown()
{
	m_Fence->Flush(m_CommandContext->GetCommandQueue(), m_FenceValue);
}
