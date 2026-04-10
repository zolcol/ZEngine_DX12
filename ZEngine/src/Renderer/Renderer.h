#pragma once

#include <memory>
#include <cstdint>

// =====================================================================
// FORWARD DECLARATIONS - Giúp tăng tốc độ biên dịch (Compilation Speed)
// =====================================================================
class Device;
class Swapchain;
class CommandContext;
class Fence;
class RootSignature;
class Shader;
class PipelineState;
class Buffer;
class DescriptorManager;
class Texture2D;
class TextureDepth;

class Renderer
{
public:
	Renderer();
	~Renderer();

	bool Init(HWND hwnd, int width, int height, uint32_t frameCount);
	
	void BeginFrame();
	void EndFrame();
	void ShutDown();

private:
	// --- Infrastructure ---
	std::unique_ptr<Device>         m_Device;
	std::unique_ptr<Swapchain>      m_Swapchain;
	std::unique_ptr<CommandContext> m_CommandContext;
	std::unique_ptr<Fence>          m_Fence;

	// --- Resources ---
	std::unique_ptr<DescriptorManager>		m_DescriptorManager;
	std::unique_ptr<Buffer>					m_VertexBuffer;
	std::vector<std::unique_ptr<Buffer>>	m_ConstantBuffers;
	std::vector<ConstantBufferData>			m_ConstantBuffersData;

	/*ComPtr<ID3D12Resource>			m_DepthBuffer;
	D3D12_CPU_DESCRIPTOR_HANDLE		m_DepthCpuHandle;*/
	std::unique_ptr<TextureDepth>	m_DepthTexture;

	std::unique_ptr<Texture2D>		m_Texture;

	// --- Pipeline ---
	std::unique_ptr<RootSignature>  m_RootSign;
	std::unique_ptr<Shader>         m_VS;
	std::unique_ptr<Shader>         m_PS;
	std::unique_ptr<PipelineState>  m_PSO;

	// --- Window & Frame State ---
	int		 m_Width		      = 0;
	int		 m_Height		      = 0;
	uint32_t m_FrameCount         = 0;
	int      m_FramesInFlight     = 0;
	int      m_CurrentFrame       = 0;
	int      m_CurrentBufferIndex = 0;
	uint64_t m_FenceValue         = 0;

	void InitConstantBuffers();
	void UpdateConstantBuffersData(int currentFrame);

	void InitDepthBuffer();
	void InitTexture2D();
};
