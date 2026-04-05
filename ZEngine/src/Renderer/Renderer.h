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
	// --- Hệ thống Core DX12 ---
	std::unique_ptr<Device>         m_Device;
	std::unique_ptr<Swapchain>      m_Swapchain;
	std::unique_ptr<CommandContext> m_CommandContext;
	std::unique_ptr<Fence>          m_Fence;

	// --- Graphics Pipeline ---
	std::unique_ptr<RootSignature>  m_RootSign;
	std::unique_ptr<Shader>         m_VS;
	std::unique_ptr<Shader>         m_PS;
	std::unique_ptr<PipelineState>  m_PSO;

	// --- Thông số cấu hình & Trạng thái ---
	uint32_t m_FrameCount      = 0;
	int      m_FramesInFlight  = 0;
	int      m_CurrentFrame    = 0;
	int      m_CurrentBufferIndex = 0;
	uint64_t m_FenceValue      = 0;
};
