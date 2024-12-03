#include "assets/material_asset.hpp"

#include "engine.hpp"
#include "gfx/vulkan/descriptor_sets.hpp"
#include "gfx/vulkan/shader_module.hpp"
#include "profiler.hpp"
#include "shader_compiler/shader_compiler.hpp"

namespace Eng
{
MaterialAsset::MaterialAsset() = default;

void MaterialAsset::set_shader_code(const std::filesystem::path& code, const std::optional<std::vector<StageInputOutputDescription>>& vertex_input_override)
{
    std::unique_lock lk(pipeline_mutex);
    PROFILER_SCOPE_NAMED(LoadMat, "Load material from path " + code.string());
    if (vertex_input_override)
        vertex_inputs = *vertex_input_override;
    shader_virtual_path = code;

    permutations.clear();
    compiler_session = ShaderCompiler::Compiler::get().create_session(code);

    if (auto path = compiler_session->get_filesystem_path())
    {
        shader_real_path = *path;
        if (!exists(shader_real_path))
            LOG_ERROR("The file {} does not exists", shader_real_path.string());
        last_update = last_write_time(shader_real_path);
    }

    default_permutation = compiler_session->get_default_permutations_description();
}

std::weak_ptr<MaterialPermutation> MaterialAsset::get_permutation(const Gfx::PermutationDescription& permutation)
{
    {
        std::shared_lock lk(pipeline_mutex);
        if (auto found = permutations.find(permutation); found != permutations.end())
            return found->second;
    }
    std::unique_lock lk(pipeline_mutex);
    return permutations.emplace(permutation, std::make_shared<MaterialPermutation>(this, permutation)).first->second;
}

void MaterialAsset::update_options(const Gfx::PipelineOptions& in_options)
{
    options = in_options;
    set_shader_code(shader_virtual_path);
}

void MaterialAsset::check_for_updates()
{
    if (scan_source_updates && exists(shader_real_path))
    {
        if (last_update < last_write_time(shader_real_path))
            set_shader_code(shader_virtual_path);
    }
}

MaterialPermutation::MaterialPermutation(MaterialAsset* in_owner, Gfx::PermutationDescription permutation_desc) : owner(in_owner), permutation_description(std::move(permutation_desc))
{
}

std::shared_ptr<Gfx::Pipeline> MaterialPermutation::get_resource(const std::string& render_pass)
{
    {
        std::shared_lock lk(owner->pipeline_mutex);
        if (auto found = passes.find(render_pass); found != passes.end())
            return found->second.pipeline;
    }

    std::unique_lock lk(owner->pipeline_mutex);
    if (!owner->compiler_session)
        return nullptr;

    PROFILER_SCOPE_NAMED(CompileShader, std::string("Compile material '") + owner->get_name() + "' for render pass " + render_pass);
    auto compilation_result = owner->compiler_session->compile(render_pass, permutation_description);

    if (!compilation_result.errors.empty())
    {
        std::string error_message = "Failed to compile shader:";
        for (const auto& error : compilation_result.errors)
            error_message += "\n" + error.message;
        LOG_ERROR("{}", error_message);

        PassInfos infos;
        return passes.emplace(render_pass, infos).first->second.pipeline;
    }

    auto device = Engine::get().get_device();

    auto render_pass_object = device.lock()->get_render_pass(render_pass);
    if (!render_pass_object.lock())
    {
        LOG_ERROR("There is no render pass named {}", render_pass);
    }

    PassInfos infos;

    std::vector<std::shared_ptr<Gfx::ShaderModule>> modules;

    for (const auto& stage : compilation_result.stages)
    {
        infos.per_stage_code.emplace(stage.first, stage.second.compiled_module);
        modules.emplace_back(Gfx::ShaderModule::create(device, stage.second));
    }
    if (!modules.empty() && render_pass_object.lock())
        infos.pipeline = Gfx::Pipeline::create(owner->get_name(), device, render_pass_object, modules, Gfx::Pipeline::CreateInfos{.options = owner->options, .vertex_inputs = owner->vertex_inputs});

    return passes.emplace(render_pass, infos).first->second.pipeline;
};
} // namespace Eng