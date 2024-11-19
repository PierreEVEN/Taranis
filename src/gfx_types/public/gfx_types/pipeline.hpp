#pragma once
#include <string>

namespace Eng::Gfx
{
using RenderPass = std::string;

enum class EBindingType
{
    SAMPLER,
    COMBINED_IMAGE_SAMPLER,
    SAMPLED_IMAGE,
    STORAGE_IMAGE,
    UNIFORM_TEXEL_BUFFER,
    STORAGE_TEXEL_BUFFER,
    UNIFORM_BUFFER,
    STORAGE_BUFFER,
    UNIFORM_BUFFER_DYNAMIC,
    STORAGE_BUFFER_DYNAMIC,
    INPUT_ATTACHMENT
};

enum class EShaderStage
{
    Vertex   = 0x00000001,
    Fragment = 0x00000010
};
}