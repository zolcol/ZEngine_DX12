#pragma once
#include "Model.h"
#include "Inspector.h"


struct MeshComponent
{
	Model* model;
};

struct RenderIndexComponent
{
	uint32_t renderIndex;
};

struct EnvironmentComponent
{
	uint32_t SkyboxSRVIndex			= UINT_MAX;
	uint32_t IrradianceSRVIndex		= UINT_MAX;
	uint32_t PrefilteredSRVIndex	= UINT_MAX;
	uint32_t BrdfLutSRVIndex		= UINT_MAX;

	float IBLIntensity = 1.0f;

	void Inspect()
	{
		Inspector::Property("IBL Intensity", IBLIntensity, 0.05f, 0.0f, 10.0f);
	}
};

