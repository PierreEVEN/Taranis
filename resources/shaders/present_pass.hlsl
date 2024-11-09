
struct VsToFsStruct
{
    float4 Pos : SV_Position;
    float2 Uv : TEXCOORD;
};

[[vk::binding(0)]] SamplerState sSampler;
[[vk::binding(1)]] Texture2D albedo;

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
    return albedo.Sample(sSampler, input.Uv);
}