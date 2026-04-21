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

SamplerState LinearWrapSampler : register(s0);
SamplerState PointClampSampler : register(s1);
SamplerComparisonState ShadowSampler : register(s2);

cbuffer TransformBuffer : register(b0, space2)
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
    float3 CameraPos;
    float Padding;
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
    int Type; // 0: Directional, 1: Point, 2: Spot
    float InnerAngle; // cos(innerAngle)
    float OuterAngle; // cos(outerAngle)
    int ShadowMapIndex;
    float _Pad;
    float4x4 LightViewProj;
};

StructuredBuffer<MaterialData> GlobalMaterials : register(t0, space2);
StructuredBuffer<ObjectData> GlobalObjectData : register(t1, space2);
StructuredBuffer<LightData> GlobalLights : register(t2, space2);

// ==========================================
// SHADOW: Poisson Disk 16-Tap + Normal Offset
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

    // Lấy kích thước thực tế của Shadow Map từ Texture
    float width, height;
    GlobalTextures[light.ShadowMapIndex].GetDimensions(width, height);
    float2 texelSize = 1.0 / float2(width, height);

    // Normal-Offset Bias logic from Shader 2
    float NoL = clamp(dot(N, L), 0.0, 1.0);
    float3 biasPos = worldPos + N * (1.0 - NoL) * 0.05;

    float4 shadowPos = mul(float4(biasPos, 1.0), light.LightViewProj);
    float3 projCoords = shadowPos.xyz / shadowPos.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    projCoords.y = 1.0 - projCoords.y;

    // Frustum check
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 1.0;

    float currentDepth = projCoords.z;
    float bias = 0.0003;

    // Poisson rotation per pixel
    float noise = InterleavedGradientNoise(screenPos) * 2.0 * PI;
    float2x2 rot = RotationMatrix(noise);
    
    // Spread tính theo đơn vị texel (ví dụ nhòe ra 1.5 pixels)
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
// PBR: Cook-Torrance BRDF
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
    
    float3x3 normalMatrix = (float3x3) WorldMatrix;
    output.normal = normalize(mul(input.normal, normalMatrix));
    output.tangent = float4(normalize(mul(input.tangent.xyz, normalMatrix)), input.tangent.w);
    output.uv = input.uv;

    return output;
}

// ==========================================
// PIXEL SHADER
// ==========================================

float4 PSMain(PixelInput input) : SV_TARGET
{
    MaterialData mat = GlobalMaterials[MaterialID];

    // Texture fetching
    float4 albedoSample = GlobalTextures[mat.AlbedoTextureID].Sample(LinearWrapSampler, input.uv);
    float3 albedo = albedoSample.rgb;

    float3 orm = GlobalTextures[mat.ORMTextureID].Sample(LinearWrapSampler, input.uv).rgb;
    float ao = orm.r;
    float roughness = clamp(orm.g, 0.04, 1.0);
    float metallic = saturate(orm.b);

    float3 emissive = GlobalTextures[mat.EmissiveTextureID].Sample(LinearWrapSampler, input.uv).rgb;

    // Build TBN 
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent.xyz);
    
    // Gram-Schmidt
    T = normalize(T - dot(T, N) * N);
    float3 B = cross(N, T) * input.tangent.w;
    float3x3 TBN = float3x3(T, B, N);

    float3 normalMap = GlobalTextures[mat.NormalTextureID].Sample(LinearWrapSampler, input.uv).rgb;
    normalMap = normalMap * 2.0 - 1.0;
    normalMap = normalize(normalMap);
    N = normalize(mul(normalMap, TBN));

    // Common vectors
    float3 V = normalize(CameraPos - input.worldPos);
    float NdotV = max(dot(N, V), 0.0);
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);

    // Direct Lighting
    float3 Lo = float3(0.0, 0.0, 0.0);

    [loop]
    for (uint i = 0; i < LightCount; ++i)
    {
        LightData light = GlobalLights[i];
        float3 L;
        float attenuation = 1.0;

        if (light.Type == 0) // Directional
        {
            L = normalize(-light.Direction);
        }
        else // Point & Spot with physical windowing
        {
            float3 lightVec = light.Position - input.worldPos;
            float distance = length(lightVec);
            L = normalize(lightVec);
            
            float dRange = distance / max(light.Range, 0.0001);
            float windowing = clamp(1.0 - pow(dRange, 4.0), 0.0, 1.0);
            attenuation = (1.0 / (distance * distance + 1.0)) * (windowing * windowing);

            if (light.Type == 2) // Spot
            {
                float theta = dot(L, normalize(-light.Direction));
                float epsilon = light.InnerAngle - light.OuterAngle;
                float spotIntensity = clamp((theta - light.OuterAngle) / max(epsilon, 0.001), 0.0, 1.0);
                spotIntensity = smoothstep(0.0, 1.0, spotIntensity);
                attenuation *= spotIntensity;
            }
        }

        float3 radiance = light.Color * light.Intensity * attenuation;
        
        float2 screenPos = input.position.xy;
        float shadow = CalculateShadow(input.worldPos, N, L, light, screenPos);

        // Cook-Torrance
        float3 H = normalize(V + L);
        float NdotL = max(dot(N, L), 0.0);
        float NdotH = max(dot(N, H), 0.0);
        float HdotV = max(dot(H, V), 0.0);

        float D = DistributionGGX(NdotH, roughness);
        float G = GeometrySmith(NdotV, NdotL, roughness);
        float3 F = FresnelSchlick(HdotV, F0);

        float3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 1e-4);
        
        float3 kS = F;
        float3 kD = (1.0 - kS) * (1.0 - metallic);

        Lo += (kD * albedo * INV_PI + specular) * radiance * NdotL * shadow;
    }

    // Ambient IBL Approx (Lazarov)
    float3 F_ambient = FresnelSchlickRoughness(NdotV, F0, roughness);
    float3 kS_amb = F_ambient;
    float3 kD_amb = (1.0 - kS_amb) * (1.0 - metallic);
    float3 diffuseAmbient = kD_amb * albedo * 0.05;

    float2 envBRDF;
    {
        const float4 c0 = float4(-1.0, -0.0275, -0.572, 0.022);
        const float4 c1 = float4(1.0, 0.0425, 1.040, -0.040);
        float4 r = roughness * c0 + c1;
        float a004 = min(r.x * r.x, exp2(-9.28 * NdotV)) * r.x + r.y;
        envBRDF = float2(-1.04, 1.04) * a004 + r.zw;
    }
    
    float3 envRadiance = float3(0.12, 0.14, 0.18);
    float3 specularAmbient = envRadiance * (F_ambient * envBRDF.x + envBRDF.y);
    float3 ambient = (diffuseAmbient + specularAmbient) * ao;

    // Compose final color
    float3 color = ambient + Lo + emissive;

    color *= 1.0; // Exposure
    color = ACESFilmic(color);
    color = pow(max(color, 0.0), 1.0 / 2.2);

    return float4(color, albedoSample.a);
}