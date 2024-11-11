struct VSInput
{
    [[vk::location(0)]] float3 pos : POSITION;
    [[vk::location(1)]] float2 uv : TEXCOORD0;
    [[vk::location(2)]] float3 normal: NORMAL0;
    [[vk::location(3)]] float3 tangent : TANGENT0;
    [[vk::location(4)]] float4 color : COLOR0;
};

[[vk::binding(0)]] SamplerState sSampler;
[[vk::binding(1)]] Texture2D albedo;

struct VsToFs
{
    float4 Pos : SV_Position;
    float3 WorldPosition : POSITION0;
    float2 Uvs : TEXCOORD0;
    float3 WorldNormals : NORMAL0;
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
    Out.Pos = mul(pc.camera, float4(Out.WorldPosition, 1));
    Out.Uvs = input.uv;
    Out.WorldNormals = normalize(mul((float3x3) pc.model, input.normal));
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

/*
float3 getNormalFromMap()
{
    float3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;

    float3 Q1 = ddx(WorldPos);
    float3 Q2 = ddy(WorldPos);
    float2 st1 = ddx(TexCoords);
    float2 st2 = ddy(TexCoords);

    float3 N = normalize(Normal);
    float3 T = normalize(Q1 * st2.t - Q2 * st1.t);
    float3 B = -normalize(cross(N, T));
    float3x3 TBN = float3x3(T, B, N);

    return normalize(TBN * tangentNormal);
}
*/

FsOutput fs_main(VsToFs input)
{
    float4 tex_col = albedo.Sample(sSampler, input.Uvs, 1);

    FsOutput output;
    output.position = input.WorldPosition;
    output.albedo_m = float4(tex_col.rgb, 0);
    output.normal_r = float4(input.WorldNormals / 2 + 0.5, 1);
    if (tex_col.a < 0.9)
        discard;
    return output;
}