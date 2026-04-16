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
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD;
    float4 tangent  : TANGENT;
};

// Bindless SRV Table
Texture2D<float4> GlobalTextures[10000] : register(t0, space0);

// Bindless UAV Table
RWTexture2D<float4> GlobalRWTextures[10000] : register(u0, space0);

SamplerState LinearWrapSampler : register(s0);
SamplerState PointClampSampler : register(s1);

// Cbuffer 1: Camera Matrices
cbuffer TransformBuffer : register(b0, space2)
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
    float3   CameraPos;
    float padding;
};

// Cbuffer 2: Root Constants (Object Material Data)
cbuffer MaterialBuffer : register(b0, space1)
{
    uint MaterialID;
    uint ObjectRenderID;
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

// Mateial Structured Buffer
StructuredBuffer<MaterialData> GlobalMaterials : register(t0, space2);
// ObjectData Structured Buffr
StructuredBuffer<ObjectData> GlobalObjectData : register(t1, space2);

PixelInput VSMain(VertexInput input)
{
    PixelInput output;
    
    float4 pos = float4(input.position, 1.0f);
    float4x4 WorldMatrix = GlobalObjectData[ObjectRenderID].WorldTransformMatrix;
    
    pos = mul(pos, WorldMatrix);
    pos = mul(pos, ViewMatrix);
    pos = mul(pos, ProjectionMatrix);

    output.position = pos;
    
    // Transform Normal and Tangent to World Space using the 3x3 rotation part of WorldMatrix
    // Note: If non-uniform scaling is applied, an inverse transpose matrix should be used instead
    output.normal = mul(input.normal, (float3x3)WorldMatrix);
    
    // Transform tangent vector and preserve the tangent sign (w)
    output.tangent = float4(mul(input.tangent.xyz, (float3x3)WorldMatrix), input.tangent.w);
    
    output.uv = input.uv;
    
    return output;
}

float4 PSMain(PixelInput input) : SV_TARGET
{
    // Normals interpolated across the triangle surface must be re-normalized
    float3 normal = normalize(input.normal);
    
    // Sample the color from the bindless texture array using NonUniformResourceIndex for SM 5.1
    uint albedoID = GlobalMaterials[MaterialID].AlbedoTextureID;
    float4 texColor = GlobalTextures[albedoID].Sample(LinearWrapSampler, input.uv);
    
    // Simply output the sampled color for now
    return texColor;
}