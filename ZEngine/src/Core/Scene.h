#pragma once

class Entity;
class ModelManager;

class Scene
{
public:
	Scene(ModelManager* modelManager);
	~Scene();

	entt::registry& GetRegistry() { return m_Registry; }

	Entity CreateEntity(const std::string& name);

	void InitModel();
	void Update(float dt);
private:
	friend class Entity;

	entt::registry m_Registry;

	ModelManager* m_ModelManager;
};