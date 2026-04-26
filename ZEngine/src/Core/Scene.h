#pragma once

class Entity;
class Renderer;
class ModelManager;

class Texture2D;
class TextureCube;
class CommandContext;
class DescriptorManager;

class Scene
{
public:
	Scene(Renderer* renderer);
	~Scene();

	entt::registry& GetRegistry() { return m_Registry; }
	const std::vector<entt::id_type>& GetGlobalComponentIdTypes() const { return m_GlobalComponentIdTypes; }

	Entity CreateEntity(const std::string& name);

	void InitModel();
	void InitEnvironment(ID3D12Device* device, CommandContext* commandContext, DescriptorManager* descriptorManager);
	void Update(float dt);
private:
	friend class Entity;

	entt::registry m_Registry;

	ModelManager* m_ModelManager;
	
	std::vector<entt::id_type>		m_GlobalComponentIdTypes;

	std::unique_ptr<Texture2D>		m_BRDFLUT_Texture;
	std::unique_ptr<TextureCube>	m_SkyboxTexture;
	std::unique_ptr<TextureCube>	m_IrradianceTexture;
	std::unique_ptr<TextureCube>	m_PrefilteredTexture;

};