#pragma once
#include "tokens.hpp"

namespace Llp {
/*####[ // / * ]####*/
DECLARE_LEXER_TOKEN(CommentToken)
    static std::unique_ptr<CommentToken> consume(Lexer& lexer, Location& in_location, const std::string& source, std::optional<ParserError>& error);

    std::string to_string(bool) const override
    {
        return multiline ? "/*" + value + "*/" : "//" + value;
    }

    bool        multiline = false;
    std::string value;
};

/*####[ ; ]####*/
DECLARE_LEXER_TOKEN(SemicolonToken)
    static std::unique_ptr<SemicolonToken> consume(Lexer& lexer, Location& in_location, const std::string& source, std::optional<ParserError>& error);

    std::string to_string(bool) const override
    {
        return ";";
    }
};

/*####[ = ]####*/
DECLARE_LEXER_TOKEN(EqualsToken)
    static std::unique_ptr<EqualsToken> consume(Lexer& lexer, Location& in_location, const std::string& source, std::optional<ParserError>& error);

    std::string to_string(bool) const override
    {
        return "=";
    }
};

/*####[ , ]####*/
DECLARE_LEXER_TOKEN(ComaToken)
    static std::unique_ptr<ComaToken> consume(Lexer& lexer, Location& in_location, const std::string& source, std::optional<ParserError>& error);

    std::string to_string(bool) const override
    {
        return ",";
    }
};

/*####[ \n ]####*/
DECLARE_LEXER_TOKEN(EndlToken)
    static std::unique_ptr<EndlToken> consume(Lexer& lexer, Location& in_location, const std::string& source, std::optional<ParserError>& error);

    std::string to_string(bool) const override
    {
        return "\n";
    }
};

/*####[ #include "value" ]####*/
DECLARE_LEXER_TOKEN(IncludeToken)
    static std::unique_ptr<IncludeToken> consume(Lexer& lexer, Location& in_location, const std::string& source, std::optional<ParserError>& error);

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
DECLARE_LEXER_TOKEN(WhitespaceToken)
    static std::unique_ptr<WhitespaceToken> consume(Lexer& lexer, Location& in_location, const std::string& source, std::optional<ParserError>& error);

    std::string to_string(bool) const override
    {
        return whitespace;
    }

    std::string whitespace;
};

/*####[ Word ]####*/
DECLARE_LEXER_TOKEN(WordToken)
    static std::unique_ptr<WordToken> consume(Lexer& lexer, Location& in_location, const std::string& source, std::optional<ParserError>& error);

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
DECLARE_LEXER_TOKEN(BlockToken)
    static std::unique_ptr<BlockToken> consume(Lexer& lexer, Location& in_location, const std::string& source, std::optional<ParserError>& error);

    std::string to_string(bool b_debug) const override
    {
        return "(" + content.to_string(b_debug) + ")";
    }

    Location       end;
    TokenizedBlock content;
};

/*####[ "string" ]####*/
DECLARE_LEXER_TOKEN(StringLiteralToken)
    static std::unique_ptr<StringLiteralToken> consume(Lexer& lexer, Location& in_location, const std::string& source, std::optional<ParserError>& error);

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
DECLARE_LEXER_TOKEN(ArgumentsToken)
    static std::unique_ptr<ArgumentsToken> consume(Lexer& lexer, Location& in_location, const std::string& source, std::optional<ParserError>& error);

    std::string to_string(bool b_debug) const override
    {
        return "(" + content.to_string(b_debug) + ")";
    }

    Location       end;
    TokenizedBlock content;
};

/*####[ 000 ]####*/
DECLARE_LEXER_TOKEN(IntegerToken)
    static std::unique_ptr<IntegerToken> consume(Lexer& lexer, Location& in_location, const std::string& source, std::optional<ParserError>& error);

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
DECLARE_LEXER_TOKEN(FloatingPointToken)
    static std::unique_ptr<FloatingPointToken> consume(Lexer& lexer, Location& in_location, const std::string& source, std::optional<ParserError>& error);

    std::string to_string(bool) const override
    {
        return std::to_string(number);
    }

    double number;
};

/*####[ ! ]####*/
DECLARE_LEXER_TOKEN(SymbolToken)
    static std::unique_ptr<SymbolToken> consume(Lexer& lexer, Location& in_location, const std::string& source, std::optional<ParserError>& error);

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
}