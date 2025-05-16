
#include "refl_enum.h"

#include <llp/native_tokens.hpp>
#include <llp/parser.hpp>

Llp::ParserError Enum::try_parse(Llp::Parser& parser, const ParserContext& context, const Llp::TokenSet& token_set)
{
    auto args = parser.consume<Llp::ParenthesisBlockToken>();
    if (!args)
        return Llp::ParserError{parser.current_location(), "'(args...)' expected after RENUM"};
    if (auto error = try_parse_args(args->content))
        return error;

    if (!parser.consume<Llp::WordToken>("enum"))
        return Llp::ParserError{parser.current_location(), "'enum' word expected"};

    scoped = parser.consume<Llp::WordToken>("class");

    if (auto error = enum_name.try_parse(parser, context))
        return error;

#if _WIN32
    type = "int";
#else
    type = "";
#endif
    if (parser.consume<Llp::SymbolToken>(':'))
    {
        if (auto t = parser.consume<Llp::WordToken>())
            type   = t->word;
        else
            return Llp::ParserError{parser.current_location(), "Expected enum type after ':'"};
    }

    auto enum_content = parser.consume<Llp::BraceBlockToken>();
    if (!enum_content)
        return Llp::ParserError{parser.current_location(), "Expected enum content {}"};
    if (auto error = try_parse_body(enum_content->content, token_set))
        return error;

    return {};
}

Llp::ParserError Enum::try_parse_args(Llp::Tokenizer& block)
{
    Llp::Parser parser(block);
    while (parser && parser.get_current_token_type() != Llp::NULL_TOKEN)
    {
        if (auto key = parser.consume<Llp::WordToken>())
        {
            if (key->word == "EnumFlags")
                b_flag_enum_flags = true;
            else
                return Llp::ParserError{parser.current_location(), std::format("Unknown word token '{}'", key->word)};
        }
        else
            return Llp::ParserError{parser.current_location(), std::format("Expected word")};
        if (parser && !parser.consume<Llp::SymbolToken>(','))
            return Llp::ParserError{parser.current_location(), "Expected coma token"};
    }
    return {};
}

Llp::ParserError Enum::try_parse_body(Llp::Tokenizer& tokenizer, const Llp::TokenSet& token_set)
{
    Llp::Parser parser(tokenizer);
    while (parser)
    {
        if (auto key = parser.consume<Llp::WordToken>())
        {
            fields.emplace_back(key->word);
            if (parser.consume<Llp::SymbolToken>('='))
            {
                if (!parser.consume<Llp::IntegerToken>())
                    return Llp::ParserError{parser.current_location(), "Expected number here"};
                if (parser.consume<Llp::SymbolToken>('<'))
                {
                    if (!parser.consume<Llp::SymbolToken>('<'))
                        return Llp::ParserError{parser.current_location(), "Expected symbol '<'"};

                    if (!parser.consume<Llp::IntegerToken>())
                        return Llp::ParserError{parser.current_location(), "Expected number here"};
                }
            }
        }
        else
            return Llp::ParserError{
                parser.current_location(),
                std::format("Expected enum field name, got {}", parser.get_current_token_name(token_set))
            };
        if (parser && !parser.consume<Llp::SymbolToken>(','))
            return Llp::ParserError{parser.current_location(), "Expected coma token"};
    }
    return {};
}