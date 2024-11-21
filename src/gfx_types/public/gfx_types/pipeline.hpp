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
    Vertex = 0x00000001,
    Fragment = 0x00000010,
    Compute = 0x00000020,
};

enum class ECulling
{
    None,
    Front,
    Back,
    Both
};

enum class EFrontFace
{
    Clockwise,
    CounterClockwise,
};

enum class ETopology
{
    Points,
    Lines,
    Triangles,
};

enum class EPolygonMode
{
    Point,
    Line,
    Fill,
};

enum class EAlphaMode
{
    Opaque,
    Translucent,
    Additive
};

struct PipelineOptions
{
    ECulling     culling      = ECulling::Back;
    EFrontFace   front_face   = EFrontFace::CounterClockwise;
    ETopology    topology     = ETopology::Triangles;
    EPolygonMode polygon = EPolygonMode::Fill;
    EAlphaMode   alpha   = EAlphaMode::Opaque;
};
}