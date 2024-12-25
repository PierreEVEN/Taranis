
import shader_base;

[DefaultEnabled]
extern static const bool PARAM_ALBEDO_TEXTURE;

[DefaultEnabled]
extern static const bool PARAM_NORMAL_TEXTURE;

[DefaultEnabled]
extern static const bool PARAM_METAL_ROUGHNESS_TEXTURE;

struct VSInput
{
    float3 pos;
    float2 uv;
    float3 normal;
    float3 tangent;
    float3 bitangents;
    float4 color;
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

struct SceneBufferData
{
    float4x4 perspective_view_mat;
    float4x4 view_mat;
    float4x4 perspective_mat;
    float4x4 inv_perspective_view_mat;
    float4x4 in_view_mat;
    float4x4 inv_perspective_mat;
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

[shader("vertex")]
[RenderPass("gbuffers")]
VsToFs vertex_main(VSInput input, uniform PushConsts pc)
{
    VsToFs Out;
    Out.WorldPosition   = mul(pc.model, float4(input.pos, 1)).xyz;
    Out.Pos = mul(scene_data_buffer.Load(0).perspective_view_mat, float4(Out.WorldPosition, 1));
    Out.Uvs = input.uv;
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


float4 UnpackBc5NormalMap(float4 TextureSample)
{

    float2 NormalXY = TextureSample.rg;

    NormalXY = NormalXY * float2(2.0f, 2.0f) - float2(1.0f, 1.0f);
    float NormalZ = sqrt(saturate(1.0f - dot(NormalXY, NormalXY)));
    return float4(NormalXY.xy, NormalZ, 1.0f);
}


[shader("fragment")]
[RenderPass("gbuffers")]
FsOutput fragment_main(VsToFs input)
{
    // @TODO handle flipped textures
    input.Uvs = float2(input.Uvs.x, -input.Uvs.y);
    float4 tex_col = float4(1, 1, 1, 1);

    if (PARAM_ALBEDO_TEXTURE)
        tex_col = albedo.Sample(sSampler, input.Uvs, 1);

    float2 mr           = float2(0, 1);
    float3 local_normal = float3(0, 0, 1);

    FsOutput output;

    if (PARAM_NORMAL_TEXTURE) {
        float4 normal = normal_map.Sample(sSampler, input.Uvs, 1);
        if (normal.z == 0) {
            local_normal = UnpackBc5NormalMap(normal).rgb;
        }
        else
            local_normal = (normal.rgb - 0.5) * 2;
    }
    if (PARAM_METAL_ROUGHNESS_TEXTURE)
        mr = mr_map.Sample(sSampler, input.Uvs, 1).bg;
   
    
    float3 world_normal = normalize(input.WorldNormals);

    if (length(input.WorldTangents) < 0.001)
    {
        if (abs(dot(input.WorldNormals, float3(1, 0, 0))) > 0.8)
            input.WorldTangents = float3(0, 1, 0);
        else
            input.WorldTangents = float3(1, 0, 0);
        input.WorldBiTangents = normalize(cross(input.WorldNormals, input.WorldTangents));
        input.WorldTangents = normalize(cross(input.WorldNormals, input.WorldBiTangents));
    }
    float3x3 TBN = float3x3(normalize(input.WorldTangents), normalize(input.WorldBiTangents), normalize(input.WorldNormals));
    world_normal = normalize(mul(local_normal, TBN));
    

    output.position = float4(input.WorldPosition, 1);
    output.albedo_m = float4(tex_col.rgb, mr.r);
    output.normal_r = float4(world_normal / 2 + 0.5, mr.g);
    if (tex_col.a < 0.25)
        discard;
    return output;
}