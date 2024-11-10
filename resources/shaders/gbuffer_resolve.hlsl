
struct VsToFsStruct
{
    float4 Pos : SV_Position;
    float2 Uv : TEXCOORD;
};

[[vk::binding(0)]] SamplerState sSampler;
Texture2D gbuffer_position;
Texture2D gbuffer_albedo_m;
Texture2D gbuffer_normal_r;
Texture2D gbuffer_depth;


VsToFsStruct vs_main(uint vertex_index : SV_VertexID)
{
    VsToFsStruct output;
    output.Uv = float2((vertex_index << 1) & 2, vertex_index & 2);
    output.Pos = float4(output.Uv * 2.0f - 1.0f, 0.0f, 1.0f);
    output.Uv *= float2(1, -1);
    return output;
}

float4 fs_main(VsToFsStruct input) : SV_TARGET
{
    float4 albedo_m = gbuffer_albedo_m.Sample(sSampler, input.Uv);
    float4 gbuffer_normal_r = gbuffer_albedo_m.Sample(sSampler, input.Uv);

    float3 albedo = albedo_m.rgb;
    float metalness = albedo_m.a;
    
    float3 normal = gbuffer_normal_r.rgb;
    float roughness = gbuffer_normal_r.a;

    float3 light_color = float3(10, 10, 10);
    float3 light_pos = float3(-10, 0, 0);

    float3 frag_pos = float3(0, 0, 0); // @TODO
    float3 world_normal = float3(1, 0, 0); // @TODO
    float3 cam_pos = float3(-10, 0, 0); // @TODO
		
    return float4(albedo, 1);
}