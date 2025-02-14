
import shader_base;

struct VSInput
{
    float3 pos;
    float3 color;
};

struct VsToFs
{
    float4 Pos : SV_Position;
    float3 Color;
};

StructuredBuffer<SceneBufferData> scene_data_buffer;

[shader("vertex")]
[RenderPass("gbuffers")]
VsToFs vertex_main(VSInput input)
{
    VsToFs Out;
    Out.Pos = mul(scene_data_buffer.Load(0).perspective_view_mat, float4(input.pos, 1));
    Out.Color = input.color;
    return Out;
}

struct FsOutput
{
    float4 position;
    float4 albedo_m;
    float4 normal_r;
};


[shader("fragment")]
[RenderPass("gbuffers")]
FsOutput fragment_main(VsToFs input)
{
    FsOutput output;
    output.position = float4(0.000001, 0, 0, 1);
    output.albedo_m = float4(input.Color.rgb, 1);
    output.normal_r = float4(0, 0, 0, 1);
    return output;
}