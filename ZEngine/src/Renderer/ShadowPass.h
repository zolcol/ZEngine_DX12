#pragma once

class DescriptorManager;
class TextureDepth;
class CommandContext;
class PipelineState;
class RootSignature;
class Shader;
class ModelManager;
class Scene;
class Buffer;

class ShadowPass
{
public:
	ShadowPass() = default;
	~ShadowPass() = default;

	uint32_t GetShadowSRVs(uint32_t currentFrame) const;

	void Init(ID3D12Device* device, CommandContext* commandContext, DescriptorManager* descriptorManager, RootSignature* rootSignature, 
		uint32_t frameCount, uint32_t frameWidth, uint32_t frameHeight);

	void BeginRenderPass(ID3D12GraphicsCommandList* cmdList, uint32_t currentFrame, ModelManager* modelManager, Scene* scene);
	void RenderingPass(ID3D12GraphicsCommandList* cmdList, Scene* scene, uint32_t currentFrame);
	void EndRenderPass(ID3D12GraphicsCommandList* cmdList, uint32_t currentFrame);

	void InitConstantBuffer(ID3D12Device* device, DescriptorManager* descriptorManager, uint32_t frameCount);

private:
	const int SHADOW_RESOLUTION = 4096;
	uint32_t m_FrameWidth;
	uint32_t m_FrameHeight;
	RootSignature* m_RootSignature = nullptr;

	std::vector<bool> m_FoundShadowLight;

	ID3D12Device* m_Device;
	CommandContext* m_CommandContext;

	std::vector<std::unique_ptr<TextureDepth>> m_ShadowMaps;
	std::vector<std::unique_ptr<Buffer>>		m_ShadowConstantBuffers;
	std::vector<ShadowConstantBufferData>		m_ShadowConstantBufferDatas;

	std::unique_ptr<Shader>	m_VS;
	std::unique_ptr<PipelineState> m_PSO;

	void InitShadowMapsTexture(ID3D12Device* device, CommandContext* commandContext, DescriptorManager* descriptorManager, uint32_t frameCount);
	void InitPSO(ID3D12Device* device, RootSignature* rootSignature);

	void UpdateConstantBuffer(ID3D12Device* device, CommandContext* commandContext, Scene* scene, uint32_t currentFrame);
};
