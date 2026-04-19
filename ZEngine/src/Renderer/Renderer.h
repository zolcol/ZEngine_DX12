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
class Model;
class Scene;
class Entity;
struct TransformComponent;
class Editor;
struct GPULightData;

class Renderer
{
public:
	Renderer();
	~Renderer();

	Device* GetDevice() const { return m_Device.get(); }
	ModelManager* GetModelManager() const { return m_ModelManager.get(); }

	bool Init(HWND hwnd, int width, int height, uint32_t frameCount);
	
	void BeginFrame(Scene* scene);
	void EndFrame(Scene* scene, Editor* editor);
	void ShutDown();

	void ConnnetToScene(entt::registry& registry);

private:
	// --- Infrastructure ---
	std::unique_ptr<Device>         m_Device;
	std::unique_ptr<Swapchain>      m_Swapchain;
	std::unique_ptr<CommandContext> m_CommandContext;
	std::unique_ptr<Fence>          m_Fence;

	// --- Resources ---
	std::unique_ptr<DescriptorManager>			m_DescriptorManager;
	std::unique_ptr<ModelManager>				m_ModelManager;
	std::vector<std::unique_ptr<Buffer>>		m_ConstantBuffers;
	std::vector<ConstantBufferData>				m_ConstantBuffersData;
	std::vector<std::unique_ptr<Buffer>>		m_ObjectDataBuffers;
	std::vector<std::vector<ObjectData>>		m_ObjectDatas;

	// Light
	uint32_t									m_FrameLightCount = 0;
	std::vector<std::vector<GPULightData>>		m_LightDatas;
	std::vector<std::unique_ptr<Buffer>>		m_LightDataBuffers;

	std::unique_ptr<TextureDepth>	m_DepthTexture;

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

	const uint32_t MAX_OBJECT_DATAS = 100000;
	const uint32_t MAX_LIGHT_OBJECT = 10000;
	uint32_t m_CurrentRenderIndex = 0;

	void InitRootConstants();
	void InitConstantBuffers();
	void UpdateConstantBuffersData(int currentFrame, Scene* scene);

	void InitDepthBuffer();
	void InitObjectDataBuffers();
	void UpdateObjectDatas(int currentFrame, Scene* scene);

	void InitLightBuffers();
	void UpdateLightBuffers(int currentFrame, Scene* scene);

	void OnRenderIndexCreated(entt::registry& registry, entt::entity entity);
};
