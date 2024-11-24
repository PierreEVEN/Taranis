#include "assets/material_asset.hpp"

#include "engine.hpp"
#include "profiler.hpp"
#include "gfx/vulkan/descriptor_sets.hpp"
#include "gfx/vulkan/shader_module.hpp"
#include "shader_compiler/shader_compiler.hpp"
#include "shader_compiler/shader_parser.hpp"

namespace Eng
{
MaterialAsset::MaterialAsset() = default;

void MaterialAsset::set_shader_code(const std::filesystem::path& code)
{
    permutations.clear();
    compiler_session    = ShaderCompiler::Compiler::get().create_session(code);
    default_permutation = compiler_session->get_default_permutations_description();
}

std::weak_ptr<MaterialPermutation> MaterialAsset::get_permutation(const Gfx::PermutationDescription& permutation)
{
    if (auto found = permutations.find(permutation); found != permutations.end())
        return found->second;
    return permutations.emplace(permutation, std::make_shared<MaterialPermutation>(compiler_session, permutation)).first->second;
}

MaterialPermutation::MaterialPermutation(const std::shared_ptr<ShaderCompiler::Session>& in_compiler_session, const Gfx::PermutationDescription& permutation_desc) : compiler_session(in_compiler_session),
    permutation_description(permutation_desc)
{
}

const std::shared_ptr<Gfx::Pipeline>& MaterialPermutation::get_resource(const std::string& render_pass)
{
    if (auto found = passes.find(render_pass); found != passes.end())
        return found->second.pipeline;

    auto compilation_result = compiler_session->compile(render_pass, permutation_description);

    if (!compilation_result.errors.empty())
    {
        std::string error_message = "Failed to compile shader:";
        for (const auto& error : compilation_result.errors)
            error_message += "\n" + error.message;
        LOG_ERROR(error_message.c_str());

        PassInfos infos;
        return passes.emplace(render_pass, infos).first->second.pipeline;
    }

    auto device = Engine::get().get_device();

    auto render_pass_object = device.lock()->get_render_pass(render_pass);
    if (!render_pass_object.lock())
        LOG_ERROR("There is no render pass named {}", render_pass);

    std::vector<std::shared_ptr<Gfx::ShaderModule>> modules;

    PassInfos infos;
    for (const auto& stage : compilation_result.stages)
    {
        infos.per_stage_code.emplace(stage.first, stage.second.compiled_module);
        modules.emplace_back(Gfx::ShaderModule::create(device, ));
    }
    
    auto pipeline = Gfx::Pipeline::create("TODOOO", device, render_pass_object, modules, Gfx::Pipeline::CreateInfos{});
    infos.pipeline = pipeline;
    return passes.emplace(render_pass, infos).first->second.pipeline;
};
}