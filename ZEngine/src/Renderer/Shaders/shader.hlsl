struct VertexInput
{
    float3 position : POSITION;
    float4 color    : COLOR;
};

struct PixelInput
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
};

cbuffer ConstantBufferData : register(b0, space1)
{
    float4x4 WorldMatrix;
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
};

PixelInput VSMain(VertexInput input)
{
    PixelInput output;
    
    float4 pos = float4(input.position, 1.0f);

    pos = mul(pos, WorldMatrix);
    pos = mul(pos, ViewMatrix);
    pos = mul(pos, ProjectionMatrix);

    output.position = pos;
    output.color = input.color;
    
    return output;
}

float4 PSMain(PixelInput input) : SV_TARGET
{
    return input.color;
}