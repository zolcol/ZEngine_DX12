struct VertexInput
{
    float3 position : POSITION;
    float4 color    : COLOR;
    float2 uv       : TEXCOORD;
};

struct PixelInput
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
    float2 uv       : TEXCOORD;
};

// Bindless SRV Table
Texture2D<float4> GlobalTextures[10000] : register(t0, space0);

// Bindless UAV Table
RWTexture2D<float4> GlobalRWTextures[] : register(u0, space0);

SamplerState LinearWrapSampler : register(s0);
SamplerState PointClampSampler : register(s1);

cbuffer TransformBuffer : register(b0, space2)
{
    float4x4 WorldMatrix;
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
};

cbuffer MaterialBuffer : register(b0, space1)
{
    uint TextureID;
}

PixelInput VSMain(VertexInput input)
{
    PixelInput output;
    
    float4 pos = float4(input.position, 1.0f);

    pos = mul(pos, WorldMatrix);
    pos = mul(pos, ViewMatrix);
    pos = mul(pos, ProjectionMatrix);

    output.position = pos;
    output.color = input.color;
    output.uv = input.uv;
    
    return output;
}

float4 PSMain(PixelInput input) : SV_TARGET
{
    float4 texColor = GlobalTextures[TextureID].Sample(LinearWrapSampler, input.uv);
    return texColor;
}