#include "llp/lexer.hpp"

#include "llp/native_tokens.hpp"
#include "llp/tokens.hpp"
#include <iostream>
namespace Llp
{

Lexer::Lexer()
{
    context = std::make_shared<LexerContext>();
    context->token_types.emplace_back(std::make_unique<TTokenBuilder<CommentToken>>());
    context->token_types.emplace_back(std::make_unique<TTokenBuilder<SemicolonToken>>());
    context->token_types.emplace_back(std::make_unique<TTokenBuilder<EndlToken>>());
    context->token_types.emplace_back(std::make_unique<TTokenBuilder<EqualsToken>>());
    context->token_types.emplace_back(std::make_unique<TTokenBuilder<FloatingPointToken>>());
    context->token_types.emplace_back(std::make_unique<TTokenBuilder<IntegerToken>>());
    context->token_types.emplace_back(std::make_unique<TTokenBuilder<ComaToken>>());
    context->token_types.emplace_back(std::make_unique<TTokenBuilder<IncludeToken>>());
    context->token_types.emplace_back(std::make_unique<TTokenBuilder<StringLiteralToken>>());
    context->token_types.emplace_back(std::make_unique<TTokenBuilder<WhitespaceToken>>());
    context->token_types.emplace_back(std::make_unique<TTokenBuilder<WordToken>>());
    context->token_types.emplace_back(std::make_unique<TTokenBuilder<ArgumentsToken>>());
    context->token_types.emplace_back(std::make_unique<TTokenBuilder<BlockToken>>());
    context->token_types.emplace_back(std::make_unique<TTokenBuilder<SymbolToken>>());
    context->token_names.emplace(TTokenType<CommentToken>::id, "Comment");
    context->token_names.emplace(TTokenType<SemicolonToken>::id, "Semicolon");
    context->token_names.emplace(TTokenType<EndlToken>::id, "Endl");
    context->token_names.emplace(TTokenType<EqualsToken>::id, "Equals");
    context->token_names.emplace(TTokenType<FloatingPointToken>::id, "FloatingPoint");
    context->token_names.emplace(TTokenType<IntegerToken>::id, "Integer");
    context->token_names.emplace(TTokenType<ComaToken>::id, "Coma");
    context->token_names.emplace(TTokenType<IncludeToken>::id, "Include");
    context->token_names.emplace(TTokenType<StringLiteralToken>::id, "StringLiteral");
    context->token_names.emplace(TTokenType<WhitespaceToken>::id, "Whitespace");
    context->token_names.emplace(TTokenType<WordToken>::id, "Word");
    context->token_names.emplace(TTokenType<ArgumentsToken>::id, "Arguments");
    context->token_names.emplace(TTokenType<BlockToken>::id, "Block");
    context->token_names.emplace(TTokenType<SymbolToken>::id, "Symbol");
}

void Lexer::run(const std::string& source)
{
    Location location;
    while (location.index < source.size())
    {
        block.consume_next(*this, source, location, error);
        if (error)
            return;
    }
}
} // namespace Llp