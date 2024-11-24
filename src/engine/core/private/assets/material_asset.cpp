#include "assets/material_asset.hpp"

#include "engine.hpp"
#include "profiler.hpp"
#include "gfx/vulkan/descriptor_sets.hpp"
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
    return passes.emplace(render_pass, {}).first->second.pipeline;
};
}