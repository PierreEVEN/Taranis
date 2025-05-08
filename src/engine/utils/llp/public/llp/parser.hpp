#pragma once

#include "tokens.hpp"

#include <vector>
#include <ankerl/unordered_dense.h>

namespace Llp
{
enum class ELexerToken;
}

namespace Llp
{
class TokenizedBlock;

class Parser
{
public:
    Parser(const TokenizedBlock& in_block);

    Parser& operator++()
    {
        idx++;
        return *this;
    }

    operator bool() const
    {
        return idx < block->get_tokens().size();
    }

    template <typename T> T* get(size_t offset = 0)
    {
        const auto& tokens = block->get_tokens();

        size_t new_token = get_with_offset(offset);
        if (new_token < tokens.size())
        {
            auto& token = tokens[new_token];
            if (token->get_type() == TTokenType<T>::id)
            {
                return static_cast<T*>(token.get());
            }
        }
        return nullptr;
    }

    template <typename T, typename U> T* get(const U& value, size_t offset)
    {
        if (auto* found = get<T>(offset))
            if (*found == value)
                return found;
        return nullptr;
    }

    template <typename T> T* consume()
    {
        if (T* token = get<T>(0))
        {
            idx = get_with_offset(0) + 1;
            return token;
        }
        return nullptr;
    }

    template <typename T, typename U> T* consume(const U& value)
    {
        if (T* token = get<T, U>(value, 0))
        {
            idx = get_with_offset(0) + 1;
            return token;
        }
        return nullptr;
    }

    const ILexerToken& operator*() const
    {
        return *block->get_tokens()[get_with_offset(0)];
    }

    const Location& current_location() const
    {
        if (block->get_tokens().empty())
        {
            static Location empty_location;
            return empty_location;
        }
        return block->get_tokens()[idx]->location;
    }

    LexerTokenTypeId get_current_token_type() const
    {
        size_t offset = get_with_offset(0);
        if (offset < block->get_tokens().size())
            return block->get_tokens()[offset]->get_type();
        return NULL_TOKEN;
    }

private:
    size_t get_with_offset(size_t offset) const
    {
        const auto& tokens     = block->get_tokens();
        size_t      out_offset = idx;
        for (size_t j = 0; out_offset < tokens.size() && (j < offset || is_skipped_token(out_offset)); ++out_offset)
        {
            if (!is_skipped_token(out_offset))
                ++j;
        }
        return out_offset;
    }

    bool is_skipped_token(size_t t_idx) const
    {
        const auto& tokens = block->get_tokens();
        auto        type   = tokens[t_idx]->get_type();
        for (const auto& t : tokens_to_skip)
            if (type == t)
                return true;
        return false;
    }

    size_t               idx = 0;
    const TokenizedBlock* block;

    std::vector<LexerTokenTypeId> tokens_to_skip;
};
} // namespace Llp