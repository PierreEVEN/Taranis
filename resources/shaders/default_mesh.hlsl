struct VSInput
{
    [[vk::location(0)]] float3 pos : POSITION;
    [[vk::location(1)]] float2 uv : TEXCOORD0;
    [[vk::location(2)]] float3 normal: NORMAL0;
    [[vk::location(3)]] float3 tangent : TANGENT0;
    [[vk::location(4)]] float3 bitangents : BITANGENT0;
    [[vk::location(5)]] float4 color : COLOR0;
};

[[vk::binding(0)]] SamplerState sSampler;
[[vk::binding(1)]] Texture2D    albedo;

#if FEAT_MR
[[vk::binding(2)]] Texture2D mr_map;
#endif
#if FEAT_NORMAL
[[vk::binding(3)]] Texture2D normal_map;
#endif

struct VsToFs
{
    float4 Pos : SV_Position;
    float3 WorldPosition : POSITION0;
    float2 Uvs : TEXCOORD0;
    float3 WorldNormals : NORMAL0;
    float3 WorldTangents : TANGENTS0;
    float3 WorldBitangents: BITANGENT0;
};

struct PushConsts
{
    float4x4 camera;
    float4x4 model;
};

[[vk::push_constant]] ConstantBuffer<PushConsts> pc;

VsToFs vs_main(VSInput input)
{
    VsToFs Out;
    Out.WorldPosition = mul(pc.model, float4(input.pos, 1)).xyz;
    Out.Pos           = mul(pc.camera, float4(Out.WorldPosition, 1));
    Out.Uvs           = input.uv;
    Out.WorldNormals  = normalize(mul((float3x3)pc.model, input.normal));
    Out.WorldTangents = input.tangent;
    Out.WorldBitangents = input.bitangents;
    return Out;
}

float pow_cord(float val)
{
    return pow(abs(sin(val * 50)), 10);
}

struct FsOutput
{
    float3 position : SV_TARGET0;
    float4 albedo_m : SV_TARGET1;
    float4 normal_r : SV_TARGET2;
};

FsOutput fs_main(VsToFs input)
{
    float4 tex_col = albedo.Sample(sSampler, input.Uvs, 1);

    float2 mr           = float2(0, 1);
    float3 local_normal = float3(0, 0, 1);

    FsOutput output;
#if FEAT_MR
        mr = mr_map.Sample(sSampler, input.Uvs, 1).bg;
#endif
#if FEAT_NORMAL
        local_normal = (normal_map.Sample(sSampler, input.Uvs, 1).rgb - 0.5) * 2;
#endif

    float3 world_normal = normalize(input.WorldNormals);
    if (length(input.WorldTangents) > 0.001)
    {
        float3 N = normalize(world_normal);
        float3 T = normalize(abs(input.WorldTangents));
        float3x3 TBN = float3x3(T, -normalize(cross(N, T)), N);
        world_normal = normalize(mul(TBN, local_normal));
    }

    output.position = input.WorldPosition;
    output.albedo_m = float4(tex_col.rgb, mr.r);
    output.normal_r = float4(world_normal / 2 + 0.5, mr.g);
    if (tex_col.a < 0.99)
        discard;
    return output;
}