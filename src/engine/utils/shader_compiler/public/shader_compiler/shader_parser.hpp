#pragma once
#include "gfx_types/pipeline.hpp"
#include "llp/native_tokens.hpp"

#include <string>
#include <ankerl/unordered_dense.h>

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

    Llp::ParserError get_error() const
    {
        return error;
    }

    const ankerl::unordered_dense::map<std::string, std::vector<std::shared_ptr<ShaderBlock>>>& get_passes() const
    {
        return passes;
    }

    const Eng::Gfx::PipelineOptions& get_pipeline_config() const
    {
        return pipeline_options;
    }

    const ankerl::unordered_dense::map<std::string, bool>& get_default_options() const
    {
        return options;
    }

private:
    Llp::ParserError        parse(const Llp::TokenSet& token_set);
    static Llp::ParserError parse_pass_args(Llp::ParenthesisBlockToken& args, std::vector<std::string>& pass_list);
    std::optional<std::string>             parse_config_value(const std::string& key, const std::string& value);

    static Llp::ParserError parse_block(const Llp::BraceBlockToken& args, ShaderBlock& block);

    ankerl::unordered_dense::map<std::string, std::vector<std::shared_ptr<ShaderBlock>>> passes;
    ankerl::unordered_dense::map<std::string, bool>                                      options;
    Llp::Tokenizer                                                                       lexer;
    Llp::ParserError                                                      error;
    Eng::Gfx::PipelineOptions                                                            pipeline_options;
    const std::string                                                                    source_code;
};
} // namespace ShaderCompiler