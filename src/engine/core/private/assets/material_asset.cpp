#include "assets/material_asset.hpp"

#include "engine.hpp"
#include "profiler.hpp"
#include "gfx/vulkan/descriptor_sets.hpp"
#include "shader_compiler/shader_compiler.hpp"
#include "shader_compiler/shader_parser.hpp"

namespace Eng
{
MaterialAsset::MaterialAsset() = default;

void MaterialAsset::set_shader_code(const std::string& code)
{
    pipelines.clear();
    compiled_passes.clear();
    shader_code = code;
    if (!parser)
        parser = std::make_unique<ShaderCompiler::ShaderParser>(shader_code);

    // Update options
    std::vector<std::string> outdated_options;
    for (const auto& name : options | std::views::keys)
        if (!parser->get_default_options().contains(name))
            outdated_options.emplace_back(name);

    for (const auto& outdated : outdated_options)
        options.erase(outdated);

    for (const auto& option : parser->get_default_options())
        if (!options.contains(option.first))
            options.emplace(option.first, option.second);
}

const std::shared_ptr<Gfx::Pipeline>& MaterialAsset::get_resource(const std::string& render_pass)
{
    if (auto found = pipelines.find(render_pass); found != pipelines.end())
        return found->second;

    PROFILER_SCOPE_NAMED(CompilePass, "Compile pipeline pass " + render_pass);
    compile_pass(render_pass);

    if (auto found = pipelines.find(render_pass); found != pipelines.end())
        return found->second;
    return {};
}

void MaterialAsset::compile_pass(const std::string& render_pass)
{
    if (!compiled_passes.contains(render_pass))
    {
        if (!parser)
            parser = std::make_unique<ShaderCompiler::ShaderParser>(shader_code);

        if (parser->get_error())
            return;

        ShaderCompiler::Compiler          compiler;
        ShaderCompiler::CompilationResult result;

        ShaderCompiler::Compiler::Parameters params{
            .selected_passes  = {render_pass},
            .source_path      = {},
            .options_override = options,
            .b_debug          = false,
        };

        if (auto error = compiler.compile_raw(*parser, {}, result))
        {
            std::cerr << "Compilation failed " << error->line << ":" << error->column << " : " << error->message << std::endl;
            return;
        }
    }

    if (auto found = compiled_passes.find(render_pass); found != compiled_passes.end())
    {
        pipelines.emplace(render_pass, Gfx::Pipeline::create(get_name(), Engine::get().get_device(), , , ));
    }
    else
    {
        pipelines.emplace(render_pass, nullptr);
    }
}
}