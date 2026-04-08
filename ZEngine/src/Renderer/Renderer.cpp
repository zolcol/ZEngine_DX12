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
#include "Buffer.h"
#include "DescriptorManager.h"
#include <Core/Time.h>


Renderer::Renderer() = default;
Renderer::~Renderer() = default;

bool Renderer::Init(HWND hwnd, int width, int height, uint32_t frameCount)
{
	m_Width = width;
	m_Height = height;
	m_FrameCount = frameCount;
	m_FramesInFlight = frameCount;

	// 1. Hệ thống Core (Device, Command, Sync)
	m_Device = std::make_unique<Device>();
	if (!m_Device->Init()) return false;

	m_CommandContext = std::make_unique<CommandContext>();
	m_CommandContext->Init(m_Device->GetDevice(), m_FramesInFlight);

	m_Fence = std::make_unique<Fence>();
	if (!m_Fence->Init(m_Device->GetDevice())) return false;

	// 2. Cửa sổ hiển thị (Swapchain)
	SwapChainConfig config{};
	config.windowHandle = hwnd;
	config.factory = m_Device->GetFactory();
	config.commandQueue = m_CommandContext->GetCommandQueue();
	config.width = width;
	config.height = height;
	config.frameCount = m_FrameCount;

	m_Swapchain = std::make_unique<Swapchain>();
	m_Swapchain->Init(config, m_Device->GetDevice());

	// Khoi tao Descriptor Manager
	m_DescriptorManager = std::make_unique<DescriptorManager>();
	m_DescriptorManager->Init(m_Device->GetDevice());

	// 3. Khởi tạo tài nguyên (Geometry)
	std::vector<VertexData> vertices =
	{
		{ XMFLOAT3(0.0f, 0.5f, 0.0f),  XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) }, 
		{ XMFLOAT3(0.5f, -0.5f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) }, 
		{ XMFLOAT3(-0.5f, -0.5f, 0.0f),XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }  
	};

	uint32_t bufferSize = static_cast<uint32_t>(sizeof(VertexData) * vertices.size());
	m_VertexBuffer = std::make_unique<Buffer>();
	
	if (!m_VertexBuffer->Init(m_Device->GetDevice(), bufferSize, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON))
		return false;

	m_VertexBuffer->UploadData(m_Device->GetDevice(), m_CommandContext.get(), vertices.data(), bufferSize, 0, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	// Khởi tạo Constant Buffer.
	InitConstantBuffers();

	// 4. Graphics Pipeline
	m_RootSign = std::make_unique<RootSignature>();
	m_RootSign->Init(m_Device->GetDevice());

	m_VS = std::make_unique<Shader>();
	m_VS->Init(L"src/Renderer/Shaders/shader.hlsl", "VSMain", "vs_5_1");

	m_PS = std::make_unique<Shader>();
	m_PS->Init(L"src/Renderer/Shaders/shader.hlsl", "PSMain", "ps_5_1");

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

	// Đợi GPU hoàn thành frame tương ứng trước đó
 	m_Fence->Wait(frameRes.fenceValue);

	frameRes.commandAllocator->Reset();
	commandList->Reset(frameRes.commandAllocator.Get(), nullptr);

	// Chỉ định Render Target
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_Swapchain->GetBackBuffer(m_CurrentBufferIndex),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	commandList->ResourceBarrier(1, &barrier);

	D3D12_CPU_DESCRIPTOR_HANDLE currentRTV = m_Swapchain->GetCurrentRTV();
	const float clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f }; // Màu nền tối hơn
	commandList->OMSetRenderTargets(1, &currentRTV, false, nullptr);
	commandList->ClearRenderTargetView(currentRTV, clearColor, 0, nullptr);

	// Cấu hình Pipeline State
	CD3DX12_VIEWPORT viewport(0.0f, 0.0f, (float)m_Width, (float)m_Height);
	CD3DX12_RECT scissorRect(0, 0, m_Width, m_Height);
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);

	commandList->SetGraphicsRootSignature(m_RootSign->Get());
	commandList->SetPipelineState(m_PSO->Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Bind Descriptor
	m_DescriptorManager->BindDescriptorHeap(commandList);
	commandList->SetGraphicsRootConstantBufferView(0, m_ConstantBuffers[m_CurrentFrame]->GetGpuAddress());

	// Bind Vertex Buffer
	D3D12_VERTEX_BUFFER_VIEW vbv{};
	vbv.BufferLocation = m_VertexBuffer->GetGpuAddress();
	vbv.SizeInBytes = m_VertexBuffer->GetBufferSize();
	vbv.StrideInBytes = sizeof(VertexData);
	commandList->IASetVertexBuffers(0, 1, &vbv);
	
	//Update ConstantBuffer 
	UpdateConstantBuffesData(m_CurrentFrame);

	commandList->DrawInstanced(3, 1, 0, 0);
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

void Renderer::InitConstantBuffers()
{
	m_ConstantBuffersData.resize(m_FramesInFlight);
	m_ConstantBuffers.resize(m_FramesInFlight);

	uint32_t bufferSize = (sizeof(ConstantBufferData) + 255) & ~255;
	for (size_t i = 0; i < m_FramesInFlight; i++)
	{
		m_ConstantBuffers[i] = std::make_unique<Buffer>();
		m_ConstantBuffers[i]->Init(m_Device->GetDevice(), bufferSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
	}
}

void Renderer::UpdateConstantBuffesData(int currentFrame)
{
	float time = Time::GetTotalTime();

	// 🔹 World (xoay object)
	XMMATRIX world = XMMatrixRotationY(time);

	// 🔹 Camera
	XMVECTOR eye = XMVectorSet(0.0f, 0.0f, -2.0f, 1.0f);
	XMVECTOR target = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(eye, target, up);

	// 🔹 Projection
	float aspect = (float)m_Width / (float)m_Height;

	XMMATRIX proj = XMMatrixPerspectiveFovLH(
		XM_PIDIV4,
		aspect,
		0.1f,
		100.0f
	);

	// 🔥 Ghi vào constant buffer (NHỚ transpose)
	XMStoreFloat4x4(&m_ConstantBuffersData[currentFrame].WorldMatrix, XMMatrixTranspose(world));
	XMStoreFloat4x4(&m_ConstantBuffersData[currentFrame].ViewMatrix, XMMatrixTranspose(view));
	XMStoreFloat4x4(&m_ConstantBuffersData[currentFrame].ProjectionMatrix, XMMatrixTranspose(proj));


	// Upload To Constant Buffer
	m_ConstantBuffers[currentFrame]->UploadData(
		m_Device->GetDevice(), m_CommandContext.get(),
		&m_ConstantBuffersData[currentFrame],
		sizeof(m_ConstantBuffersData[currentFrame]), 0,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
	);
}
