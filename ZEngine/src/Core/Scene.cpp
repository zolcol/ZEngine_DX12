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
	renderer->ConnectToScene(m_Registry);
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
	

	Entity light1 = CreateEntity("Light 1");
	light1.AddComponent<LightComponent>();
	light1.GetComponent<LightComponent>().CastShadow = true;
	light1.GetComponent<LightComponent>().Intensity = 5;
	light1.GetComponent<TransformComponent>().Position = { -2.0f, 4.0f, -2.0f };
	light1.GetComponent<TransformComponent>().SetEulerAnglesDegrees({ 45.0f, 45.0f, 0.0f });
	
	Model* arrowModel = m_ModelManager->InitModel("Resources/Models/Arrow/scene.gltf", { 0.1f, 0.1f, 0.1f }, { 0, -90, 0 });
	if (arrowModel)
	{
		light1.AddComponent<MeshComponent>(arrowModel);
		light1.AddComponent<RenderIndexComponent>();
	}

	Entity m_ElfGirl = CreateEntity("Elf Girl");
	Model* elfModel = m_ModelManager->InitModel("Resources/Models/Elf/scene.gltf", { 0.01, 0.01, 0.01 }, { 90, 0, 0 });
	if (elfModel)
	{
		m_ElfGirl.AddComponent<MeshComponent>(elfModel);
		m_ElfGirl.AddComponent<RenderIndexComponent>();
	}
	m_ElfGirl.GetComponent<TransformComponent>().Position = { 0.5, 0, 0 };

	Entity m_AnimeGirl = CreateEntity("Anime Girl");
	Model* girlModel = m_ModelManager->InitModel("Resources/Models/Girl/scene.gltf");
	if (girlModel)
	{
		m_AnimeGirl.AddComponent<MeshComponent>(girlModel);
		m_AnimeGirl.AddComponent<RenderIndexComponent>();
	}
	m_AnimeGirl.GetComponent<TransformComponent>().Position = { -0.5, 0, 0 };

	Entity m_KnightGirl = CreateEntity("Knight Girl");
	Model* knightModel = m_ModelManager->InitModel("Resources/Models/KnightGirl/scene.gltf", { 0.02, 0.02, 0.02 }, { 90, 0, 0 });
	if (knightModel)
	{
		m_KnightGirl.AddComponent<MeshComponent>(knightModel);
		m_KnightGirl.AddComponent<RenderIndexComponent>();
	}
	m_KnightGirl.GetComponent<TransformComponent>().Position = { -1, 0, 0 };

	m_ModelManager->UploadMaterialBuffer();
}

void Scene::Update(float dt) 
{
	m_Registry.view<TransformComponent>().each([=](entt::entity entity, TransformComponent& transform)
		{
			if (!m_Registry.any_of<CameraComponent>(entity) && !m_Registry.any_of<LightComponent>(entity))
			{
				transform.Rotate({ 0.0f, 1.0f, 0.0f }, dt * 50.0f);
			}
		});
}
