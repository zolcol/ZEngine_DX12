#pragma once

#include "Device.h"
#include "Swapchain.h"
#include "CommandContext.h"
#include "Fence.h"
#include "Shader.h"
#include "RootSignature.h"
#include "PipelineState.h"

class Renderer
{
public:
	Renderer() = default;
	~Renderer() = default;

	bool Init(HWND hwnd, int width, int height, uint32_t frameCount);
	
	void BeginFrame();
	void EndFrame();
	void ShutDown();

private:
	std::unique_ptr<Device> m_Device;
	std::unique_ptr<Swapchain> m_Swapchain;
	std::unique_ptr<CommandContext> m_CommandContext;
	std::unique_ptr<Fence> m_Fence;

	std::unique_ptr<RootSignature> m_RootSign;
	std::unique_ptr<Shader> m_VS;
	std::unique_ptr<Shader> m_PS;
	std::unique_ptr<PipelineState> m_PSO;

	uint32_t m_FrameCount;
	int m_FramesInFlight;

	int m_CurrentBufferIndex = 0;
	int m_CurrentFrame = 0;
	uint64_t m_FenceValue = 0;
};
