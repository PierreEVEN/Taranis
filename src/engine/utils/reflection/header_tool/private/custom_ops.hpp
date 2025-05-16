#pragma once
#include <llp/token.hpp>

/******* CUSTOM TOKENS *******/
namespace Llp
{
/*####[ :: ]####*/
DECLARE_LEXER_TOKEN(ScopeOperator)
static std::unique_ptr<ScopeOperator> consume(const TokenSet&, Location& in_location, const std::string& source, ParserError&)
{
    if (source[in_location.get_index()] == ':' && source[in_location.get_index() + 1] == ':')
    {
        ++ ++in_location;
        return std::make_unique<ScopeOperator>(in_location);
    }
    return nullptr;
}

[[nodiscard]] std::string to_string(const TokenSet&, bool) const override
{
    return "::";
}
};

/*####[ :: ]####*/
DECLARE_LEXER_TOKEN(EllipsisOperator)
static std::unique_ptr<EllipsisOperator> consume(const TokenSet&, Location& in_location, const std::string& source, ParserError&)
{
    if (source[in_location.get_index()] == '.' && source[in_location.get_index() + 1] == '.' && source[in_location.get_index() + 2] == '.')
    {
        ++ ++ ++in_location;
        return std::make_unique<EllipsisOperator>(in_location);
    }
    return nullptr;
}

[[nodiscard]] std::string to_string(const TokenSet&, bool) const override
{
    return "...";
}
};
}
