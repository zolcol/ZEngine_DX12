#include "entt/entt.hpp"
#include <entt/meta/factory.hpp>
#include <entt/meta/resolve.hpp>
#include "CoreComponent.h"
#include "Editor.h"

using namespace entt::literals;
void RegisterMetaData()
{
	entt::meta_factory<TagComponent>{}
	.custom<EditorComponentInfo>("Tag Component")
		.func<&TagComponent::Inspect>("Inspect"_hs);

	entt::meta_factory<TransformComponent>{}
	.custom<EditorComponentInfo>("Transform Component")
		.func<&TransformComponent::Inspect>("Inspect"_hs);

	entt::meta_factory<CameraComponent>{}
	.custom<EditorComponentInfo>("Camera Component")
		.func<&CameraComponent::Inspect>("Inspect"_hs);

	entt::meta_factory<LightComponent>{}
	.custom<EditorComponentInfo>("Light Component")
		.func<&LightComponent::Inspect>("Inspect"_hs);
	}