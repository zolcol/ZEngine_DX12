#pragma once

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
class ModelManager;
class Scene;
class Editor;
struct GPULightData;
class ShadowPass;
class MipmapManager;

class Renderer
{
public:
	Renderer();
	~Renderer();

	// --- Lifecycle ---
	bool Init(HWND hwnd, int width, int height, uint32_t frameCount);
	void ShutDown();

	// --- Main Render Loop ---
	void BeginFrame(Scene* scene);
	void EndFrame(Scene* scene, Editor* editor);

	// --- Getters & System Connect ---
	Device*       GetDevice()       const { return m_Device.get(); }
	CommandContext* GetCommandContext() const { return m_CommandContext.get(); }
	DescriptorManager* GetDescriptorManager() const { return m_DescriptorManager.get(); }
	ModelManager* GetModelManager() const { return m_ModelManager.get(); }
	void ConnectToScene(entt::registry& registry);

private:
	// --- Internal Initialization ---
	void InitDepthBuffer();
	void InitConstantBuffers();
	void InitObjectDataBuffers();
	void InitLightBuffers();

	// --- Frame Data Updates ---
	void UpdateConstantBuffersData(int currentFrame, Scene* scene);
	void UpdateObjectDatas(int currentFrame, Scene* scene);
	void UpdateLightBuffers(int currentFrame, Scene* scene);

	// --- Callbacks ---
	void OnRenderIndexCreated(entt::registry& registry, entt::entity entity);

private:
	// --- Layer 1: Infrastructure (Hạ tầng core) ---
	std::unique_ptr<Device>         m_Device;
	std::unique_ptr<Swapchain>      m_Swapchain;
	std::unique_ptr<CommandContext> m_CommandContext;
	std::unique_ptr<Fence>          m_Fence;

	// --- Layer 2: Managers (Quản lý tài nguyên) ---
	std::unique_ptr<DescriptorManager> m_DescriptorManager;
	std::unique_ptr<ModelManager>      m_ModelManager;

	// --- Layer 3: Pipeline (Luồng xử lý đồ họa) ---
	std::unique_ptr<RootSignature>  m_RootSign;
	std::unique_ptr<Shader>         m_VS;
	std::unique_ptr<Shader>         m_PS;
	std::unique_ptr<PipelineState>  m_PSO;

	// --- Layer 4: Render Passes ---
	std::unique_ptr<ShadowPass>     m_ShadowPass;

	std::unique_ptr<MipmapManager>  m_MipmapManager;

	// --- Layer 5: Resources (Dữ liệu thực tế) ---
	std::unique_ptr<TextureDepth>             m_DepthTexture;
	
	std::vector<std::unique_ptr<Buffer>>      m_ConstantBuffers;
	std::vector<ConstantBufferData>           m_ConstantBuffersData;

	std::vector<std::unique_ptr<Buffer>>      m_ObjectDataBuffers;
	std::vector<std::vector<ObjectData>>      m_ObjectDatas;

	std::vector<std::unique_ptr<Buffer>>      m_LightDataBuffers;
	std::vector<std::vector<GPULightData>>    m_GpuLightDatas;

	// --- Layer 6: State & Constants ---
	int      m_Width              = 0;
	int      m_Height             = 0;
	uint32_t m_FrameCount         = 0;
	int      m_FramesInFlight     = 0;
	int      m_CurrentFrame       = 0;
	int      m_CurrentBufferIndex = 0;
	uint64_t m_FenceValue         = 0;

	const uint32_t m_MaxObjectDatas = 100000;
	const uint32_t m_MaxLightObjects = 10000;
	uint32_t       m_CurrentRenderIndex = 0;
	uint32_t       m_FrameLightCount    = 0;
};
