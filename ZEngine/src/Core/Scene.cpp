#include "pch.h"
#include "Scene.h"
#include "Entity.h"
#include "CoreComponent.h"
#include "ModelManager.h"
#include "RenderComponent.h"
#include "Renderer/Renderer.h"


Scene::Scene(Renderer* renderer)
{
	m_ModelManager = renderer->GetModelManager();
	renderer->ConnnetToScene(m_Registry);
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
	Entity camera = CreateEntity("Main Camera");
	camera.AddComponent<CameraComponent>();
	camera.GetComponent<TransformComponent>().Position = { 0.0f, 1.0f, -3.0f };

	Entity m_ElfGirl = CreateEntity("Elf Girl");
	Entity m_AnimeGirl = CreateEntity("Anime Girl");

	m_AnimeGirl.AddComponent<MeshComponent>(m_ModelManager->InitModel("Resources/Models/Girl/scene.gltf"));
	m_ElfGirl.AddComponent<MeshComponent>(m_ModelManager->InitModel("Resources/Models/Elf/scene.gltf"));
	m_AnimeGirl.AddComponent<RenderIndexComponent>();
	m_ElfGirl.AddComponent<RenderIndexComponent>();

	m_AnimeGirl.GetComponent<TransformComponent>().Position = { -0.5, 0, 0 };

	m_ElfGirl.GetComponent<TransformComponent>().Position = { 0.5, 0, 0 };
	m_ElfGirl.GetComponent<TransformComponent>().Rotation = { XM_PIDIV2, 0, 0 };
	m_ElfGirl.GetComponent<TransformComponent>().Scale = { 0.01, 0.01, 0.01 };

	m_ModelManager->UploadMaterialBuffer();
}

void Scene::Update(float dt)
{
	m_Registry.view<TransformComponent>().each([=](entt::entity entity, TransformComponent& transform)
		{
			if (!m_Registry.any_of<CameraComponent>(entity))
			{
				transform.Rotation.y += dt;
			}
		});
}
