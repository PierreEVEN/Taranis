
import shader_base;

#define PI 3.14159265358979323846

// ----------------------------------------------------------------------------
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom       = PI * denom * denom;

    return nom / denom;
}

// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

// ----------------------------------------------------------------------------
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// ----------------------------------------------------------------------------
float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ----------------------------------------------------------------------------
float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ----------------------------------------------------------------------------

struct VsToFsStruct
{
    float4 Pos : SV_Position;
    float2 Uv;
};

SamplerState sSampler;
Texture2D                       gbuffer_position;
Texture2D                       gbuffer_albedo_m;
Texture2D                       gbuffer_normal_r;
Texture2D gbuffer_depth;

struct LightData {
    float3 dir;
    float3 pos;
    uint32_t type;
    bool has_shadows;
}
StructuredBuffer<LightData> light_buffer;

Texture2D shadow_maps[];
struct PushConsts
{
    float3 camera;
    uint32_t light_count;
};

[shader("vertex")]
[RenderPass("gbuffer_resolve")]
VsToFsStruct vs_main(uint vertex_index : SV_VertexID)
{
    VsToFsStruct output;
    output.Uv  = float2(vertex_index << 1 & 2, vertex_index & 2);
    output.Pos = float4(output.Uv * 2.0f - 1.0f, 0.0f, 1.0f);
    output.Uv *= float2(1, -1);
    return output;
}

[shader("fragment")]
[RenderPass("gbuffer_resolve")]
float4 fs_main(VsToFsStruct input, uniform PushConsts pc)
{
    float3 worldPosition = gbuffer_position.Sample(sSampler, input.Uv).xyz;
    float4 albedo_m      = gbuffer_albedo_m.Sample(sSampler, input.Uv);
    float4 normal_r = gbuffer_normal_r.Sample(sSampler, input.Uv);

    float3 albedo   = albedo_m.rgb;
    float  metallic = albedo_m.a;

    float3 normal    = normalize((normal_r.rgb - 0.5) * 2);
    float  roughness = normal_r.a;

    float3 cam_pos = pc.camera;

    float3 ambiant_intensity = float3(0.1, 0.1, 0.1) * 0.01;
    float3 light_color       = float3(1, 1, 1);

    //float ao = texture(aoMap, TexCoords).r;

    float3 N = normalize(normal);
    float3 V = normalize(cam_pos - worldPosition);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0        = lerp(F0, albedo, metallic);

    // reflectance equation
    float3 Lo = float3(0, 0, 0);


    for (uint32_t i = 0; i < pc.light_count; ++i) {

        LightData light = light_buffer.Load(i);

        if (light.has_shadows || true) {
            float3 sm = shadow_maps[i].Sample(sSampler, input.Uv).xyz;
            if (sm.b < 10)
                return float4(sm.r, 0 , 0, 1);
            // @TODO
        }

        // calculate per-light radiance
        float3 L = normalize(light.dir);
        float3 H = normalize(V + L);
        //float distance = length(lightPositions[i] - WorldPos);
        //float attenuation = 1.0 / (distance * distance);
        float3 radiance = light_color;

        // Cook-Torrance BRDF
        float  NDF = DistributionGGX(N, H, roughness);
        float  G   = GeometrySmith(N, V, L, roughness);
        float3 F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

        float3 numerator   = NDF * G * F;
        float  denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        float3 specular    = numerator / denominator;

        // kS is equal to Fresnel
        float3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        float3 kD = float3(1, 1, 1) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }

    // ambient lighting (note that the next IBL tutorial will replace 
    // this ambient lighting with environment lighting).
    float3 ambient = ambiant_intensity * albedo; // * ao;

    float3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + float3(1, 1, 1));
    // gamma correct
    color = pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));

    return float4(color, 1);
}