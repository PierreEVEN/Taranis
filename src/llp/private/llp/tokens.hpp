#pragma once
#include <string>

namespace Llp
{
enum class ELexerToken
{
    Semicolon,
    Endl,
    Include,
    Word,
};

struct ILexerToken
{
  public:
    ILexerToken(size_t in_line, size_t in_column, size_t in_index) : line(in_line), column(in_column), index(in_index)
    {
    }

    size_t line   = 0;
    size_t column = 0;
    size_t index  = 0;

    virtual ELexerToken get_type() const = 0;
};

struct SemicolonToken : public ILexerToken
{
    using ILexerToken::ILexerToken;
    ELexerToken get_type() const override
    {
        return ELexerToken::Semicolon;
    }
};

struct EndlToken : public ILexerToken
{
    using ILexerToken::ILexerToken;
    ELexerToken get_type() const override
    {
        return ELexerToken::Endl;
    }
};

struct IncludeToken : public ILexerToken
{
    using ILexerToken::ILexerToken;
    ELexerToken get_type() const override
    {
        return ELexerToken::Include;
    }
    std::string include;
};

struct WordToken : public ILexerToken
{
    using ILexerToken::ILexerToken;
    ELexerToken get_type() const override
    {
        return ELexerToken::Word;
    }
    std::string word;
};
} // namespace Llp