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
#include "Core/ImageLoader.h"
#include "Texture2D.h"
#include "TextureRenderTarget.h"
#include "TextureDepth.h"
#include "Core/ModelManager.h"
#include "Core/Model.h"
#include "Core/Scene.h"
#include "Core/Entity.h"
#include "Core/CoreComponent.h"
#include "Core/RenderComponent.h"




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

	// Khoi tao Descriptor Manager
	m_DescriptorManager = std::make_unique<DescriptorManager>();
	m_DescriptorManager->Init(m_Device->GetDevice(), m_FramesInFlight);

	// Khoi tao Model Manager
	m_ModelManager = std::make_unique<ModelManager>();
	m_ModelManager->Init(m_Device->GetDevice(), m_CommandContext.get(), m_DescriptorManager.get());

	// 2. Cửa sổ hiển thị (Swapchain)
	SwapChainConfig config{};
	config.windowHandle = hwnd;
	config.factory = m_Device->GetFactory();
	config.commandQueue = m_CommandContext->GetCommandQueue();
	config.width = width;
	config.height = height;
	config.frameCount = m_FrameCount;

	m_Swapchain = std::make_unique<Swapchain>();
	m_Swapchain->Init(config, m_Device->GetDevice(), m_DescriptorManager.get());

	// Khởi tạo Constant Buffer (Đăng ký Root CBV trước)
	InitDepthBuffer();
	InitRootConstants();
	InitConstantBuffers();
	InitObjectDataBuffers();

	// Chốt cấu trúc Descriptor Tables (Bindless) sau khi đã có hết các Root CBV
	m_DescriptorManager->SetupStandardDescriptorTables();

	// 4. Graphics Pipeline
	m_RootSign = std::make_unique<RootSignature>();
	m_RootSign->Init(m_Device->GetDevice(), m_DescriptorManager.get());

	m_VS = std::make_unique<Shader>();
	m_VS->Init(L"src/Renderer/Shaders/shader.hlsl", "VSMain", "vs_5_1");

	m_PS = std::make_unique<Shader>();
	m_PS->Init(L"src/Renderer/Shaders/shader.hlsl", "PSMain", "ps_5_1");

	m_PSO = std::make_unique<PipelineState>();
	m_PSO->Init(m_Device->GetDevice(), *m_RootSign, *m_VS, *m_PS);

	ENGINE_INFO("Renderer initialized successfully.");
	return true;
}

void Renderer::BeginFrame(Scene* scene)
{
	m_Swapchain->WaitForLatencyWaitableObject();

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
	
	const float clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f }; // Màu nền tối hơn

	commandList->OMSetRenderTargets(1, &m_Swapchain->GetCurrentRTVCpuHandle(), false, &m_DepthTexture->GetDSVCpuHandle());
	commandList->ClearRenderTargetView(m_Swapchain->GetCurrentRTVCpuHandle(), clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(m_DepthTexture->GetDSVCpuHandle(), D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);

	// Cấu hình Pipeline State
	CD3DX12_VIEWPORT viewport(0.0f, 0.0f, (float)m_Width, (float)m_Height);
	CD3DX12_RECT scissorRect(0, 0, m_Width, m_Height);
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);

	commandList->SetGraphicsRootSignature(m_RootSign->Get());
	commandList->SetPipelineState(m_PSO->Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Bind Descriptor
	m_DescriptorManager->BindDescriptors(commandList, m_CurrentFrame);

	// Bind Vertex Buffer
	Buffer* vertexBuffer = m_ModelManager->GetVertexBuffer();
	D3D12_VERTEX_BUFFER_VIEW vbv{};
	vbv.BufferLocation = vertexBuffer->GetGpuAddress();
	vbv.SizeInBytes = vertexBuffer->GetBufferSize();
	vbv.StrideInBytes = sizeof(VertexData);
	commandList->IASetVertexBuffers(0, 1, &vbv);

	// Bind Index Buffer
	Buffer* indexBuffer = m_ModelManager->GetIndexBuffer();
	D3D12_INDEX_BUFFER_VIEW idv{};
	idv.Format = DXGI_FORMAT_R32_UINT;
	idv.BufferLocation = indexBuffer->GetGpuAddress();
	idv.SizeInBytes = indexBuffer->GetBufferSize();
	commandList->IASetIndexBuffer(&idv);

	// Draw Model
	if (!m_ModelManager->IsMaterialUpdated())
	{
		ENGINE_FATAL("Material Data Not Update to GPU!!!");
		return;
	}
	UpdateConstantBuffersData(m_CurrentFrame);
	UpdateObjectDatas(m_CurrentFrame, scene);
	scene->GetRegistry().view<MeshComponent, TransformComponent, RenderIndexComponent>().each([&](const MeshComponent& mesh, const TransformComponent& transform, const RenderIndexComponent& renderID) 
		{
			const auto* model = mesh.model;

			if (!model) return; // Skip if model failed to load

			commandList->SetGraphicsRoot32BitConstants(0, 1, &renderID.renderIndex, 1);

			const auto& meshes = model->GetMeshes();
			for (const auto& mesh : meshes)
			{
				uint32_t materialIndex = mesh.materialIndex;
				commandList->SetGraphicsRoot32BitConstants(0, 1, &materialIndex, 0);
				commandList->DrawIndexedInstanced(mesh.indexCount, 1, mesh.startIndexLocation, mesh.startVertexLocation, 0);
			}
		});
}

void Renderer::EndFrame(Scene* scene)
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

void Renderer::ConnnetToScene(entt::registry& registry)
{
	registry.on_construct<RenderIndexComponent>().connect<&Renderer::OnRenderIndexCreated>(this);
}

void Renderer::InitRootConstants()
{
	m_DescriptorManager->CreateRootConstants(2, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
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

	m_DescriptorManager->CreateRootCBVPerFrame(
		m_ConstantBuffers,
		0, 2,
		D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
		D3D12_SHADER_VISIBILITY_ALL
	);
}

void Renderer::UpdateConstantBuffersData(int currentFrame)
{
	// 🔹 Camera
	XMFLOAT3 cameraPos = XMFLOAT3(0, 1, -3);
	XMVECTOR eye = XMVectorSet(cameraPos.x, cameraPos.y, cameraPos.z, 1.0f);
	XMVECTOR target = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
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
	m_ConstantBuffersData[currentFrame].CameraPos = cameraPos;
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

void Renderer::InitDepthBuffer()
{
	m_DepthTexture = std::make_unique<TextureDepth>();
	m_DepthTexture->Init(m_Device->GetDevice(), m_CommandContext.get(), m_DescriptorManager.get(),
		m_Width, m_Height
	);
}

void Renderer::InitObjectDataBuffers()
{
	m_ObjectDataBuffers.resize(m_FramesInFlight);
	m_ObjectDatas.resize(m_FramesInFlight);

	for (size_t i = 0; i < m_FramesInFlight; i++)
	{
		m_ObjectDatas[i].reserve(MAX_OBJECT_DATAS);

		m_ObjectDataBuffers[i] = std::make_unique<Buffer>();
		m_ObjectDataBuffers[i]->Init(m_Device->GetDevice(), sizeof(ObjectData) * MAX_OBJECT_DATAS, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_COMMON);
	}
	
	m_DescriptorManager->CreateRootSRVPerFrame(m_ObjectDataBuffers, 1, 2, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
}

void Renderer::UpdateObjectDatas(int currentFrame, Scene* scene)
{
	scene->GetRegistry().view<RenderIndexComponent, TransformComponent>().each([&](const RenderIndexComponent& renderIDComp, const TransformComponent& transform)
		{
			m_ObjectDatas[currentFrame][renderIDComp.renderIndex].WorldTransform = XMMatrixTranspose(transform.GetWorldMatrix());
		});

	m_ObjectDataBuffers[currentFrame]->UploadData(m_Device->GetDevice(), m_CommandContext.get(),
		m_ObjectDatas[currentFrame].data(),
		m_ObjectDatas[currentFrame].size() * sizeof(ObjectData),
		0,
		D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
	);
}

void Renderer::OnRenderIndexCreated(entt::registry& registry, entt::entity entity)
{
	RenderIndexComponent& comp = registry.get<RenderIndexComponent>(entity);

	comp.renderIndex = m_CurrentRenderIndex;
	m_CurrentRenderIndex += 1;

	for (size_t i = 0; i < m_FramesInFlight; i++)
	{
		m_ObjectDatas[i].push_back(ObjectData());
	}
	
}

