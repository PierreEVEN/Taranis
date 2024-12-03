
import shader_base;

extern static const bool PARAM_NORMAL_TEXTURE = true;
extern static const bool PARAM_METAL_ROUGHNESS_TEXTURE = true;

struct VSInput
{
    float3 pos;
    float2 uv;
    float3 normal;
    float3 tangent;
    float3 bitangents;
    float4 color;
};

struct SceneBufferData
{
    float4x4 perspective_view_mat;
    float4x4 view_mat;
    float4x4 perspective_mat;
    float4x4 inv_perspective_view_mat;
    float4x4 in_view_mat;
    float4x4 inv_perspective_mat;
};

struct VsToFs
{
    float4 Pos : SV_Position;
    float3 WorldPosition;
    float2 Uvs;
    float3 WorldNormals;
    float3 WorldTangents;
    float3 WorldBiTangents;
};

StructuredBuffer<SceneBufferData> scene_data_buffer;

SamplerState                      sSampler;
Texture2D                         albedo;
Texture2D mr_map;
Texture2D normal_map;

struct PushConsts
{
    float4x4 model;
};
// PushConsts pc

[shader("vertex")]
[RenderPass("gbuffers")]
VsToFs vertex_main(VSInput input, uniform PushConsts pc)
{
    VsToFs Out;
    Out.WorldPosition   = mul(pc.model, float4(input.pos, 1)).xyz;
    Out.Pos             = mul(scene_data_buffer.Load(0).perspective_view_mat, float4(Out.WorldPosition, 1));
    Out.Uvs             = input.uv;
    Out.WorldNormals = mul((float3x3)pc.model, input.normal);
    Out.WorldTangents = mul((float3x3)pc.model, input.tangent);
    Out.WorldBiTangents = mul((float3x3)pc.model, input.bitangents);
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
    float4 tex_col = albedo.Sample(sSampler, input.Uvs, 1);

    float2 mr           = float2(0, 1);
    float3 local_normal = float3(0, 0, 1);

    FsOutput output;

    if (PARAM_NORMAL_TEXTURE)
        local_normal = (normal_map.Sample(sSampler, input.Uvs, 1).rgb - 0.5) * 2;
    if (PARAM_METAL_ROUGHNESS_TEXTURE)
        mr = mr_map.Sample(sSampler, input.Uvs, 1).bg;
   
    
    float3 world_normal = normalize(input.WorldNormals);
    if (length(input.WorldTangents) > 0.001)
    {
        float3x3 TBN = float3x3(normalize(input.WorldTangents), normalize(input.WorldBiTangents), normalize(input.WorldNormals));
        world_normal = normalize(mul(local_normal, TBN));
    }

    output.position = float4(input.WorldPosition, 1);
    output.albedo_m = float4(tex_col.rgb, mr.r);
    output.normal_r = float4(world_normal / 2 + 0.5, mr.g);
    if (tex_col.a < 0.25)
        discard;
    return output;
}