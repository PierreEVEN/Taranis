#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>

class IToken;

namespace Llp
{
class Lexer;
struct LexerContext;
class ILexerToken;

using LexerTokenTypeId = void (*)();

inline size_t _token_type_id_gen_function_dummy_value = 0;
template <typename T> void _token_type_id_gen_function()
{
    // Required otherwise the compiler could factorize this function
    _token_type_id_gen_function_dummy_value += typeid(T).hash_code();
}

template <typename T> constexpr LexerTokenTypeId _token_type_id_generator = &_token_type_id_gen_function<T>;
static constexpr LexerTokenTypeId                NULL_TOKEN               = nullptr;

template <typename T> struct TTokenType
{
    static constexpr const char*      name = "Null";
    static constexpr LexerTokenTypeId id   = NULL_TOKEN;
};

#define DECLARE_LEXER_TOKEN(Token)                                                              \
    template <> struct TTokenType<class Token>                                                  \
    {                                                                                           \
        static constexpr const char*      name        = #Token;                                 \
        static constexpr LexerTokenTypeId id   = _token_type_id_generator<Token>;               \
    };                                                                                          \
    class Token : public ILexerToken                                                            \
    {                                                                                           \
    public:                                                                                     \
        LexerTokenTypeId get_type() const override                                              \
        {                                                                                       \
            return TTokenType<Token>::id;                                                       \
        }                                                                                       \
    using ILexerToken::ILexerToken;

struct Location
{
    size_t line   = 0;
    size_t column = 0;
    size_t index  = 0;

    Location& operator++()
    {
        index++;
        column++;
        return *this;
    }

    Location operator++(int)
    {
        Location old = *this;
        ++*this;
        return old;
    }

    Location on_new_line()
    {
        Location old = *this;
        line++;
        column = 0;
        index++;
        return old;
    }
};

struct ParserError
{
    ParserError(Location in_location, std::string in_message) : location(std::move(in_location)), message(std::move(in_message))
    {
    }

    Location    location;
    std::string message;
};


class ITokenBuilder
{
public:
    virtual std::unique_ptr<ILexerToken> consume(Lexer& lexer, Location& in_location, const std::string& source, std::optional<ParserError>& error) = 0;
    virtual LexerTokenTypeId             id() const = 0;
};

template <typename T> class TTokenBuilder : public ITokenBuilder
{
    std::unique_ptr<ILexerToken> consume(Lexer& lexer, Location& in_location, const std::string& source, std::optional<ParserError>& error) override
    {
        return T::consume(lexer, in_location, source, error);
    }

public:
    LexerTokenTypeId id() const override
    {
        return TTokenType<T>::id;
    }
};

class ILexerToken
{
public:
    ILexerToken(const Location& in_location) : location(in_location)
    {
    }

    virtual  ~ILexerToken() = default;
    Location location;

    virtual LexerTokenTypeId get_type() const = 0;
    virtual std::string      to_string(bool b_debug) const = 0;

protected:
    static bool consume_string(Location& loc, const std::string& str, const std::string& test)
    {
        size_t i = 0;
        for (; i < test.size() && i + loc.index < str.size(); ++i)
        {
            if (test[i] != str[loc.index + i])
                return false;
        }
        if (i == test.size())
        {
            loc.index += i;
            return true;
        }
        return false;
    }

    static bool consume_whitespace(Location& loc, const std::string& str)
    {
        while (loc.index < str.size())
        {
            if (str[loc.index] == '\n')
                loc.on_new_line();
            else if (std::isspace(str[loc.index]))
                ++loc.index;
            else
                break;
        }
        return loc.index != str.size();
    }
};

class TokenizedBlock
{
public:
    void consume_next(Lexer& lexer, const std::string& source, Location& location, std::optional<ParserError>& error);

    const std::vector<std::unique_ptr<ILexerToken>>& get_tokens() const
    {
        return tokens;
    }

    std::string to_string(bool) const;

    const LexerContext& get_lexer_context() const
    {
        return *lexer_context;
    }

private:
    std::shared_ptr<LexerContext> lexer_context;

    std::vector<std::unique_ptr<ILexerToken>> tokens;
};
} // namespace Llp