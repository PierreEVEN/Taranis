#pragma once
#include <memory>
#include <string>
#include <vector>

namespace Llp
{
struct ILexerToken;

class Lexer
{
public:
    Lexer(const std::string& source);

    const std::vector<std::unique_ptr<ILexerToken>>& get_tokens() const
    {
        return tokens;
    }

private:
    std::vector<std::unique_ptr<ILexerToken>> tokens;
};
}