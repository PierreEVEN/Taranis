#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>

class IToken;

namespace Llp
{
enum class ELexerToken
{
    Symbol,
    Semicolon,
    Coma,
    Comment,
    Endl,
    Equals,
    Include,
    Whitespace,
    Word,
    Block,
    StringLiteral,
    Arguments,
    Integer,
    FloatingPoint
};

inline const char* token_type_to_string(ELexerToken type)
{
    switch (type)
    {
    case ELexerToken::Symbol:
        return "Symbol";
    case ELexerToken::Semicolon:
        return "Semicolon";
    case ELexerToken::Coma:
        return "Coma";
    case ELexerToken::Comment:
        return "Comment";
    case ELexerToken::Endl:
        return "Endl";
    case ELexerToken::Include:
        return "Include";
    case ELexerToken::Whitespace:
        return "Space";
    case ELexerToken::Word:
        return "Word";
    case ELexerToken::Block:
        return "Block";
    case ELexerToken::StringLiteral:
        return "String";
    case ELexerToken::Arguments:
        return "Arguments";
    case ELexerToken::Integer:
        return "Integer";
    case ELexerToken::FloatingPoint:
        return "FloatingPoint";
    case ELexerToken::Equals:
        return "Equals";
    default:
        return "Unknown";
    }

}

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

struct ILexerToken
{
public:
    ILexerToken(const Location& in_location) : location(in_location)
    {
    }

    virtual  ~ILexerToken() = default;
    Location location;

    virtual ELexerToken get_type() const = 0;
    virtual std::string to_string(bool b_debug) const = 0;

protected:
    static bool consume_string(Location& loc, const std::string& str, const std::string& test)
    {
        size_t i = 0;
        for (; i < test.size() && i + loc.index < str.size(); ++i)
        {
            if (test[i] != str[loc.index + i])
                return false;
        }
        if (i == test.size() - 1)
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

class Block
{
public:
    void consume_next(const std::string& source, Location& location, std::optional<ParserError>& error);

    const std::vector<std::unique_ptr<ILexerToken>>& get_tokens() const
    {
        return tokens;
    }

    std::string to_string(bool) const;

private:
    std::vector<std::unique_ptr<ILexerToken>> tokens;
};

/*####[ // /* ]####*/
struct CommentToken : ILexerToken
{
    using ILexerToken::ILexerToken;

    static std::unique_ptr<CommentToken> consume(Location& in_location, const std::string& source, std::optional<ParserError>& error);

    ELexerToken get_type() const override
    {
        return ELexerToken::Comment;
    }

    static ELexerToken static_type()
    {
        return ELexerToken::Comment;
    }

    std::string to_string(bool) const override
    {
        return multiline ? "/*" + value + "*/" : "//" + value;
    }

    bool        multiline = false;
    std::string value;
};

/*####[ ; ]####*/
struct SemicolonToken : ILexerToken
{
    using ILexerToken::ILexerToken;

    static std::unique_ptr<SemicolonToken> consume(Location& in_location, const std::string& source, std::optional<ParserError>& error);

    ELexerToken get_type() const override
    {
        return ELexerToken::Semicolon;
    }

    static ELexerToken static_type()
    {
        return ELexerToken::Semicolon;
    }

    std::string to_string(bool) const override
    {
        return ";";
    }
};

/*####[ = ]####*/
struct EqualsToken : ILexerToken
{
    using ILexerToken::ILexerToken;

    static std::unique_ptr<EqualsToken> consume(Location& in_location, const std::string& source, std::optional<ParserError>& error);

    ELexerToken get_type() const override
    {
        return ELexerToken::Equals;
    }

    static ELexerToken static_type()
    {
        return ELexerToken::Equals;
    }

    std::string to_string(bool) const override
    {
        return "=";
    }
};

/*####[ , ]####*/
struct ComaToken : ILexerToken
{
    using ILexerToken::ILexerToken;

    static std::unique_ptr<ComaToken> consume(Location& in_location, const std::string& source, std::optional<ParserError>& error);

    ELexerToken get_type() const override
    {
        return ELexerToken::Coma;
    }

    static ELexerToken static_type()
    {
        return ELexerToken::Coma;
    }

    std::string to_string(bool) const override
    {
        return ",";
    }
};

/*####[ \n ]####*/
struct EndlToken : ILexerToken
{
    using ILexerToken::ILexerToken;

    static std::unique_ptr<EndlToken> consume(Location& in_location, const std::string& source, std::optional<ParserError>& error);

    ELexerToken get_type() const override
    {
        return ELexerToken::Endl;
    }

    static ELexerToken static_type()
    {
        return ELexerToken::Endl;
    }

    std::string to_string(bool) const override
    {
        return "\n";
    }
};

/*####[ #include "value" ]####*/
struct IncludeToken : ILexerToken
{
    using ILexerToken::ILexerToken;

    static std::unique_ptr<IncludeToken> consume(Location& in_location, const std::string& source, std::optional<ParserError>& error);

    ELexerToken get_type() const override
    {
        return ELexerToken::Include;
    }

    static ELexerToken static_type()
    {
        return ELexerToken::Include;
    }

    std::string to_string(bool) const override
    {
        return "#include \"" + path + "\"";
    }

    bool operator==(const std::string& a) const
    {
        return path == a;
    }

    std::string path;
};

/*####[ Whitespace ]####*/
struct WhitespaceToken : ILexerToken
{
    using ILexerToken::ILexerToken;

    static std::unique_ptr<WhitespaceToken> consume(Location& in_location, const std::string& source, std::optional<ParserError>& error);

    ELexerToken get_type() const override
    {
        return ELexerToken::Whitespace;
    }

    static ELexerToken static_type()
    {
        return ELexerToken::Whitespace;
    }

    std::string to_string(bool) const override
    {
        return whitespace;
    }

    std::string whitespace;
};

/*####[ Word ]####*/
struct WordToken : ILexerToken
{
    using ILexerToken::ILexerToken;

    static std::unique_ptr<WordToken> consume(Location& in_location, const std::string& source, std::optional<ParserError>& error);

    ELexerToken get_type() const override
    {
        return ELexerToken::Word;
    }

    static ELexerToken static_type()
    {
        return ELexerToken::Word;
    }

    std::string to_string(bool) const override
    {
        return word;
    }

    bool operator==(const std::string& a) const
    {
        return word == a;
    }

    std::string word;
};

/*####[ {} ]####*/
struct BlockToken : ILexerToken
{
    using ILexerToken::ILexerToken;

    static std::unique_ptr<BlockToken> consume(Location& in_location, const std::string& source, std::optional<ParserError>& error);

    ELexerToken get_type() const override
    {
        return ELexerToken::Block;
    }

    static ELexerToken static_type()
    {
        return ELexerToken::Block;
    }

    std::string to_string(bool b_debug) const override
    {
        return "(" + content.to_string(b_debug) + ")";
    }

    Location end;
    Block    content;
};

/*####[ "string" ]####*/
struct StringLiteralToken : ILexerToken
{
    using ILexerToken::ILexerToken;

    static std::unique_ptr<StringLiteralToken> consume(Location& in_location, const std::string& source, std::optional<ParserError>& error);

    ELexerToken get_type() const override
    {
        return ELexerToken::StringLiteral;
    }

    static ELexerToken static_type()
    {
        return ELexerToken::StringLiteral;
    }

    std::string to_string(bool) const override
    {
        return '"' + value + '"';
    }

    bool operator==(const std::string& a) const
    {
        return value == a;
    }

    std::string value;
};

/*####[ () ]####*/
struct ArgumentsToken : ILexerToken
{
    using ILexerToken::ILexerToken;

    static std::unique_ptr<ArgumentsToken> consume(Location& in_location, const std::string& source, std::optional<ParserError>& error);

    ELexerToken get_type() const override
    {
        return ELexerToken::Arguments;
    }

    static ELexerToken static_type()
    {
        return ELexerToken::Arguments;
    }

    std::string to_string(bool b_debug) const override
    {
        return "(" + content.to_string(b_debug) + ")";
    }

    Location end;
    Block    content;
};

/*####[ 000 ]####*/
struct IntegerToken : ILexerToken
{
    using ILexerToken::ILexerToken;

    static std::unique_ptr<IntegerToken> consume(Location& in_location, const std::string& source, std::optional<ParserError>& error);

    ELexerToken get_type() const override
    {
        return ELexerToken::Integer;
    }

    static ELexerToken static_type()
    {
        return ELexerToken::Integer;
    }

    std::string to_string(bool) const override
    {
        return std::to_string(number);
    }

    bool operator==(int64_t a) const
    {
        return number == a;
    }

    int64_t number;
};

/*####[ 00.00 ]####*/
struct FloatingPointToken : ILexerToken
{
    using ILexerToken::ILexerToken;

    static std::unique_ptr<FloatingPointToken> consume(Location& in_location, const std::string& source, std::optional<ParserError>& error);

    ELexerToken get_type() const override
    {
        return ELexerToken::FloatingPoint;
    }

    static ELexerToken static_type()
    {
        return ELexerToken::FloatingPoint;
    }

    std::string to_string(bool) const override
    {
        return std::to_string(number);
    }

    double number;
};

/*####[ ! ]####*/
struct SymbolToken : ILexerToken
{
    using ILexerToken::ILexerToken;

    static std::unique_ptr<SymbolToken> consume(Location& in_location, const std::string& source, std::optional<ParserError>& error);

    ELexerToken get_type() const override
    {
        return ELexerToken::Symbol;
    }

    static ELexerToken static_type()
    {
        return ELexerToken::Symbol;
    }

    std::string to_string(bool) const override
    {
        return std::string() + symbol;
    }

    bool operator==(char a) const
    {
        return symbol == a;
    }

    char symbol;
};
} // namespace Llp