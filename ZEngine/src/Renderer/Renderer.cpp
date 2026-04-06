#include "pch.h"
#include "Renderer.h"

// --- Include các Header thực tế để sử dụng trong file .cpp ---
#include "Device.h"
#include "Swapchain.h"
#include "CommandContext.h"
#include "Fence.h"
#include "Shader.h"
#include "RootSignature.h"
#include "PipelineState.h"
#include "VertexBuffer.h"

Renderer::Renderer() = default;

// Bắt buộc phải định nghĩa Destructor ở đây khi dùng unique_ptr với Forward Declaration
Renderer::~Renderer() = default;

bool Renderer::Init(HWND hwnd, int width, int height, uint32_t frameCount)
{
	m_Width = width;
	m_Height = height;
	m_FrameCount = frameCount;
	m_FramesInFlight = frameCount;

	// 1. Khởi tạo Device & Command System
	m_Device = std::make_unique<Device>();
	if (!m_Device->Init()) return false;

	m_CommandContext = std::make_unique<CommandContext>();
	m_CommandContext->Init(m_Device->GetDevice(), m_FramesInFlight);

	// 2. Khởi tạo Đồng bộ hóa (Fence)
	m_Fence = std::make_unique<Fence>();
	if (!m_Fence->Init(m_Device->GetDevice())) return false;

	// 3. Khởi tạo Swapchain
	SwapChainConfig config{};
	config.windowHandle = hwnd;
	config.factory = m_Device->GetFactory();
	config.commandQueue = m_CommandContext->GetCommandQueue();
	config.width = width;
	config.height = height;
	config.frameCount = m_FrameCount;

	m_Swapchain = std::make_unique<Swapchain>();
	m_Swapchain->Init(config, m_Device->GetDevice());

	// Khởi tạo VertexBuffer
	std::vector<VertexData> vertices =
	{
		{ XMFLOAT3(0.0f, 0.5f, 0.0f),  XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) }, // Đỉnh trên (đỏ)
		{ XMFLOAT3(0.5f, -0.5f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) }, // Đỉnh phải (xanh lá)
		{ XMFLOAT3(-0.5f, -0.5f, 0.0f),XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }  // Đỉnh trái (xanh dương)
	};

	m_VertexBuffer = std::make_unique<VertexBuffer>();
	m_VertexBuffer->Init(m_Device->GetDevice(), m_CommandContext.get(), vertices);

	// 4. Khởi tạo Graphics Pipeline (Shaders, RootSign, PSO)
	m_RootSign = std::make_unique<RootSignature>();
	m_RootSign->Init(m_Device->GetDevice());

	m_VS = std::make_unique<Shader>();
	m_VS->Init(L"src/Renderer/Shaders/shader.hlsl", "VSMain", "vs_5_0");

	m_PS = std::make_unique<Shader>();
	m_PS->Init(L"src/Renderer/Shaders/shader.hlsl", "PSMain", "ps_5_0");

	m_PSO = std::make_unique<PipelineState>();
	m_PSO->Init(m_Device->GetDevice(), *m_RootSign, *m_VS, *m_PS);

	ENGINE_INFO("Renderer initialized successfully.");
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

	// Chuyển trạng thái buffer sang RENDER_TARGET để vẽ
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_Swapchain->GetBackBuffer(m_CurrentBufferIndex),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	commandList->ResourceBarrier(1, &barrier);

	// Cấu hình Viewport & Render Target
	D3D12_CPU_DESCRIPTOR_HANDLE currentRTV = m_Swapchain->GetCurrentRTV();
	const float clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
	commandList->OMSetRenderTargets(1, &currentRTV, false, nullptr);
	commandList->ClearRenderTargetView(currentRTV, clearColor, 0, nullptr);

	// Thiết lập viewport và scissor
	CD3DX12_VIEWPORT viewport(0.0f, 0.0f, (float)m_Width, (float)m_Height);
	CD3DX12_RECT scissorRect(0, 0, m_Width, m_Height);

	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);

	// Thiết lập Rootsign, PSO
	commandList->SetGraphicsRootSignature(m_RootSign->Get());
	commandList->SetPipelineState(m_PSO->Get());

	// Thiết lập Topology
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Thiết lập Vertex
	commandList->IASetVertexBuffers(0, 1, &m_VertexBuffer->GetView());
	
	// Vẽ
	commandList->DrawInstanced(m_VertexBuffer->GetVertexCount(), 1, 0, 0);
}

void Renderer::EndFrame()
{
	auto& frameRes = m_CommandContext->GetFrameCommandResource(m_CurrentFrame);
	auto commandList = frameRes.commandList.Get();
	
	// Chuyển trạng thái buffer sang PRESENT để hiển thị
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_Swapchain->GetBackBuffer(m_CurrentBufferIndex),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);
	commandList->ResourceBarrier(1, &barrier);

	commandList->Close();

	// Thực thi các lệnh đã record
	ID3D12CommandList* ppCommandLists[] = { commandList };
	m_CommandContext->GetCommandQueue()->ExecuteCommandLists(1, ppCommandLists);

	// Hiển thị frame
	CHECK(m_Swapchain->GetSwapchain()->Present(1, 0));

	// Đồng bộ hóa Frame tiếp theo
	m_Fence->Signal(m_CommandContext->GetCommandQueue(), ++m_FenceValue);
	frameRes.fenceValue = m_FenceValue;
	
	m_CurrentFrame = (m_CurrentFrame + 1) % m_FramesInFlight;
}

void Renderer::ShutDown()
{
	// Đảm bảo GPU hoàn thành mọi công việc trước khi tắt
	if (m_CommandContext && m_Fence)
	{
		m_Fence->Flush(m_CommandContext->GetCommandQueue(), m_FenceValue);
	}
}
