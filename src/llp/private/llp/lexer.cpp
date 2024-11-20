#include "llp/lexer.hpp"

#include "llp/tokens.hpp"

namespace Llp
{

Lexer::Lexer(const std::string& source)
{
    Location location;
    while (location.index < source.size())
    {
        block.consume_next(source, location, error);
        if (error)
            return;
    }
}
}