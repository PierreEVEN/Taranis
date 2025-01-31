
import shader_base;

struct VSInput
{
    float3 pos;
    float2 uv;
    float3 normal;
};

struct VsToFs
{
    float4 Pos : SV_Position;
    float3 WorldPosition;
    float2 Uvs;
    float3 WorldNormals;
};

StructuredBuffer<SceneBufferData> scene_data_buffer;

struct PushConsts
{
    float4x4 model;
};

[shader("vertex")]
[RenderPass("gbuffers")]
VsToFs vertex_main(VSInput input, uniform PushConsts pc)
{
    VsToFs Out;
    Out.WorldPosition = mul(pc.model, float4(input.pos, 1)).xyz;
    Out.Pos = mul(scene_data_buffer.Load(0).perspective_view_mat, float4(Out.WorldPosition, 1));
    Out.Uvs = input.uv;
    Out.WorldNormals = mul((float3x3)pc.model, input.normal);
    return Out;
}
[shader("vertex")]
[RenderPass("shadows")]
float4 vertex_main_shadows(VSInput input, uniform PushConsts pc) : SV_Position
{
    float3 WorldPosition = mul(pc.model, float4(input.pos, 1)).xyz;
    return mul(scene_data_buffer.Load(0).perspective_view_mat, float4(WorldPosition, 1));
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
    output.position = float4(input.WorldPosition, 1);
    output.albedo_m = float4(0.4, 0.2, 0.3, 0);
    output.normal_r = float4(normalize(input.WorldNormals), 0);
    return output;
}