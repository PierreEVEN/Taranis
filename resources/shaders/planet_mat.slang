
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
    float4 color;
};

StructuredBuffer<SceneBufferData> scene_data_buffer;

struct PushConsts
{
    uint test;
};

float3 HUEtoRGB(in float H)
{
    float R = abs(H * 6 - 3) - 1;
    float G = 2 - abs(H * 6 - 2);
    float B = 2 - abs(H * 6 - 4);
    return saturate(float3(R, G, B));
}

float3 HSVtoRGB(in float3 HSV)
{
    float3 RGB = HUEtoRGB(HSV.x);
    return ((RGB - 1) * HSV.y + 1) * HSV.z;
}

[shader("vertex")]
[RenderPass("gbuffers")]
VsToFs vertex_main(VSInput input, uniform PushConsts pc)
{
    VsToFs Out;
    Out.WorldPosition = input.pos;
    Out.Pos = mul(scene_data_buffer.Load(0).perspective_view_mat, float4(Out.WorldPosition, 1));
    Out.Uvs = input.uv;
    Out.WorldNormals = input.normal;
    Out.color = float4(HSVtoRGB(float3(fmod(pc.test * 0.08789, 1), 1, 0.3)) + float3(0.5, 0.3, 0.6), 1.0);
    return Out;
}
[shader("vertex")]
[RenderPass("shadows")]
float4 vertex_main_shadows(VSInput input, uniform PushConsts pc) : SV_Position
{
    float3 WorldPosition = input.pos;
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
    output.albedo_m = float4(input.color.rgb, 0.0);//float4(0.4, 0.2, 0.3, 0);
    output.normal_r = float4(normalize(input.WorldNormals), 0);
    return output;
}