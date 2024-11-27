#include "llp/tokens.hpp"
#include <format>
#include <iostream>

namespace Llp
{

void Block::consume_next(const std::string& source, Location& location, std::optional<ParserError>& error)
{
    if (auto comment = error ? nullptr : CommentToken::consume(location, source, error))
        tokens.emplace_back(std::move(comment));
    else if (auto sem = error ? nullptr : SemicolonToken::consume(location, source, error))
        tokens.emplace_back(std::move(sem));
    else if (auto endl = error ? nullptr : EndlToken::consume(location, source, error))
        tokens.emplace_back(std::move(endl));
    else if (auto equals = error ? nullptr : EqualsToken::consume(location, source, error))
        tokens.emplace_back(std::move(equals));
    else if (auto flt = error ? nullptr : FloatingPointToken::consume(location, source, error))
        tokens.emplace_back(std::move(flt));
    else if (auto integer = error ? nullptr : IntegerToken::consume(location, source, error))
        tokens.emplace_back(std::move(integer));
    else if (auto coma = error ? nullptr : ComaToken::consume(location, source, error))
        tokens.emplace_back(std::move(coma));
    else if (auto include = error ? nullptr : IncludeToken::consume(location, source, error))
        tokens.emplace_back(std::move(include));
    else if (auto string = error ? nullptr : StringLiteralToken::consume(location, source, error))
        tokens.emplace_back(std::move(string));
    else if (auto whitespace = error ? nullptr : WhitespaceToken::consume(location, source, error))
        tokens.emplace_back(std::move(whitespace));
    else if (auto word = error ? nullptr : WordToken::consume(location, source, error))
        tokens.emplace_back(std::move(word));
    else if (auto args = error ? nullptr : ArgumentsToken::consume(location, source, error))
        tokens.emplace_back(std::move(args));
    else if (auto block = error ? nullptr : BlockToken::consume(location, source, error))
        tokens.emplace_back(std::move(block));
    else if (auto symbol = error ? nullptr : SymbolToken::consume(location, source, error))
        tokens.emplace_back(std::move(symbol));
    else if (!error)
        error = {location, std::format("Parsing failed {}:{} : unidentified token '{}'", location.line, location.column, source[location.index])};
}

std::string Block::to_string(bool b_debug) const
{
    std::string str;
    for (const auto& token : tokens)
        if (b_debug)
            str += std::format("{}[{}], ", Llp::token_type_to_string(token->get_type()), token->to_string(b_debug));
        else
            str += token->to_string(b_debug);
    return str;
}

std::unique_ptr<CommentToken> CommentToken::consume(Location& in_location, const std::string& source, std::optional<ParserError>& error)
{
    if (in_location.index + 1 < source.size())
    {
        // comment
        if (source[in_location.index] == '/' && source[in_location.index + 1] == '/')
        {
            ++ ++in_location;
            Location start = in_location;

            for (; in_location.index < source.length(); ++in_location.index)
                if (source[in_location.index] == '\n')
                    break;
            std::string value   = source.substr(start.index, in_location.index - start.index);
            auto        comment = std::make_unique<CommentToken>(start);
            comment->value      = value;
            comment->multiline  = false;
            return comment;
        }
        /*comment*/
        if (source[in_location.index] == '/' && source[in_location.index + 1] == '*')
        {
            ++ ++in_location;
            Location start = in_location;
            for (size_t i = start.index; i < source.length() - 1; ++i)
            {
                if (source[i] == '*' && source[i + 1] == '/')
                {
                    std::string value = source.substr(start.index, in_location.index - start.index);
                    ++ ++in_location;
                    auto comment       = std::make_unique<CommentToken>(start);
                    comment->value     = value;
                    comment->multiline = true;
                    return comment;
                }
                if (source[i] == '\n')
                    in_location.on_new_line();
                else
                    ++in_location;
            }
            error = {start, "String comment doesn't end. '*/' expected"};
        }
    }
    return nullptr;
}

std::unique_ptr<SemicolonToken> SemicolonToken::consume(Location& in_location, const std::string& source, std::optional<ParserError>&)
{
    return source[in_location.index] == ';' ? std::make_unique<SemicolonToken>(in_location++) : nullptr;
}

std::unique_ptr<EqualsToken> EqualsToken::consume(Location& in_location, const std::string& source, std::optional<ParserError>&)
{
    return source[in_location.index] == '=' ? std::make_unique<EqualsToken>(in_location++) : nullptr;
}

std::unique_ptr<ComaToken> ComaToken::consume(Location& in_location, const std::string& source, std::optional<ParserError>&)
{
    return source[in_location.index] == ',' ? std::make_unique<ComaToken>(in_location++) : nullptr;
}

std::unique_ptr<EndlToken> EndlToken::consume(Location& in_location, const std::string& source, std::optional<ParserError>&)
{
    return source[in_location.index] == '\n' ? std::make_unique<EndlToken>(in_location.on_new_line()) : nullptr;
}

std::unique_ptr<IncludeToken> IncludeToken::consume(Location& in_location, const std::string& source, std::optional<ParserError>& error)
{
    if (consume_string(in_location, source, "#include"))
    {
        if (consume_whitespace(in_location, source))
        {
            if (source[in_location.index] == '"' || source[in_location.index] == '<')
            {
                Location start = ++in_location;

                for (; in_location.index < source.size(); ++in_location)
                {
                    if (source[in_location.index] == '"' || source[in_location.index] == '>')
                    {
                        auto include  = std::make_unique<IncludeToken>(start);
                        include->path = source.substr(start.index, in_location.index - start.index - 1);
                        ++in_location;
                        return include;
                    }
                }
                error = {start, "No include directive end, expected '>' or '\"'"};
            }
            else
                error = {in_location, "Invalid token, expected '\"' or '<'"};
        }
        else
            error = {in_location, "Invalid #include directive, encountered endl"};
    }
    return nullptr;
}

std::unique_ptr<WhitespaceToken> WhitespaceToken::consume(Location& in_location, const std::string& source, std::optional<ParserError>&)
{
    if (std::isspace(source[in_location.index]))
    {
        Location start = in_location;
        consume_whitespace(in_location, source);
        auto whitespace        = std::make_unique<WhitespaceToken>(start);
        whitespace->whitespace = source.substr(start.index, in_location.index - start.index);
        return whitespace;
    }
    return nullptr;
}

std::unique_ptr<WordToken> WordToken::consume(Location& in_location, const std::string& source, std::optional<ParserError>&)
{
    auto start_chr = source[in_location.index];
    if (std::isalpha(start_chr) || start_chr == '_')
    {
        auto start = in_location++;
        while (in_location.index < source.size())
        {
            auto chr = source[in_location.index];
            if (!std::isalnum(chr) && chr != '_')
                break;
            ++in_location;
        }
        auto word  = std::make_unique<WordToken>(start);
        word->word = source.substr(start.index, in_location.index - start.index);
        return word;
    }
    return nullptr;
}

std::unique_ptr<BlockToken> BlockToken::consume(Location& in_location, const std::string& source, std::optional<ParserError>& error)
{
    if (source[in_location.index] == '{')
    {
        auto block = std::make_unique<BlockToken>(++in_location);
        while (in_location.index < source.size())
        {
            if (source[in_location.index] == '}')
            {
                block->end = in_location++;
                return block;
            }
            block->content.consume_next(source, in_location, error);
            if (error)
                return nullptr;
        }
        error = {in_location, "Missing block end. '}' expected"};
    }
    return nullptr;
}

std::unique_ptr<StringLiteralToken> StringLiteralToken::consume(Location& in_location, const std::string& source, std::optional<ParserError>& error)
{
    if (source[in_location.index] == '"')
    {
        Location start = ++in_location;
        while (in_location.index < source.length() - 1)
        {
            if (source[in_location.index] == '\\' && source[in_location.index + 1] == '\\')
                ++ ++in_location.index;
            if (source[in_location.index] == '"' && source[in_location.index - 1] != '\\')
            {
                std::string value  = source.substr(start.index, in_location.index - start.index);
                auto        string = std::make_unique<StringLiteralToken>(start);
                string->value      = value;
                ++in_location.index;
                return string;
            }
            ++in_location.index;
        }
        error = {start, "Missing string literal end. '\"' expected"};
    }
    return nullptr;
}

std::unique_ptr<ArgumentsToken> ArgumentsToken::consume(Location& in_location, const std::string& source, std::optional<ParserError>& error)
{
    if (source[in_location.index] == '(')
    {
        auto block = std::make_unique<ArgumentsToken>(++in_location);
        while (in_location.index < source.size())
        {
            if (source[in_location.index] == ')')
            {
                block->end = in_location++;
                return block;
            }
            block->content.consume_next(source, in_location, error);
            if (error)
            {
                return nullptr;
            }
        }
        error = {in_location, "Missing arguments end. ')' expected"};
    }
    return nullptr;
}

std::unique_ptr<IntegerToken> IntegerToken::consume(Location& in_location, const std::string& source, std::optional<ParserError>& error)
{
    auto start_chr = source[in_location.index];
    if (std::isdigit(start_chr))
    {
        auto start = in_location;
        auto end   = start;
        while (end.index < source.size())
        {
            auto chr = source[end.index];
            if (!std::isdigit(chr))
                break;
            ++end;
        }
        auto        word    = std::make_unique<IntegerToken>(start);
        std::string num_str = source.substr(start.index, end.index - start.index);
        char*       end_ptr;
        word->number = strtoll(&*num_str.begin(), &end_ptr, 10);
        if (end_ptr != &*num_str.begin() + num_str.size())
        {
            error = {start, "Failed to parse integer number '" + num_str + "'"};
            return nullptr;
        }
        in_location.index += num_str.size();
        return word;
    }
    return nullptr;
}

std::unique_ptr<FloatingPointToken> FloatingPointToken::consume(Location& in_location, const std::string& source, std::optional<ParserError>& error)
{
    auto start_chr = source[in_location.index];
    if (std::isdigit(start_chr) || (in_location.index < source.size() - 1 && start_chr == '.' && std::isdigit(source[in_location.index + 1])))
    {
        bool     b_first_decimal = true;
        Location start           = in_location;
        Location end             = start;
        while (end.index < source.size())
        {
            auto chr = source[end.index];
            if (chr == '.')
            {
                if (b_first_decimal)
                    b_first_decimal = false;
                else
                {
                    error = {start, "Invalid floating point number : unexpected '.'"};
                    return nullptr;
                }
                ++end;
                continue;
            }

            if (!std::isdigit(chr) && chr != '.')
                break;
            ++end;
        }
        if (b_first_decimal)
            return nullptr;
        auto        word    = std::make_unique<FloatingPointToken>(start);
        std::string num_str = "0" + source.substr(start.index, end.index - start.index);
        char*       end_ptr;
        word->number = strtod(&*num_str.begin(), &end_ptr);
        if (end_ptr != &*num_str.begin() + num_str.size())
        {
            error = {start, "Failed to parse floating point number '" + num_str + "'"};
            return nullptr;
        }
        in_location.index += num_str.size() - 1;
        return word;
    }
    return nullptr;
}

std::unique_ptr<SymbolToken> SymbolToken::consume(Location& in_location, const std::string& source, std::optional<ParserError>&)
{
    auto symbol    = std::make_unique<SymbolToken>(in_location);
    symbol->symbol = source[in_location.index++];
    return symbol;
}
} // namespace Llp