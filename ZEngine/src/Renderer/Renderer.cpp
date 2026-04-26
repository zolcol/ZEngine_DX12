#include "pch.h"
#include "Renderer.h"

#include "Device.h"
#include "Swapchain.h"
#include "CommandContext.h"
#include "Fence.h"
#include "Shader.h"
#include "RootSignature.h"
#include "PipelineState.h"
#include "Buffer.h"
#include "DescriptorManager.h"
#include "TextureDepth.h"
#include "Texture2D.h"
#include "ShadowPass.h"

#include "Core/Time.h"
#include "Core/ModelManager.h"
#include "Core/Scene.h"
#include "Core/CoreComponent.h"
#include "Core/RenderComponent.h"
#include "Core/Editor.h"
#include "Core/MipmapManager.h"

// =====================================================================
// CONSTRUCTOR / DESTRUCTOR
// =====================================================================

Renderer::Renderer() = default;
Renderer::~Renderer() = default;

// =====================================================================
// LIFECYCLE
// =====================================================================

bool Renderer::Init(HWND hwnd, int width, int height, uint32_t frameCount)
{
	m_Width = width;
	m_Height = height;
	m_FrameCount = frameCount;
	m_FramesInFlight = frameCount;

	// 1. Core Infrastructure
	m_Device = std::make_unique<Device>();
	if (!m_Device->Init()) return false;

	m_CommandContext = std::make_unique<CommandContext>();
	m_CommandContext->Init(m_Device->GetDevice(), m_FramesInFlight);

	m_Fence = std::make_unique<Fence>();
	if (!m_Fence->Init(m_Device->GetDevice())) return false;

	// 2. Resource Managers
	m_DescriptorManager = std::make_unique<DescriptorManager>();
	m_DescriptorManager->Init(m_Device->GetDevice(), m_FramesInFlight);

	// 3. Presentation (Swapchain)
	SwapChainConfig swConfig{};
	swConfig.windowHandle = hwnd;
	swConfig.factory = m_Device->GetFactory();
	swConfig.commandQueue = m_CommandContext->GetCommandQueue();
	swConfig.width = width;
	swConfig.height = height;
	swConfig.frameCount = m_FrameCount;

	m_Swapchain = std::make_unique<Swapchain>();
	m_Swapchain->Init(swConfig, m_Device->GetDevice(), m_DescriptorManager.get());

	// 4. Global Resources & Render Passes
	InitDepthBuffer();
	InitConstantBuffers();
	InitObjectDataBuffers();
	InitLightBuffers();

	m_ShadowPass = std::make_unique<ShadowPass>();
	m_ShadowPass->InitConstantBuffer(m_Device->GetDevice(), m_DescriptorManager.get(), m_FramesInFlight);

	// 5. Pipeline State
	m_RootSign = std::make_unique<RootSignature>();
	m_RootSign->Init(m_Device->GetDevice(), m_DescriptorManager.get());

	m_VS = std::make_unique<Shader>();
	m_VS->Init(L"src/Renderer/Shaders/shader.hlsl", "VSMain", "vs_5_1");

	m_PS = std::make_unique<Shader>();
	m_PS->Init(L"src/Renderer/Shaders/shader.hlsl", "PSMain", "ps_5_1");

	PipelineConfig psoConfig{};
	psoConfig.vs = m_VS.get();
	psoConfig.ps = m_PS.get();
	psoConfig.numRenderTargets = 1;
	psoConfig.rtvFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	m_PSO = std::make_unique<PipelineState>();
	m_PSO->Init(m_Device->GetDevice(), *m_RootSign, psoConfig);

	m_ShadowPass->Init(m_Device->GetDevice(), m_CommandContext.get(), m_DescriptorManager.get(), m_RootSign.get(), m_FramesInFlight, m_Width, m_Height);

	m_MipmapManager = std::make_unique<MipmapManager>();
	m_MipmapManager->Init(m_Device->GetDevice(), m_DescriptorManager.get());

	m_ModelManager = std::make_unique<ModelManager>();
	m_ModelManager->Init(m_Device->GetDevice(), m_CommandContext.get(), m_DescriptorManager.get());

	ENGINE_INFO("Renderer initialized successfully.");
	return true;
}

void Renderer::ShutDown()
{
	if (m_CommandContext && m_Fence)
	{
		m_Fence->Flush(m_CommandContext->GetCommandQueue(), m_FenceValue);
	}
}

// =====================================================================
// MAIN RENDER LOOP
// =====================================================================

void Renderer::BeginFrame(Scene* scene)
{
	m_Swapchain->WaitForLatencyWaitableObject();
	m_CurrentBufferIndex = m_Swapchain->GetCurrentBackBufferIndex();
	
	auto& frameRes = m_CommandContext->GetFrameCommandResource(m_CurrentFrame);
	auto commandList = frameRes.commandList.Get();

	// 1. Synchronization & Reset
	m_Fence->Wait(frameRes.fenceValue);
	frameRes.commandAllocator->Reset();
	commandList->Reset(frameRes.commandAllocator.Get(), nullptr);

	// 2. Data Updates (CPU -> GPU)
	UpdateConstantBuffersData(m_CurrentFrame, scene);
	UpdateObjectDatas(m_CurrentFrame, scene);
	UpdateLightBuffers(m_CurrentFrame, scene);

	// 3. Global Bindings
	commandList->SetGraphicsRootSignature(m_RootSign->Get());
	m_DescriptorManager->BindDescriptors(commandList, m_CurrentFrame);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set Root Constants (Slot 0)
	commandList->SetGraphicsRoot32BitConstants(Slot_RootConstants, 1, &m_FrameLightCount, 2);

	// 4. Shadow Pass
	m_ShadowPass->BeginRenderPass(commandList, m_CurrentFrame, m_ModelManager.get(), scene);
	m_ShadowPass->RenderingPass(commandList, scene, m_CurrentFrame);
	m_ShadowPass->EndRenderPass(commandList, m_CurrentFrame);

	// 5. Main Pass Transition
	m_Swapchain->Transition(commandList, m_CurrentBufferIndex, D3D12_RESOURCE_STATE_RENDER_TARGET);

	// 6. Setup Render Targets & Clear
	const float clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
	commandList->OMSetRenderTargets(1, &m_Swapchain->GetCurrentRTVCpuHandle(), false, &m_DepthTexture->GetDSVCpuHandle());
	commandList->ClearRenderTargetView(m_Swapchain->GetCurrentRTVCpuHandle(), clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(m_DepthTexture->GetDSVCpuHandle(), D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 0, nullptr);

	// 7. Viewport & Scissor
	CD3DX12_VIEWPORT viewport(0.0f, 0.0f, (float)m_Width, (float)m_Height);
	CD3DX12_RECT scissorRect(0, 0, m_Width, m_Height);
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);
	
	// 8. Draw Scene
	commandList->SetPipelineState(m_PSO->Get());

	// Bind Mesh Buffers
	D3D12_VERTEX_BUFFER_VIEW vbv{};
	vbv.BufferLocation = m_ModelManager->GetVertexBuffer()->GetGpuAddress();
	vbv.SizeInBytes = m_ModelManager->GetVertexBuffer()->GetBufferSize();
	vbv.StrideInBytes = sizeof(VertexData);
	commandList->IASetVertexBuffers(0, 1, &vbv);

	D3D12_INDEX_BUFFER_VIEW idv{};
	idv.Format = DXGI_FORMAT_R32_UINT;
	idv.BufferLocation = m_ModelManager->GetIndexBuffer()->GetGpuAddress();
	idv.SizeInBytes = m_ModelManager->GetIndexBuffer()->GetBufferSize();
	commandList->IASetIndexBuffer(&idv);

	if (!m_ModelManager->IsMaterialUpdated()) return;

	scene->GetRegistry().view<MeshComponent, TransformComponent, RenderIndexComponent>().each([&](const MeshComponent& meshComp, const TransformComponent& transform, const RenderIndexComponent& renderID)
	{
		if (!meshComp.model) return;

		commandList->SetGraphicsRoot32BitConstants(Slot_RootConstants, 1, &renderID.renderIndex, 1);

		for (const auto& mesh : meshComp.model->GetMeshes())
		{
			commandList->SetGraphicsRoot32BitConstants(Slot_RootConstants, 1, &mesh.materialIndex, 0);
			commandList->DrawIndexedInstanced(mesh.indexCount, 1, mesh.startIndexLocation, mesh.startVertexLocation, 0);
		}
	});
}

void Renderer::EndFrame(Scene* scene, Editor* editor)
{
	auto& frameRes = m_CommandContext->GetFrameCommandResource(m_CurrentFrame);
	auto commandList = frameRes.commandList.Get();

	// 1. Editor UI
	editor->Render(commandList);

	// Multi-Viewport support
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault(nullptr, (void*)commandList);
	}

	// 2. Final Transition & Close
	m_Swapchain->Transition(commandList, m_CurrentBufferIndex, D3D12_RESOURCE_STATE_PRESENT);
	commandList->Close();

	// 3. Execute
	ID3D12CommandList* ppCommandLists[] = { commandList };
	m_CommandContext->GetCommandQueue()->ExecuteCommandLists(1, ppCommandLists);

	// 4. Present
	CHECK(m_Swapchain->GetSwapchain()->Present(1, 0));

	// 5. Signal next frame
	m_Fence->Signal(m_CommandContext->GetCommandQueue(), ++m_FenceValue);
	frameRes.fenceValue = m_FenceValue;

	m_CurrentFrame = (m_CurrentFrame + 1) % m_FramesInFlight;
}

// =====================================================================
// INTERNAL INITIALIZATION
// =====================================================================

void Renderer::InitDepthBuffer()
{
	m_DepthTexture = std::make_unique<TextureDepth>();
	m_DepthTexture->Init(m_Device->GetDevice(), m_CommandContext.get(), m_DescriptorManager.get(), m_Width, m_Height);
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
		m_DescriptorManager->SetRootCBVPerFrame(Slot_GlobalCBV, (uint32_t)i, m_ConstantBuffers[i]->GetGpuAddress());
	}
}

void Renderer::InitObjectDataBuffers()
{
	m_ObjectDataBuffers.resize(m_FramesInFlight);
	m_ObjectDatas.resize(m_FramesInFlight);

	for (size_t i = 0; i < m_FramesInFlight; i++)
	{
		m_ObjectDatas[i].reserve(m_MaxObjectDatas);
		m_ObjectDataBuffers[i] = std::make_unique<Buffer>();
		m_ObjectDataBuffers[i]->Init(m_Device->GetDevice(), sizeof(ObjectData) * m_MaxObjectDatas, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
		m_DescriptorManager->SetRootSRVPerFrame(Slot_ObjectSRV, (uint32_t)i, m_ObjectDataBuffers[i]->GetGpuAddress());
	}
}

void Renderer::InitLightBuffers()
{
	m_GpuLightDatas.resize(m_FramesInFlight);
	m_LightDataBuffers.resize(m_FramesInFlight);

	for (size_t i = 0; i < m_FramesInFlight; i++)
	{
		m_GpuLightDatas[i].resize(m_MaxLightObjects);
		m_LightDataBuffers[i] = std::make_unique<Buffer>();
		m_LightDataBuffers[i]->Init(m_Device->GetDevice(), sizeof(GPULightData) * m_MaxLightObjects, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
		m_DescriptorManager->SetRootSRVPerFrame(Slot_LightSRV, (uint32_t)i, m_LightDataBuffers[i]->GetGpuAddress());
	}
}

// =====================================================================
// FRAME DATA UPDATES
// =====================================================================

void Renderer::UpdateConstantBuffersData(int currentFrame, Scene* scene)
{
	XMFLOAT3 cameraPos = { 0, 0, 0 };
	XMMATRIX viewMatrix = XMMatrixIdentity();
	XMMATRIX projMatrix = XMMatrixPerspectiveFovLH(XM_PIDIV4, (float)m_Width / m_Height, 0.1f, 1000.0f);
	bool hasCamera = false;

	scene->GetRegistry().view<TransformComponent, CameraComponent>().each([&](TransformComponent& transform, CameraComponent& camera)
	{
		if (camera.IsPrimary && !hasCamera)
		{
			cameraPos = transform.Position;
			viewMatrix = camera.GetViewMatrix(transform);
			projMatrix = camera.GetProjectionMatrix((float)m_Width / m_Height);
			hasCamera = true;
		}
	});

	EnvironmentComponent& envComponent = scene->GetRegistry().ctx().get<EnvironmentComponent>();
	m_ConstantBuffersData[currentFrame].BrdfLutSRVIndex = envComponent.BrdfLutSRVIndex;
	m_ConstantBuffersData[currentFrame].IrradianceSRVIndex = envComponent.IrradianceSRVIndex;
	m_ConstantBuffersData[currentFrame].PrefilteredSRVIndex = envComponent.PrefilteredSRVIndex;
	m_ConstantBuffersData[currentFrame].SkyboxSRVIndex = envComponent.SkyboxSRVIndex;
	m_ConstantBuffersData[currentFrame].IBLIntensity = envComponent.IBLIntensity;

	m_ConstantBuffersData[currentFrame].CameraPos = cameraPos;
	XMStoreFloat4x4(&m_ConstantBuffersData[currentFrame].ViewMatrix, XMMatrixTranspose(viewMatrix));
	XMStoreFloat4x4(&m_ConstantBuffersData[currentFrame].ProjectionMatrix, XMMatrixTranspose(projMatrix));

	m_ConstantBuffers[currentFrame]->UploadData(m_Device->GetDevice(), m_CommandContext.get(), &m_ConstantBuffersData[currentFrame], sizeof(ConstantBufferData), 0, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
}

void Renderer::UpdateObjectDatas(int currentFrame, Scene* scene)
{
	scene->GetRegistry().view<RenderIndexComponent, TransformComponent>().each([&](const RenderIndexComponent& renderID, const TransformComponent& transform)
	{
		m_ObjectDatas[currentFrame][renderID.renderIndex].WorldTransform = XMMatrixTranspose(transform.GetWorldMatrix());
	});

	m_ObjectDataBuffers[currentFrame]->UploadData(m_Device->GetDevice(), m_CommandContext.get(), m_ObjectDatas[currentFrame].data(), m_ObjectDatas[currentFrame].size() * sizeof(ObjectData), 0, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
}

void Renderer::UpdateLightBuffers(int currentFrame, Scene* scene)
{
	m_FrameLightCount = 0;
	bool shadowCastFound = false;

	scene->GetRegistry().view<LightComponent, TransformComponent>().each([&](const LightComponent& light, const TransformComponent& transform)
	{
		int shadowIdx = (!shadowCastFound && light.CastShadow) ? m_ShadowPass->GetShadowSRVs(currentFrame) : -1;
		m_GpuLightDatas[currentFrame][m_FrameLightCount] = GPULightData(light, transform, shadowIdx);
		
		m_FrameLightCount++;
		if (light.CastShadow) shadowCastFound = true;
	});

	m_LightDataBuffers[currentFrame]->UploadData(m_Device->GetDevice(), m_CommandContext.get(), m_GpuLightDatas[currentFrame].data(), m_FrameLightCount * sizeof(GPULightData), 0, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

// =====================================================================
// GETTERS & SYSTEM CONNECT
// =====================================================================

void Renderer::ConnectToScene(entt::registry& registry)
{
	registry.on_construct<RenderIndexComponent>().connect<&Renderer::OnRenderIndexCreated>(this);
}

void Renderer::OnRenderIndexCreated(entt::registry& registry, entt::entity entity)
{
	RenderIndexComponent& comp = registry.get<RenderIndexComponent>(entity);
	comp.renderIndex = m_CurrentRenderIndex++;

	for (size_t i = 0; i < m_FramesInFlight; i++)
	{
		m_ObjectDatas[i].push_back(ObjectData());
	}
}
