#include "llp/tokens.hpp"

#include "llp/lexer.hpp"

#include <format>
#include <iostream>

namespace Llp
{
void TokenizedBlock::consume_next(Lexer& lexer, const std::string& source, Location& location, std::optional<ParserError>& error)
{
    if (!lexer_context)
        lexer_context = lexer.context;
    for (const auto& token_type_fn : lexer.context->token_types)
    {
        if (error)
            return;
        if (auto token = token_type_fn->consume(lexer, location, source, error))
        {
            if (error)
                return;
            tokens.emplace_back(std::move(token));
            return;
        }
    }
    if (!error)
        error = {location, std::format("Parsing failed {}:{} : unidentified token '{}'", location.line, location.column, source[location.index])};
}

std::string TokenizedBlock::to_string(bool b_debug) const
{
    std::string str;
    for (const auto& token : tokens)
        if (b_debug)
        {
            if (!lexer_context)
            {
                std::cerr << "Lexer context have not been set\n";
                return "Lexer_context_have_not_been_set";
            }
            str += std::format("{}[{}], ", lexer_context->get_token_name(token->get_type()), token->to_string(b_debug));
        }
        else
            str += token->to_string(b_debug);
    return str;
}
} // namespace Llp