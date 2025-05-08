#pragma once
#include "tokens.hpp"

#include <string>
#include <ankerl/unordered_dense.h>

namespace Llp
{
struct LexerContext
{
    const char* get_token_name(LexerTokenTypeId id) const
    {
        if (id == NULL_TOKEN)
            return "Null";
        if (auto it = token_names.find(id); it != token_names.end())
            return it->second;
        return "Undefined_Token_Id";
    }

    std::vector<std::unique_ptr<ITokenBuilder>>                 token_types;
    ankerl::unordered_dense::map<LexerTokenTypeId, const char*> token_names;
};

class Lexer
{
public:
    friend class TokenizedBlock;
    Lexer();
    void run(const std::string& source);

    const TokenizedBlock& get_root() const
    {
        return block;
    }

    const std::optional<ParserError>& get_error() const
    {
        return error;
    }

    template <typename T> void register_token(const char* name)
    {
        context->token_types.emplace_back(std::make_unique<TTokenBuilder<T>>());
        context->token_names.emplace(TTokenType<T>::id, name);
    }

    template <typename T, typename Other> void register_token_before(const char* name)
    {
        for (auto it = context->token_types.begin(); it != context->token_types.end(); ++it)
            if ((*it)->id() == TTokenType<Other>::id)
            {
                context->token_types.insert(it, std::make_unique<TTokenBuilder<T>>());
                context->token_names.emplace(TTokenType<T>::id, name);
                break;
            }
    }

    template <typename T> void remove_token()
    {
        for (auto it = context->token_types.begin(); it != context->token_types.end(); ++it)
            if ((*it)->id() == TTokenType<T>::id)
            {
                context->token_names.erase(TTokenType<T>::id);
                context->token_types.erase(it);
                break;
            }
    }

    const LexerContext& get_context() const
    {
        return *context;
    }

private:
    std::shared_ptr<LexerContext> context;

    std::optional<ParserError> error;
    TokenizedBlock             block;
};
} // namespace Llp