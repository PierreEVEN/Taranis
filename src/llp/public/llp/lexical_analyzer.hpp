#pragma once

#include "file_data.hpp"

#include <string>
#include <vector>

class IToken;

enum class TokenType
{
    Symbol,
    Word,
    Include,
    TokenizerBlock,
    Arguments,
    Number
};

using Symbol  = char;
using Number  = std::string;
using Word    = std::string;
using Include = std::string;

struct TokenizerBlock
{
    struct Reader
    {

        size_t       location = 0;
        const TokenizerBlock* parent;

        void operator++()
        {
            location++;
        }

        IToken& operator*() const
        {
            return *parent->tokens[location];
        }

        IToken* operator->() const
        {
            return parent->tokens[location].get();
        }

        operator bool() const
        {
            return location < parent->tokens.size();
        }

        template <typename T> T* consume(TokenType type);

        template <typename T> bool consume(TokenType type, T tested_value);
    };

    Reader read() const
    {
        return Reader{.parent = this};
    }

    TokenizerBlock() = default;
    TokenizerBlock(TextReader& reader);
    void print(size_t i = 0) const;

    std::vector<std::unique_ptr<IToken>> tokens;
};

using Arguments = TokenizerBlock;

struct ReflectedClassBody
{
    size_t                   line;
    size_t                   column;
    std::vector<std::string> flags;
};

class IToken
{
  public:
    virtual ~IToken() = default;
    TokenType type;
    size_t    line;
    size_t    column;
    size_t    offset;

    template <typename T> T& data();

  protected:
    IToken(TokenType in_type, size_t in_line, size_t in_column, size_t in_offset) : type(in_type), line(in_line), column(in_column), offset(in_offset)
    {
    }
};

template <typename T> class TToken : public IToken
{
  public:
    TToken(TokenType in_type, T in_data, size_t in_line, size_t in_column, size_t in_offset) : IToken(in_type, in_line, in_column, in_offset)
    {
        data = std::move(in_data);
    }

    T data;
};

template <typename T> T* TokenizerBlock::Reader::consume(TokenType type)
{
    if (*this && parent->tokens[location]->type == type)
    {
        auto* data = &parent->tokens[location]->data<T>();
        ++*this;
        return data;
    }
    return nullptr;
}

template <typename T> bool TokenizerBlock::Reader::consume(TokenType type, T tested_value)
{
    if (*this && parent->tokens[location]->type == type)
    {
        if (parent->tokens[location]->data<T>() == tested_value)
        {
            ++*this;
            return true;
        }
    }
    return false;
}

template <typename T> T& IToken::data()
{
    return dynamic_cast<TToken<T>*>(this)->data;
}