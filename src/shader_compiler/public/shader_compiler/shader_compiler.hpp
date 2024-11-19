#pragma once
#include "gfx_types/format.hpp"
#include "gfx_types/pipeline.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct IDxcCompiler3;
struct IDxcIncludeHandler;
struct IDxcUtils;

namespace std::filesystem
{
class path;
}

namespace Eng::Gfx
{
struct StageInputOutputDescription
{
    uint32_t    location;
    uint32_t    offset;
    ColorFormat format;
};

struct BindingDescription
{
    BindingDescription(std::string in_name, uint32_t in_binding, EBindingType in_type) : name(std::move(in_name)), binding(in_binding), type(in_type)
    {
    }

    std::string  name;
    uint32_t     binding;
    EBindingType type;
};

struct ShaderProperties
{
    EShaderStage                             stage       = EShaderStage::Vertex;
    std::string                              entry_point = "main";
    std::vector<BindingDescription>          bindings;
    std::vector<StageInputOutputDescription> stage_inputs;
    std::vector<StageInputOutputDescription> stage_outputs;
    uint32_t                                 push_constant_size = 0;
    std::vector<uint8_t>                     spirv;
    std::vector<std::string>                 features = {};
};

struct CompilationError
{
    std::string error_message;
    int64_t     line = -1, column = -1;
};

struct CompilationResult
{
    std::vector<CompilationError>                    errors;
    std::unordered_map<RenderPass, ShaderProperties> compiled_passes;
};

struct RenderPassSources
{
    std::string                                   raw;
    std::unordered_map<EShaderStage, std::string> stage_inputs;

    RenderPassSources& operator+=(const RenderPassSources& other)
    {
        raw += other.raw;
        for (const auto& input : other.stage_inputs)
            stage_inputs.emplace(input.first, input.second);

        return *this;
    }
};

class ShaderSource
{
public:
    ShaderSource(const std::string& raw);

    const std::vector<CompilationError>& get_errors() const
    {
        return parse_errors;
    }

    const RenderPassSources& get_render_pass(const RenderPass& render_pass)
    {
        if (const auto found = render_passes.find(render_pass); found != render_passes.end())
            return found->second;
        return {};
    }

private:
    std::vector<CompilationError>                     parse_errors;
    std::unordered_map<std::string, bool>             options;
    std::unordered_map<RenderPass, RenderPassSources> render_passes;
};

class ShaderCompiler
{
public:
    ShaderCompiler();

    CompilationResult compile_raw(const std::string& raw, const std::string& entry_point, EShaderStage shader_stage, const std::filesystem::path& path, std::vector<std::pair<std::string, bool>> options = {},
                                  bool               b_debug                                                                                                                                              = false) const;

private:
    static std::optional<std::string> extract_spirv_properties(ShaderProperties& properties);

    IDxcIncludeHandler* include_handler;
    IDxcCompiler3*      compiler = nullptr;
    IDxcUtils*          utils    = nullptr;
};
} // namespace Eng::Gfx