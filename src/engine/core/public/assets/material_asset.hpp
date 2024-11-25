#pragma once
#include "asset_base.hpp"
#include "assets\material_asset.gen.hpp"
#include "gfx/vulkan/pipeline.hpp"
#include "gfx_types/pipeline.hpp"

#include <filesystem>

namespace ShaderCompiler
{
class Session;
}

namespace Eng::Gfx
{
class DescriptorSet;
class Pipeline;
} // namespace Eng::Gfx

namespace Eng
{
struct MaterialPermutation
{
    MaterialPermutation(MaterialAsset* owner, Gfx::PermutationDescription permutation_desc);

    const std::shared_ptr<Eng::Gfx::Pipeline>& get_resource(const std::string& render_pass);

private:
    struct PassInfos
    {
        std::unordered_map<Gfx::EShaderStage, std::vector<uint8_t>> per_stage_code;
        std::shared_ptr<Gfx::Pipeline>                              pipeline;
    };

    std::unordered_map<std::string, PassInfos> passes;
    MaterialAsset*                             owner = nullptr;
    Gfx::PermutationDescription                permutation_description;
};

class MaterialAsset : public AssetBase
{
    REFLECT_BODY()

public:
    MaterialAsset();

    void set_shader_code(const std::filesystem::path& code);

    Gfx::PermutationDescription get_default_permutation() const
    {
        return Gfx::PermutationDescription(default_permutation);
    }

    std::weak_ptr<MaterialPermutation> get_permutation(const Gfx::PermutationDescription& permutation);

private:
    friend class MaterialPermutation;
    Gfx::PermutationDescription default_permutation;

    std::unordered_map<Gfx::PermutationDescription, std::shared_ptr<MaterialPermutation>> permutations;

    std::shared_ptr<ShaderCompiler::Session> compiler_session;

    Gfx::PipelineOptions options;
};
} // namespace Eng