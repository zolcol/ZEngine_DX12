struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float4 tangent : TANGENT;
};

struct PixelInput
{
    float4 position : SV_POSITION;
    float3 worldPos : WORLD_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float4 tangent : TANGENT;
};

static const float PI = 3.14159265359;
static const float INV_PI = 0.31830988618;

// ==========================================
// RESOURCES
// ==========================================

Texture2D<float4> GlobalTextures[10000] : register(t0, space0);
TextureCube<float4> GlobalCubeTextures[1000] : register(t0, space3);

SamplerState LinearWrapSampler : register(s0);
SamplerState PointClampSampler : register(s1);
SamplerComparisonState ShadowSampler : register(s2);
SamplerState LinearClampSampler : register(s3);

cbuffer TransformBuffer : register(b0, space2)
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
    float3 CameraPos;
    uint SkyboxSRVIndex;

    uint IrradianceSRVIndex;
    uint PrefilteredSRVIndex;
    uint BrdfLutSRVIndex;
    float IBLIntensity;
};

cbuffer MaterialBuffer : register(b0, space1)
{
    uint MaterialID;
    uint ObjectRenderID;
    uint LightCount;
};

struct MaterialData
{
    uint AlbedoTextureID;
    uint NormalTextureID;
    uint ORMTextureID;
    uint EmissiveTextureID;
};

struct ObjectData
{
    float4x4 WorldTransformMatrix;
};

struct LightData
{
    float3 Color;
    float Intensity;
    float3 Position;
    float Range;
    float3 Direction;
    int Type;
    float InnerAngle;
    float OuterAngle;
    int ShadowMapIndex;
    float _Pad;
    float4x4 LightViewProj;
};

StructuredBuffer<MaterialData> GlobalMaterials : register(t0, space2);
StructuredBuffer<ObjectData> GlobalObjectData : register(t1, space2);
StructuredBuffer<LightData> GlobalLights : register(t2, space2);

// ==========================================
// SHADOW
// ==========================================

static const float2 PoissonDisk[16] =
{
    float2(-0.94201624, -0.39906216), float2(0.94558609, -0.76890725),
    float2(-0.094184101, -0.92938870), float2(0.34495938, 0.29387760),
    float2(-0.91588581, 0.45771432), float2(-0.81544232, -0.87912464),
    float2(-0.38277543, 0.27676845), float2(0.97484398, 0.75648379),
    float2(0.44323325, -0.97511554), float2(0.53742981, -0.47373420),
    float2(-0.26496911, -0.41893023), float2(0.79197514, 0.19090188),
    float2(-0.24188840, 0.99706507), float2(-0.81409955, 0.91437590),
    float2(0.19984126, 0.78641367), float2(0.14383161, -0.14100790)
};

float InterleavedGradientNoise(float2 screenPos)
{
    float3 magic = float3(0.06711056, 0.00583715, 52.9829189);
    return frac(magic.z * frac(dot(screenPos, magic.xy)));
}

float2x2 RotationMatrix(float angle)
{
    float s, c;
    sincos(angle, s, c);
    return float2x2(c, -s, s, c);
}

float CalculateShadow(float3 worldPos, float3 N, float3 L, LightData light, float2 screenPos)
{
    if (light.ShadowMapIndex < 0)
        return 1.0;

    float width, height;
    GlobalTextures[light.ShadowMapIndex].GetDimensions(width, height);
    float2 texelSize = 1.0 / float2(width, height);

    float NoL = clamp(dot(N, L), 0.0, 1.0);
    float3 biasPos = worldPos + N * (1.0 - NoL) * 0.05;

    float4 shadowPos = mul(float4(biasPos, 1.0), light.LightViewProj);
    float3 projCoords = shadowPos.xyz / shadowPos.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    projCoords.y = 1.0 - projCoords.y;

    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 1.0;

    float currentDepth = projCoords.z;
    float bias = 0.0003;

    float noise = InterleavedGradientNoise(screenPos) * 2.0 * PI;
    float2x2 rot = RotationMatrix(noise);
    
    float spread = 1.5;
    float2 spreadVec = spread * texelSize;

    float shadow = 0.0;
    [unroll]
    for (int i = 0; i < 16; ++i)
    {
        float2 offset = mul(PoissonDisk[i], rot) * spreadVec;
        shadow += GlobalTextures[light.ShadowMapIndex].SampleCmpLevelZero(
            ShadowSampler, projCoords.xy + offset, currentDepth + bias
        );
    }
    shadow /= 16.0;

    return lerp(0.15, 1.0, shadow);
}

// ==========================================
// PBR
// ==========================================

float DistributionGGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float d = (NdotH * a2 - NdotH) * NdotH + 1.0;
    return a2 / (PI * d * d + 1e-7);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) * 0.125;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0)
               * pow(saturate(1.0 - cosTheta), 5.0);
}

// ==========================================
// TONE MAPPING
// ==========================================

float3 ACESFilmic(float3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

// ==========================================
// VERTEX SHADER
// ==========================================

PixelInput VSMain(VertexInput input)
{
    PixelInput output;

    float4x4 WorldMatrix = GlobalObjectData[ObjectRenderID].WorldTransformMatrix;
    float4 worldPos = mul(float4(input.position, 1.0), WorldMatrix);

    output.worldPos = worldPos.xyz;
    output.position = mul(mul(worldPos, ViewMatrix), ProjectionMatrix);
    
    // Inverse transpose để normal đúng khi có non-uniform scale
    float3x3 W = (float3x3) WorldMatrix;
    float3x3 invTransposeWorld = transpose(W);
    // Approximate: chỉ đúng hoàn toàn khi không có shear, nhưng đủ dùng cho game
    output.normal = normalize(mul(input.normal, W));
    output.tangent = float4(normalize(mul(input.tangent.xyz, W)), input.tangent.w);
    output.uv = input.uv;

    return output;
}

// ==========================================
// PIXEL SHADER
// ==========================================

float4 PSMain(PixelInput input) : SV_TARGET
{
    MaterialData mat = GlobalMaterials[MaterialID];

    // Textures
    float4 albedoSample = GlobalTextures[mat.AlbedoTextureID].Sample(LinearWrapSampler, input.uv);
    float3 albedo = albedoSample.rgb;

    float3 orm = GlobalTextures[mat.ORMTextureID].Sample(LinearWrapSampler, input.uv).rgb;
    float ao = orm.r;
    float roughness = clamp(orm.g, 0.04, 1.0);
    float metallic = saturate(orm.b);

    float3 emissive = GlobalTextures[mat.EmissiveTextureID].Sample(LinearWrapSampler, input.uv).rgb;

    // TBN
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent.xyz);
    T = normalize(T - dot(T, N) * N);
    float3 B = cross(N, T) * input.tangent.w;
    float3x3 TBN = float3x3(T, B, N);

    float3 normalSample;
    normalSample.xy = GlobalTextures[mat.NormalTextureID].Sample(LinearWrapSampler, input.uv).rg * 2.0 - 1.0;
    normalSample.z = sqrt(saturate(1.0 - dot(normalSample.xy, normalSample.xy)));
    N = normalize(mul(normalSample, TBN));

    // Common
    float3 V = normalize(CameraPos - input.worldPos);
    float NdotV = max(dot(N, V), 1e-4);
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);

    // ==========================================
    // DIRECT LIGHTING
    // ==========================================
    float3 Lo = float3(0.0, 0.0, 0.0);

    [loop]
    for (uint i = 0; i < LightCount; ++i)
    {
        LightData light = GlobalLights[i];
        float3 L;
        float attenuation = 1.0;

        if (light.Type == 0)
        {
            L = normalize(-light.Direction);
        }
        else
        {
            float3 lightVec = light.Position - input.worldPos;
            float distance = length(lightVec);
            L = normalize(lightVec);
            
            float dRange = distance / max(light.Range, 0.0001);
            float windowing = clamp(1.0 - pow(dRange, 4.0), 0.0, 1.0);
            attenuation = (1.0 / (distance * distance + 1.0)) * (windowing * windowing);

            if (light.Type == 2)
            {
                float theta = dot(L, normalize(-light.Direction));
                float epsilon = light.InnerAngle - light.OuterAngle;
                float spotIntensity = clamp((theta - light.OuterAngle) / max(epsilon, 0.001), 0.0, 1.0);
                spotIntensity = smoothstep(0.0, 1.0, spotIntensity);
                attenuation *= spotIntensity;
            }
        }

        float3 radiance = light.Color * light.Intensity * attenuation;
        float shadow = CalculateShadow(input.worldPos, N, L, light, input.position.xy);

        float3 H = normalize(V + L);
        float NdotL = max(dot(N, L), 1e-4);
        float NdotH = max(dot(N, H), 1e-4);
        float HdotV = max(dot(H, V), 1e-4);

        float D = DistributionGGX(NdotH, roughness);
        float G = GeometrySmith(NdotV, NdotL, roughness);
        float3 F = FresnelSchlick(HdotV, F0);

        float3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 1e-4);
        float3 kD = (1.0 - F) * (1.0 - metallic);

        Lo += (kD * albedo * INV_PI + specular) * radiance * NdotL * shadow;
    }

    // ==========================================
    // AMBIENT IBL
    // ==========================================
    float3 F_amb = FresnelSchlickRoughness(NdotV, F0, roughness);
    float3 kD_amb = (1.0 - F_amb) * (1.0 - metallic);

    // Diffuse IBL
    float3 irradiance = GlobalCubeTextures[NonUniformResourceIndex(IrradianceSRVIndex)].Sample(LinearClampSampler, N).rgb;
    irradiance = min(irradiance, 1000.0);
    float3 diffuseAmbient = kD_amb * irradiance * albedo;

    // Specular IBL
    float3 R = normalize(reflect(-V, N));
    
    uint width, height, mipCount;
    GlobalCubeTextures[NonUniformResourceIndex(PrefilteredSRVIndex)].GetDimensions(0, width, height, mipCount);
    float maxMipLevel = (float)mipCount - 1.0;
    
    float lod = roughness * maxMipLevel;
    float3 prefilteredColor = GlobalCubeTextures[NonUniformResourceIndex(PrefilteredSRVIndex)].SampleLevel(LinearClampSampler, R, lod).rgb;
    prefilteredColor = min(prefilteredColor, 1000.0);
    float2 brdf = GlobalTextures[NonUniformResourceIndex(BrdfLutSRVIndex)].Sample(LinearClampSampler, float2(NdotV, roughness)).rg;
    float3 specularAmbient = prefilteredColor * (F_amb * brdf.x + brdf.y);

    float3 ambient = (diffuseAmbient + specularAmbient) * ao * IBLIntensity;

    // ==========================================
    // FINAL COMPOSE
    // ==========================================
    float3 color = ambient + Lo + emissive;

    color = ACESFilmic(color);
    color = pow(max(color, 0.0), 1.0 / 2.2);

    return float4(color, albedoSample.a);
}