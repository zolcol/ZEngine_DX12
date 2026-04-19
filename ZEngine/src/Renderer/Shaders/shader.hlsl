struct VertexInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD;
    float4 tangent  : TANGENT;
};

struct PixelInput
{
    float4 position : SV_POSITION;
    float3 worldPos : WORLD_POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD;
    float4 tangent  : TANGENT;
};

static const float PI = 3.14159265359;

// ==========================================
// RESOURCES
// ==========================================

// Bindless SRV Table
Texture2D<float4> GlobalTextures[10000] : register(t0, space0);

SamplerState LinearWrapSampler : register(s0);
SamplerState PointClampSampler : register(s1);

// Cbuffer 1: Camera Matrices
cbuffer TransformBuffer : register(b0, space2)
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
    float3   CameraPos;
    float    Padding;
};

// Cbuffer 2: Root Constants
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
    float  Intensity;
    float3 Position;
    float  Range;
    float3 Direction;
    int    Type; // 0: Dir, 1: Point, 2: Spot
    float  InnerAngle;
    float  OuterAngle;
    float2 Padding;
};

StructuredBuffer<MaterialData> GlobalMaterials : register(t0, space2);
StructuredBuffer<ObjectData>   GlobalObjectData : register(t1, space2);
StructuredBuffer<LightData>    GlobalLights     : register(t2, space2);

// ==========================================
// PBR MATH FUNCTIONS
// ==========================================

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 0.0000001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ==========================================
// MAIN SHADERS
// ==========================================

PixelInput VSMain(VertexInput input)
{
    PixelInput output;
    
    float4x4 WorldMatrix = GlobalObjectData[ObjectRenderID].WorldTransformMatrix;
    float4 worldPos = mul(float4(input.position, 1.0f), WorldMatrix);
    
    output.worldPos = worldPos.xyz;
    output.position = mul(mul(worldPos, ViewMatrix), ProjectionMatrix);
    
    // Transform Normal and Tangent to World Space
    output.normal = normalize(mul(input.normal, (float3x3)WorldMatrix));
    output.tangent = float4(normalize(mul(input.tangent.xyz, (float3x3)WorldMatrix)), input.tangent.w);
    output.uv = input.uv;
    
    return output;
}

float4 PSMain(PixelInput input) : SV_TARGET
{
    // 1. Fetch Material Data
    MaterialData mat = GlobalMaterials[MaterialID];
    
    float4 albedoTex = GlobalTextures[mat.AlbedoTextureID].Sample(LinearWrapSampler, input.uv);
    float3 albedo = pow(albedoTex.rgb, 2.2); // Convert to Linear Space
    
    float3 normalMap = GlobalTextures[mat.NormalTextureID].Sample(LinearWrapSampler, input.uv).rgb;
    normalMap = normalMap * 2.0 - 1.0;
    
    float3 orm = GlobalTextures[mat.ORMTextureID].Sample(LinearWrapSampler, input.uv).rgb;
    float ao = orm.r;
    float roughness = orm.g;
    float metallic = orm.b;

    // 2. Prepare TBN and Normal
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent.xyz);
    float3 B = cross(N, T) * input.tangent.w;
    float3x3 TBN = float3x3(T, B, N);
    N = normalize(mul(normalMap, TBN));

    // 3. Prepare Vectors
    float3 V = normalize(CameraPos - input.worldPos);
    float3 F0 = float3(0.04, 0.04, 0.04); 
    F0 = lerp(F0, albedo, metallic);

    // 4. Lighting Loop
    float3 Lo = float3(0.0, 0.0, 0.0);
    for(uint i = 0; i < LightCount; ++i)
    {
        LightData light = GlobalLights[i];
        float3 L, radiance;
        float attenuation = 1.0;

        if(light.Type == 0) // Directional
        {
            L = normalize(-light.Direction);
            radiance = light.Color * light.Intensity;
        }
        else // Point & Spot
        {
            float3 lightVec = light.Position - input.worldPos;
            float distance = length(lightVec);
            L = normalize(lightVec);
            
            // Physical inverse square falloff
            attenuation = 1.0 / (distance * (distance + 0.0001));
            
            if(light.Type == 2) // Spot
            {
                float theta = dot(L, normalize(-light.Direction));
                float epsilon = light.InnerAngle - light.OuterAngle;
                float spotIntensity = clamp((theta - light.OuterAngle) / epsilon, 0.0, 1.0);
                attenuation *= spotIntensity;
            }
            
            radiance = light.Color * light.Intensity * attenuation;
        }

        // Cook-Torrance BRDF
        float3 H = normalize(V + L);
        float D = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

        float3 numerator = D * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        float3 specular = numerator / denominator;

        float3 kS = F;
        float3 kD = float3(1.0, 1.0, 1.0) - kS;
        kD *= 1.0 - metallic;

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // 5. Final Color Composition
    float3 ambient = float3(0.03, 0.03, 0.03) * albedo * ao;
    float3 color = ambient + Lo;

    // Tone Mapping & Gamma Correction
    color = color / (color + float3(1.0, 1.0, 1.0));
    color = pow(color, float3(1.0/2.2, 1.0/2.2, 1.0/2.2));

    return float4(color, albedoTex.a);
}
