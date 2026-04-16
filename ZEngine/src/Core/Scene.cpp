#include "pch.h"
#include "Scene.h"
#include "Entity.h"
#include "CoreComponent.h"
#include "ModelManager.h"
#include "RenderComponent.h"


Scene::Scene(ModelManager* modelManager) : m_ModelManager(modelManager)
{
}

Scene::~Scene() = default;

Entity Scene::CreateEntity(const std::string& name)
{
	Entity entity(m_Registry.create(), this);
	entity.AddComponent<TagComponent>(name);
	entity.AddComponent<TransformComponent>();

	return entity;
}

void Scene::InitModel()
{

	Entity m_ElfGirl = CreateEntity("Elf Girl");
	Entity m_AnimeGirl = CreateEntity("Anime Girl");

	m_AnimeGirl.AddComponent<MeshComponent>(m_ModelManager->InitModel("Resources/Models/Girl/scene.gltf"));
	m_ElfGirl.AddComponent<MeshComponent>(m_ModelManager->InitModel("Resources/Models/Elf/scene.gltf"));

	m_AnimeGirl.GetComponent<TransformComponent>().Position = { -1, 0, 0 };

	m_ElfGirl.GetComponent<TransformComponent>().Position = { 1, 0, 0 };
	m_ElfGirl.GetComponent<TransformComponent>().Rotation = { 0, 0, 0 };
	m_ElfGirl.GetComponent<TransformComponent>().Scale = { 0.01, 0.01, 0.01 };

	m_ModelManager->UploadMaterialBuffer();
}

void Scene::Update(float dt)
{
	m_Registry.view<TransformComponent>().each([=](TransformComponent& transform)
		{
			transform.Rotation.y += dt;
		});
}
