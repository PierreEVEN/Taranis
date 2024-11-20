#pragma once
#include "llp/lexer.hpp"

#include <string>
#include <unordered_map>

namespace ShaderCompiler
{


class ShaderParser
{
public:
    ShaderParser(const std::string& source_shader);

private:
    struct ShaderBlock
    {
        std::string   raw_code;
        Llp::Location start;
        Llp::Location end;
    };

    std::optional<Llp::ParserError> parse();

    std::unordered_map<std::string, std::vector<std::shared_ptr<ShaderBlock>>> passes;
    std::unordered_map<std::string, bool> options;
    Llp::Lexer                      lexer;
    std::optional<Llp::ParserError> error;

    std::optional<Llp::ParserError> parse_pass_args(Llp::ArgumentsToken& args, std::vector<std::string>& pass_list);

};
}