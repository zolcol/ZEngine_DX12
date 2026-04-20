// src/Renderer/Shaders/Shadow.hlsl

// Root Constant (Space 1)
cbuffer RootConstants : register(b0, space1)
{
    uint MaterialID;
    uint ObjectRenderID;
    uint LightCount;
};

// Shadow Transform Buffer (Space 2 - Register b1)
// Đăng ký khớp với: descriptorManager->CreateRootCBVPerFrame(..., 1, 2, ...)
cbuffer ShadowTransformBuffer : register(b1, space2)
{
    float4x4 LightViewMatrix;
    float4x4 LightProjectionMatrix;
    float3   PaddingPos;
    float    Padding;
};

struct ObjectData
{
    float4x4 WorldTransformMatrix;
};
StructuredBuffer<ObjectData> GlobalObjectData : register(t1, space2);

float4 VSMain(float3 position : POSITION) : SV_POSITION
{
    float4x4 WorldMatrix = GlobalObjectData[ObjectRenderID].WorldTransformMatrix;
    
    // Chuyển vertex sang không gian thế giới, sau đó sang không gian của đèn (Light Space)
    float4 worldPos = mul(float4(position, 1.0f), WorldMatrix);
    return mul(mul(worldPos, LightViewMatrix), LightProjectionMatrix);
}
