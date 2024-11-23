#pragma once
#include "gfx_types/format.hpp"
#include "gfx_types/pipeline.hpp"

#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace slang
{
struct ISession;
struct IModule;
struct IGlobalSession;
}

namespace ShaderCompiler
{
class Compiler;
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

class Session
{
  public:
    void compile(const std::filesystem::path& path);

    ~Session();
  private:
    friend class Compiler;
    Session(Compiler* in_compiler);

    Compiler* compiler;

    slang::ISession* session = nullptr;
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

    std::optional<CompilationError> compile_raw(const ShaderParser& parser, const Parameters& params, CompilationResult& compilation_result) const;

    std::optional<CompilationError> pre_compile_shader(const std::filesystem::path& path);

    static Compiler& get();

    ~Compiler();

    std::shared_ptr<Session> create_session();

private:
    friend Session;

    slang::IModule* load_module(const std::filesystem::path& module_path);

    slang::IGlobalSession* global_session = nullptr;

    Compiler();
};
}