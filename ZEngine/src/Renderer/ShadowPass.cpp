#include "pch.h"
#include "ShadowPass.h"
#include "DescriptorManager.h"
#include "TextureDepth.h"
#include "CommandContext.h"
#include "Shader.h"
#include "PipelineState.h"
#include "RootSignature.h"
#include <Core/ModelManager.h>
#include "Buffer.h"
#include <Core/Scene.h>
#include <Core/RenderComponent.h>
#include <Core/CoreComponent.h>

uint32_t ShadowPass::GetShadowSRVs(uint32_t currentFrame) const
{
	return m_ShadowMaps[currentFrame]->GetSRVIndex();
}

void ShadowPass::Init(ID3D12Device* device, CommandContext* commandContext, DescriptorManager* descriptorManager, RootSignature* rootSignature, uint32_t frameCount, uint32_t frameWidth, uint32_t frameHeight)
{
	m_FrameWidth = frameWidth;
	m_FrameHeight = frameHeight;
	m_RootSignature = rootSignature;
	m_Device = device;
	m_CommandContext = commandContext;

	InitShadowMapsTexture(device, commandContext, descriptorManager, frameCount);
	InitPSO(device, rootSignature);
}

void ShadowPass::InitConstantBuffer(ID3D12Device* device, DescriptorManager* descriptorManager, uint32_t frameCount)
{
	m_ShadowConstantBuffers.resize(frameCount);
	m_ShadowConstantBufferDatas.resize(frameCount);

	uint32_t bufferSize = (sizeof(ShadowConstantBufferData) + 255) & ~255;


	for (size_t i = 0; i < frameCount; i++)
	{
		m_ShadowConstantBuffers[i] = std::make_unique<Buffer>();
		m_ShadowConstantBuffers[i]->Init(
			device,
			bufferSize,
			D3D12_HEAP_TYPE_UPLOAD
		);

		descriptorManager->SetRootCBVPerFrame(Slot_ShadowCBV, (uint32_t)i, m_ShadowConstantBuffers[i]->GetGpuAddress());
	}
}

void ShadowPass::InitShadowMapsTexture(ID3D12Device* device, CommandContext* commandContext, DescriptorManager* descriptorManager, uint32_t frameCount)
{
	m_ShadowMaps.resize(frameCount);
	for (size_t i = 0; i < frameCount; i++)
	{
		m_ShadowMaps[i] = std::make_unique<TextureDepth>();
		m_ShadowMaps[i]->Init(
			device, commandContext, descriptorManager,
			SHADOW_RESOLUTION, SHADOW_RESOLUTION,
			true
		);
	}
}

void ShadowPass::InitPSO(ID3D12Device* device, RootSignature* rootSignature)
{
	m_VS = std::make_unique<Shader>();
	m_VS->Init(L"src/Renderer/Shaders/Shadow.hlsl", "VSMain", "vs_5_1");

	PipelineConfig config{};
	config.vs = m_VS.get();
	config.numRenderTargets = 0;
	config.depthBias = 1000;
	config.cullMode = D3D12_CULL_MODE_FRONT;
	config.slopeScaledDepthBias = 1.5f;

	m_PSO = std::make_unique<PipelineState>();
	m_PSO->Init(device, *rootSignature, config); 
}


void ShadowPass::UpdateConstantBuffer(ID3D12Device* device, CommandContext* commandContext, Scene* scene, uint32_t currentFrame)
{
	bool foundShadowLight = false;
	scene->GetRegistry().view<TransformComponent, LightComponent>().each([&](const TransformComponent& transform, LightComponent& light)
		{
			if (foundShadowLight || !light.CastShadow) return;

			m_ShadowConstantBufferDatas[currentFrame].PaddingPos = transform.Position;
			
			DirectX::XMFLOAT4X4 viewF = light.GetViewMatrix(transform);
			DirectX::XMFLOAT4X4 projF = light.GetProjectionMatrix();
			DirectX::XMStoreFloat4x4(&m_ShadowConstantBufferDatas[currentFrame].LightViewMatrix, DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&viewF)));
			DirectX::XMStoreFloat4x4(&m_ShadowConstantBufferDatas[currentFrame].LightProjectionMatrix, DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&projF)));
			
			m_ShadowConstantBuffers[currentFrame]->UploadData(
				device,	commandContext,
				&m_ShadowConstantBufferDatas[currentFrame],
				sizeof(ShadowConstantBufferData),
				0,
				D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
			);

			foundShadowLight = true;
		}
	);
}

void ShadowPass::BeginRenderPass(ID3D12GraphicsCommandList* cmdList, uint32_t currentFrame, ModelManager* modelManager, Scene* scene)
{
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_ShadowMaps[currentFrame]->GetResource(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE
		);

	cmdList->ResourceBarrier(1, &barrier);

	// 👉 Thiết lập Viewport và Scissor đúng kích thước Shadow Map (2048x2048)
	CD3DX12_VIEWPORT viewport(0.0f, 0.0f, (float)SHADOW_RESOLUTION, (float)SHADOW_RESOLUTION);
	CD3DX12_RECT scissorRect(0, 0, SHADOW_RESOLUTION, SHADOW_RESOLUTION);
	cmdList->RSSetViewports(1, &viewport);
	cmdList->RSSetScissorRects(1, &scissorRect);

	cmdList->OMSetRenderTargets(0, nullptr, false, &m_ShadowMaps[currentFrame]->GetDSVCpuHandle());
	cmdList->ClearDepthStencilView(m_ShadowMaps[currentFrame]->GetDSVCpuHandle(), D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 0, nullptr);

	// Cấu hình Pipeline State cho Shadow
	cmdList->SetPipelineState(m_PSO->Get());

	// Bind Vertex Buffer
	Buffer* vertexBuffer = modelManager->GetVertexBuffer();
	D3D12_VERTEX_BUFFER_VIEW vbv{};
	vbv.BufferLocation = vertexBuffer->GetGpuAddress();
	vbv.SizeInBytes = vertexBuffer->GetBufferSize();
	vbv.StrideInBytes = sizeof(VertexData);
	cmdList->IASetVertexBuffers(0, 1, &vbv);

	// Bind Index Buffer
	Buffer* indexBuffer = modelManager->GetIndexBuffer();
	D3D12_INDEX_BUFFER_VIEW idv{};
	idv.Format = DXGI_FORMAT_R32_UINT;
	idv.BufferLocation = indexBuffer->GetGpuAddress();
	idv.SizeInBytes = indexBuffer->GetBufferSize();
	cmdList->IASetIndexBuffer(&idv);

	// Update Constant Buffer
	UpdateConstantBuffer(m_Device, m_CommandContext, scene, currentFrame);
}

void ShadowPass::RenderingPass(ID3D12GraphicsCommandList* cmdList, Scene* scene)
{
	scene->GetRegistry().view<MeshComponent, TransformComponent, RenderIndexComponent>().
		each([&](entt::entity entity,const MeshComponent& mesh, const TransformComponent& transform, const RenderIndexComponent& renderID)
		{
			const auto* model = mesh.model;

			if (!model) return;
			if (scene->GetRegistry().any_of<LightComponent>(entity)) return;

			cmdList->SetGraphicsRoot32BitConstants(0, 1, &renderID.renderIndex, 1);

			const auto& meshes = model->GetMeshes();
			for (const auto& mesh : meshes)
			{
				cmdList->DrawIndexedInstanced(mesh.indexCount, 1, mesh.startIndexLocation, mesh.startVertexLocation, 0);
			}
		});
}

void ShadowPass::EndRenderPass(ID3D12GraphicsCommandList* cmdList, uint32_t currentFrame)
{
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_ShadowMaps[currentFrame]->GetResource(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);

	cmdList->ResourceBarrier(1, &barrier);
}
