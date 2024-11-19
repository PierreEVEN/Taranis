#pragma once
#include "lexical_analyzer.hpp"
#include "tokens.hpp"

#include <span>

namespace Llp
{
enum class ELexerToken;
}

namespace Llp
{
class Lexer;

class ParserStructure
{
public:
    ParserStructure& expect(const IToken&)
    {
    }

    ParserStructure& repeat(ParserStructure)
    {
    }
};

struct IAstItem
{
    
};



class IParserStruct
{
public:
    virtual bool test(const std::span<IToken*>& tokens) = 0;
};


class FunctionParser : public IParserStruct
{
public:
    bool test(const std::span<IToken*>& tokens) override
    {
        tokens[0]->type
    }
};


void test()
{
    ParserBlock str = ParserBlock();

    ParserStructure str = ParserStructure().expect(WordToken("class")).expect(ELexerToken::Symbol()).repeat(ParserStructure().expect());

}


class Parser
{
public:
    Parser();

    void add_structure(ParserStructure str);

    void parse(const Lexer& lexer);

private:
};
}