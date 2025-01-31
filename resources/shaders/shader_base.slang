#pragma once


[__AttributeUsage(_AttributeTargets.Function)]
struct RenderPassAttribute
{
    string render_pass;
};

[__AttributeUsage(_AttributeTargets.Var)]
struct DefaultEnabledAttribute
{
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