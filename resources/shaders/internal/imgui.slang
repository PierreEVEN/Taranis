import shader_base;

struct VSInput {
    float2 aPos;
    float2 aUV;
    float4 aColor;
};

struct VsToFs {
    float4 Pos : SV_Position;
    float4 Color;
    float2 UV;
};

struct PushConsts {
    float2 uScale;
    float2 uTranslate;
};

[shader("vertex")]
[RenderPass("present_pass")]
VsToFs vs_main(VSInput input, uniform PushConsts pc) {
    VsToFs Out;
    Out.Color = input.aColor;
    Out.UV = input.aUV;
    Out.Pos = float4(input.aPos * pc.uScale + pc.uTranslate, 0, 1);
    return Out;
}

Texture2D sTexture;
SamplerState sSampler;

[shader("fragment")]
[RenderPass("present_pass")]
float4 fs_main(VsToFs input) {
    return input.Color * sTexture.Sample(sSampler, input.UV);
}