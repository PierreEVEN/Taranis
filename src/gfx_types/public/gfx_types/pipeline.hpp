#pragma once
#include <algorithm>
#include <ranges>
#include <string>
#include <ankerl/unordered_dense.h>
#include <vector>

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
    Fragment = 0x00000010,
    Compute  = 0x00000020,
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
    ECulling     culling    = ECulling::Back;
    EFrontFace   front_face = EFrontFace::CounterClockwise;
    ETopology    topology   = ETopology::Triangles;
    EPolygonMode polygon    = EPolygonMode::Fill;
    EAlphaMode   alpha      = EAlphaMode::Opaque;
    bool         depth_test = true;
    float        line_width = 1.0f;
};

class PermutationGroup
{
  public:
    ankerl::unordered_dense::map<std::string, bool> permutation_group;
};

class PermutationDescription
{
  public:
    PermutationDescription() = default;

    PermutationDescription(const PermutationGroup& group)
    {
        for (const auto& item : group.permutation_group | std::views::keys)
            switch_names.emplace_back(item);
        std::ranges::sort(switch_names);
        for (const auto& item : group.permutation_group)
            set(item.first, item.second);
    }

    bool operator==(const PermutationDescription& other) const
    {
        return other.bit_mask == bit_mask;
    }

    bool get(const std::string& key) const
    {
        for (size_t i = 0; i < switch_names.size(); ++i)
        {
            if (switch_names[i] == key)
                return bit_mask & 1ull << i;
        }
        return false;
    }

    void set(const std::string& key, bool value)
    {
        for (size_t i = 0; i < switch_names.size(); ++i)
        {
            if (switch_names[i] == key)
            {
                if (value)
                    bit_mask |= 1ull << i;
                else
                    bit_mask &= ~(1ull << i);
                return;
            }
        }
    }

    const std::vector<std::string>& keys() const
    {
        return switch_names;
    };

    uint64_t bits() const
    {
        return bit_mask;
    }

  private:
    std::vector<std::string> switch_names;
    uint64_t                 bit_mask = 0;
};
} // namespace Eng::Gfx

template <> struct std::hash<Eng::Gfx::PermutationDescription>
{
    size_t operator()(const Eng::Gfx::PermutationDescription& val) const noexcept
    {
        return std::hash<uint64_t>()(val.bits());
    }
};