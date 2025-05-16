#include "header_parser.hpp"

#include "cpp_objects/refl_enum.h"
#include "custom_ops.hpp"
#include "llp/native_tokens.hpp"
#include "llp/parser.hpp"

#include <filesystem>
#include <format>

HeaderParser::HeaderParser(const std::string& header_data, std::filesystem::path in_generated_header_include_path, std::filesystem::path in_header_path)
    : generated_header_include_path(std::move(in_generated_header_include_path)), header_path(std::move(in_header_path))
{
    Llp::TokenSet token_set = Llp::TokenSet::preset_c_like();
    token_set.register_token_before<Llp::ScopeOperator, Llp::SymbolToken>("ScopeOp");
    token_set.register_token_before<Llp::EllipsisOperator, Llp::SymbolToken>("Ellipsis");

    Llp::Tokenizer lexer;
    lexer.tokenize(header_data, token_set).exit_on_error(header_path);
    parse_block(lexer, token_set, {}).exit_on_error(header_path);
}

Llp::ParserError HeaderParser::parse_block(const Llp::Tokenizer& block, const Llp::TokenSet& token_set, const ParserContext& context)
{
    Llp::Parser parser(block);
    while (parser)
    {
        // Preprocessor
        if (parser.consume<Llp::SymbolToken>('#') && parser.consume<Llp::WordToken>("pragma") && parser.consume<Llp::WordToken>("once"))
        {
            line_after_last_include = parser.current_location().get_line() + 1;
        }
        else if (auto word = parser.consume<Llp::WordToken>())
        {
            // Class declaration
            if (word->word == "class")
            {
                Class class_info;
                if (auto error = class_info.try_parse(parser, context, token_set))
                    return error;

                if (last_template_declaration)
                    class_info.set_template_arguments(*last_template_declaration);
                last_template_declaration = {};

                if (!class_info.is_forward_declaration())
                {
                    if (auto* class_block = parser.consume<Llp::BraceBlockToken>())
                        if (auto error = parse_block(class_block->content, token_set, context.push_class(class_info)))
                            return error;
                }
            }
            else if (word->word == "template")
            {
                if (parser.get<Llp::SymbolToken>('<', 0))
                {
                    last_template_declaration = TemplateDeclaration{};
                    if (auto error = last_template_declaration->try_parse(parser, context))
                        return error;
                    continue;
                }
            }
            else if (word->word == "using" && parser.consume<Llp::WordToken>("namespace"))
            {
                return Llp::ParserError{parser.current_location(), "'using namespace ...;' is forbidden in headers !"};
            }
            else if (word->word == "namespace")
            {
                ParserContext new_context = context;
                do
                {
                    auto namespace_name = parser.consume<Llp::WordToken>();
                    if (!namespace_name)
                        return Llp::ParserError{parser.current_location(), "Expected word here. (Global namespace are not supported yet)"};
                    new_context = new_context.push_namespace(namespace_name->word);
                } while (parser.consume<Llp::ScopeOperator>());
                auto* namespace_block = parser.consume<Llp::BraceBlockToken>();
                if (!namespace_block)
                    return Llp::ParserError{parser.current_location(), "Expected brace block."};
                if (auto error = parse_block(namespace_block->content, token_set, new_context))
                    return error;
            }
            if (word->word == "RPROPERTY")
            {
                if (context.class_stack.empty())
                    return Llp::ParserError{parser.current_location(), "RPROPERTY() should not be used outside class context"};
                if (auto error = context.class_stack.back()->parse_property(parser, context))
                    return error;
            }
            if (word->word == "REFLECT_BODY")
            {
                if (context.class_stack.empty())
                    return Llp::ParserError{parser.current_location(), "REFLECT_BODY() should not be declared outside class context"};

                auto refl_class = context.class_stack.back();
                refl_class->make_reflected(parser);
                reflected_classes.emplace(refl_class->name().cpp_name(), refl_class);
            }
            if (word->word == "RENUM")
            {
                Enum enum_data;
                if (auto error = enum_data.try_parse(parser, context, token_set))
                    return error;

                reflected_enums.emplace(enum_data.name().cpp_name(), enum_data);
            }
        }
        else if (auto include = parser.consume<Llp::IncludeToken>())
        {
            if (b_found_include)
                return Llp::ParserError{parser.current_location(), std::format("Generated header \"{}\" should be the last included", generated_header_include_path.generic_string())};
            line_after_last_include = parser.current_location().get_line() + 1;
            if (std::filesystem::path(include->path).lexically_normal() == generated_header_include_path)
                b_found_include = true;
        }
        else if (auto raw_block = parser.consume<Llp::BraceBlockToken>())
        {
            if (auto error = parse_block(raw_block->content, token_set, context))
                return error;
        }
        else
            ++parser;

        last_template_declaration = {};
    }
    return {};
}
