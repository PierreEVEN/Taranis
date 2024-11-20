#pragma once

#include "tokens.hpp"

#include <vector>

namespace Llp
{
enum class ELexerToken;
}

namespace Llp
{
class Block;

class Parser
{
public:
    Parser(const Block& in_block, const std::vector<ELexerToken>& skip_tokens = {}) : block(&in_block), skipped_tokens(skip_tokens)
    {

    }

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
        const auto& tokens    = block->get_tokens();

        size_t new_token = get_with_offset(offset);
        if (new_token < tokens.size())
        {
            auto& token = tokens[new_token];
            if (token->get_type() == T::static_type())
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

    const Location& current_location()
    {
        return block->get_tokens()[idx]->location;
    }

private:
    size_t get_with_offset(size_t offset) const
    {
        const auto& tokens = block->get_tokens();
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
        for (const auto& t : skipped_tokens)
            if (type == t)
                return true;
        return false;
    }

    size_t                   idx = 0;
    const Block*             block;
    std::vector<ELexerToken> skipped_tokens;
};
}