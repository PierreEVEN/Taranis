#pragma once
#include "tokens.hpp"

#include <string>

namespace Llp
{
class Lexer
{
public:
    Lexer(const std::string& source);

    const Block& get_root() const
    {
        return block;
    }

    const ParserError* get_error() const
    {
        return error ? &*error : nullptr;
    }

private:
    std::optional<ParserError> error;
    Block                      block;
};
}