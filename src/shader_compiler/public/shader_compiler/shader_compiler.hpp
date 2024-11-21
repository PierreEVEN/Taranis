#pragma once
#include "gfx_types/format.hpp"
#include "gfx_types/pipeline.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>


struct IDxcCompiler3;
struct IDxcIncludeHandler;
struct IDxcUtils;

namespace ShaderCompiler
{
class ShaderParser;

struct StageInputOutputDescription
{
    uint32_t              location;
    uint32_t              offset;
    Eng::Gfx::ColorFormat format;
};

struct BindingDescription
{
    BindingDescription(std::string in_name, uint32_t in_binding, Eng::Gfx::EBindingType in_type) : name(std::move(in_name)), binding(in_binding), type(in_type)
    {
    }

    std::string            name;
    uint32_t               binding;
    Eng::Gfx::EBindingType type;
};

struct CompiledStage
{
    Eng::Gfx::EShaderStage                   stage = Eng::Gfx::EShaderStage::Vertex;
    std::string                              entry_point;
    std::vector<BindingDescription>          bindings;
    std::vector<StageInputOutputDescription> stage_inputs;
    std::vector<StageInputOutputDescription> stage_outputs;
    uint32_t                                 push_constant_size = 0;
    std::vector<uint8_t>                     spirv;
};

struct CompiledPass
{
    Eng::Gfx::PipelineOptions                                 pipeline_config;
    std::unordered_map<Eng::Gfx::EShaderStage, CompiledStage> stages;
};

struct CompilationError
{
    CompilationError(std::string message) : message(std::move(message))
    {
    }

    std::string message;
    size_t      line = 0, column = 0;
};

struct CompilationResult
{
    std::vector<CompilationError>                          errors;
    std::unordered_map<Eng::Gfx::RenderPass, CompiledPass> compiled_passes;
};

class Compiler
{
public:
    struct Parameters
    {
        std::vector<std::string>              selected_passes = {};
        std::optional<std::filesystem::path>  source_path;
        std::unordered_map<std::string, bool> options_override = {};
        bool                                  b_debug          = false;
    };

    Compiler();
    std::optional<CompilationError> compile_raw(const ShaderParser& parser, const Parameters& params, CompilationResult& compilation_result) const;

private:
    std::optional<CompilationError> compile_pass(const std::string& pass, const ShaderParser& parser, const Parameters& params, CompilationResult& compilation_result) const;

    std::optional<CompilationError> compile_stage(Eng::Gfx::EShaderStage stage, const std::string& code, const std::string& entry_point, const Parameters& params, CompiledStage& result) const;


    static std::optional<CompilationError> extract_spirv_properties(CompiledStage& properties);

    IDxcIncludeHandler* include_handler;
    IDxcCompiler3*      compiler = nullptr;
    IDxcUtils*          utils    = nullptr;
};
}