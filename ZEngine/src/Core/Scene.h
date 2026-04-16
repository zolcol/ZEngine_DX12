#pragma once

class Entity;
class Renderer;
class ModelManager;

class Scene
{
public:
	Scene(Renderer* renderer);
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