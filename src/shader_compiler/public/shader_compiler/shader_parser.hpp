#pragma once
#include "gfx_types/pipeline.hpp"
#include "llp/lexer.hpp"

#include <string>
#include <unordered_map>

namespace ShaderCompiler
{
struct EntryPoint
{
    std::string            name;
    Eng::Gfx::EShaderStage stage;
};

struct ShaderBlock
{
    std::vector<EntryPoint> entry_point;
    std::string             raw_code;
    Llp::Location           start;
    Llp::Location           end;
};

class ShaderParser
{
public:
    ShaderParser(const std::string& source_shader);

    std::optional<Llp::ParserError> get_error() const
    {
        return error;
    }

    const std::unordered_map<std::string, std::vector<std::shared_ptr<ShaderBlock>>>& get_passes() const
    {
        return passes;
    }

    const Eng::Gfx::PipelineOptions& get_pipeline_config() const
    {
        return pipeline_options;
    }

    const std::unordered_map<std::string, bool>& get_default_options() const
    {
        return options;
    }

private:
    std::optional<Llp::ParserError>        parse();
    static std::optional<Llp::ParserError> parse_pass_args(Llp::ArgumentsToken& args, std::vector<std::string>& pass_list);
    std::optional<std::string>             parse_config_value(const std::string& key, const std::string& value);

    static std::optional<Llp::ParserError> parse_block(const Llp::BlockToken& args, ShaderBlock& block);

    std::unordered_map<std::string, std::vector<std::shared_ptr<ShaderBlock>>> passes;
    std::unordered_map<std::string, bool>                                      options;
    Llp::Lexer                                                                 lexer;
    std::optional<Llp::ParserError>                                            error;
    Eng::Gfx::PipelineOptions                                                  pipeline_options;
    const std::string                                                          source_code;

};
}