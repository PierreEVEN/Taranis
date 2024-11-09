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
    float2 Uvs : TEXCOORD0;
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
    Out.Pos    = mul(pc.camera, float4(input.pos, 1));
    Out.Uvs = input.uv;
    return Out;
}

float pow_cord(float val)
{
    return pow(abs(sin(val * 50)), 10);
}

float4 fs_main(VsToFs input) : SV_TARGET
{
    return albedo.Sample(sSampler, input.Uvs, 1);
}