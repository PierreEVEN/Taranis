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
