struct MipSettings
{
    uint srvIndex;
    uint uavIndex;
    uint textureType;
};

ConstantBuffer<MipSettings> Config : register(b0);
Texture2D<float4> GlobalSRV[10000] : register(t0, space0);
RWTexture2D<float4> GlobalUAV[10000] : register(u0, space0);

static const uint TEXTURE_TYPE_ALBEDO = 0;
static const uint TEXTURE_TYPE_NORMAL = 1;
static const uint TEXTURE_TYPE_ORM = 2;
static const uint TEXTURE_TYPE_EMISSIVE = 3;

float3 LinearToSRGB(float3 color)
{
    float3 s1 = sqrt(color);
    float3 s2 = sqrt(s1);
    float3 s3 = sqrt(s2);
    return 0.662002687 * s1 + 0.684122060 * s2 - 0.323583601 * s3 - 0.0225411470 * color;
}

[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    Texture2D<float4> srcTex = GlobalSRV[NonUniformResourceIndex(Config.srvIndex)];
    RWTexture2D<float4> dstTex = GlobalUAV[NonUniformResourceIndex(Config.uavIndex)];

    uint width, height;
    dstTex.GetDimensions(width, height);

    if (DTid.x >= width || DTid.y >= height)
        return;

    uint2 srcCoord = DTid.xy * 2;

    float4 p00 = srcTex.Load(int3(srcCoord, 0));
    float4 p01 = srcTex.Load(int3(srcCoord + uint2(1, 0), 0));
    float4 p10 = srcTex.Load(int3(srcCoord + uint2(0, 1), 0));
    float4 p11 = srcTex.Load(int3(srcCoord + uint2(1, 1), 0));

    float4 resultColor = (p00 + p01 + p10 + p11) * 0.25f;

    if (Config.textureType == TEXTURE_TYPE_ALBEDO || Config.textureType == TEXTURE_TYPE_EMISSIVE)
    {
        resultColor.rgb = LinearToSRGB(resultColor.rgb);
    }
    else if (Config.textureType == TEXTURE_TYPE_NORMAL)
    {
        // Normalizing the averaged normal map to prevent loss of detail on smaller mips
        float3 n = resultColor.xyz * 2.0f - 1.0f;
        n = normalize(n);
        resultColor.xyz = n * 0.5f + 0.5f;
    }

    dstTex[DTid.xy] = resultColor;
}