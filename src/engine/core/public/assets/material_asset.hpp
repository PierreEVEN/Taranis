#pragma once
#include "asset_base.hpp"
#include "gfx/vulkan/pipeline.hpp"
#include "gfx_types/pipeline.hpp"

#include <filesystem>

#include <shared_mutex>

#include "assets/material_asset.gen.hpp"

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
    ~MaterialPermutation();
    std::shared_ptr<Gfx::Pipeline> get_resource(const Gfx::RenderPassRef& render_pass);

private:
    struct PassInfos
    {
        ankerl::unordered_dense::map<Gfx::EShaderStage, std::vector<uint8_t>> per_stage_code;
        std::shared_ptr<Gfx::Pipeline>                                        pipeline;
    };

    ankerl::unordered_dense::map<Gfx::RenderPassRef, PassInfos> passes;
    MaterialAsset*                                              owner = nullptr;
    Gfx::PermutationDescription                                 permutation_description;
};

class MaterialAsset : public AssetBase
{
    REFLECT_BODY()

public:
    MaterialAsset();

    void set_shader_code(const std::filesystem::path& code, const std::optional<std::vector<StageInputOutputDescription>>& vertex_input_override = {});

    Gfx::PermutationDescription get_default_permutation() const
    {
        return Gfx::PermutationDescription(default_permutation);
    }

    std::weak_ptr<MaterialPermutation> get_permutation(const Gfx::PermutationDescription& permutation);

    void update_options(const Gfx::PipelineOptions& options);

    void check_for_updates();

    glm::vec3 asset_color() const override
    {
        return {0.5, 1, 0.7};
    }

private:
    friend MaterialPermutation;
    Gfx::PermutationDescription default_permutation;

    ankerl::unordered_dense::map<Gfx::PermutationDescription, std::shared_ptr<MaterialPermutation>> permutations;

    bool                  scan_source_updates = true;
    std::filesystem::path shader_real_path;

    std::filesystem::file_time_type last_update;

    std::shared_ptr<ShaderCompiler::Session> compiler_session;
    std::filesystem::path                    shader_virtual_path;
    std::vector<StageInputOutputDescription> vertex_inputs;
    std::shared_mutex                        pipeline_mutex;
    Gfx::PipelineOptions                     options;
};
} // namespace Eng
