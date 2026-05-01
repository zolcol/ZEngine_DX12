#include "pch.h"
#include "Scene.h"
#include "Entity.h"
#include "CoreComponent.h"
#include "ModelManager.h"
#include "RenderComponent.h"
#include "Renderer/Renderer.h"
#include "Renderer/Texture2D.h"
#include "Renderer/TextureCube.h"
#include "Renderer/CommandContext.h"
#include "Renderer/DescriptorManager.h"

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


	Entity m_Mirror = CreateEntity("Ball");
	Model* mirrorModel = m_ModelManager->InitModel("Resources/Models/Mirror/scene.gltf", { 0.01, 0.01, 0.01 }, { 0, 0, -90 });
	if (mirrorModel)
	{
		m_Mirror.AddComponent<MeshComponent>(mirrorModel);
		m_Mirror.AddComponent<RenderIndexComponent>();
	}

	m_ModelManager->UploadMaterialBuffer();

	/*Entity m_Sofa = CreateEntity("Sofa");
	Model* sofaModel = m_ModelManager->InitModel("Resources/Models/Sofa/scene.gltf", { 0.005, 0.005, 0.005 }, { 0, 0, 0});
	if (sofaModel)
	{
		m_Sofa.AddComponent<MeshComponent>(sofaModel);
		m_Sofa.AddComponent<RenderIndexComponent>();
	}*/

	Entity m_Plane = CreateEntity("Plane");
	Model* planeModel = m_ModelManager->InitModel("Resources/Models/Plane/scene.gltf", { 3, 3, 3 },  { 90, 0,0 });
	if (planeModel)
	{
		m_Plane.AddComponent<MeshComponent>(planeModel);
		m_Plane.AddComponent<RenderIndexComponent>();
	}


	m_ModelManager->UploadMaterialBuffer();
}

void Scene::InitEnvironment(ID3D12Device* device, CommandContext* commandContext, DescriptorManager* descriptorManager)
{
	m_BRDFLUT_Texture = std::make_unique<Texture2D>();
	m_SkyboxTexture = std::make_unique<TextureCube>();
	m_IrradianceTexture = std::make_unique<TextureCube>();
	m_PrefilteredTexture = std::make_unique<TextureCube>();

	m_BRDFLUT_Texture->Init(device, commandContext, descriptorManager,		"Resources/Textures/SkyBox/brdf_lut.dds");
	m_SkyboxTexture->Init(device, commandContext, descriptorManager,		"Resources/Textures/SkyBox/sky_box_skybox.dds");
	m_IrradianceTexture->Init(device, commandContext, descriptorManager,	"Resources/Textures/SkyBox/sky_box_irradiance.dds");
	m_PrefilteredTexture->Init(device, commandContext, descriptorManager,	"Resources/Textures/SkyBox/sky_box_prefiltered.dds");

	m_Registry.ctx().emplace<EnvironmentComponent>();
	auto& envComponent = m_Registry.ctx().get<EnvironmentComponent>();
	envComponent.BrdfLutSRVIndex =		m_BRDFLUT_Texture->GetSRVIndex();
	envComponent.SkyboxSRVIndex =		m_SkyboxTexture->GetSRVIndex();
	envComponent.IrradianceSRVIndex =	m_IrradianceTexture->GetSRVIndex();
	envComponent.PrefilteredSRVIndex =	m_PrefilteredTexture->GetSRVIndex();
	envComponent.IBLIntensity = 0;

	m_GlobalComponentIdTypes.push_back(entt::type_id<EnvironmentComponent>().hash());
}

void Scene::Update(float dt) 
{
	m_Registry.view<TransformComponent>().each([=](entt::entity entity, TransformComponent& transform)
		{
			if (!m_Registry.any_of<CameraComponent>(entity) && !m_Registry.any_of<LightComponent>(entity))
			{
				if (!(m_Registry.get<TagComponent>(entity).name == "Plane"))
				{
					//transform.Rotate({ 0.0f, 1.0f, 0.0f }, dt * 20.0f);

				}
			}
		});
}
