#pragma once

#include "Scene.h"

class Entity
{
public:
	Entity() = default;
	Entity(entt::entity handle, Scene* scene)
		: m_Handle(handle), m_Scene(scene) {}
	
	~Entity() = default;

	
	template<typename T, typename... Args>
	T& AddComponent(Args&&... args)
	{
		return m_Scene->m_Registry.emplace<T>(m_Handle, std::forward<Args>(args)...);
	}

	template<typename T>
	T& GetComponent()
	{
		return m_Scene->m_Registry.get<T>(m_Handle);
	}

	template<typename T>
	bool HasComponent()
	{
		return m_Scene->m_Registry.any_of<T>(m_Handle);
	}
private:
	entt::entity m_Handle;
	Scene* m_Scene;
};