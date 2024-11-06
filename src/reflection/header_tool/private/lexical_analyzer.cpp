#include "lexical_analyzer.hpp"

#include <format>
#include <iostream>

Block::Block(FileData::Reader& reader)
{
    for (; reader && !reader.try_consume_field("}") && !reader.try_consume_field(")");)
    {
        if (static_cast<int>(*reader) < 0 || isspace(*reader))
        {
            ++reader;
            continue;
        }

        // Skip /* */ comments
        if (reader.try_consume_field("/*"))
        {
            while (reader)
                if (reader.try_consume_field("*/"))
                    break;
        }

        // Skip // comments
        else if (reader.try_consume_field("//"))
            reader.skip_line();

        // Skip quotes
        else if (reader.try_consume_field("\""))
        {
            while (reader)
            {
                if (reader.try_consume_field("\\\\") || reader.try_consume_field("\\\""))
                    continue;
                if (reader.try_consume_field("\""))
                    break;
                ++reader;
            }
        }

        // Search for generated header include
        else if (reader.try_consume_field("#include"))
        {
            bool        b_started = false;
            std::string include;
            do
            {
                if (*reader == '\"' || *reader == '<' || *reader == '>')
                {
                    if (!b_started)
                        b_started = true;
                    else
                        break;
                }
                else if (b_started)
                    include += *reader;
                ++reader;
            } while (reader);
            tokens.emplace_back(std::make_unique<TToken<Include>>(TokenType::Include, include, reader.current_line(), reader.current_char_index()));
            reader.skip_line();
        }

        // Reflect body
        else if (reader.try_consume_field("REFLECT_BODY("))
        {
            std::vector<std::string> flags;
            do
            {
                if (auto word = reader.consume_next_word())
                    flags.emplace_back(*word);
            } while (reader.try_consume_field(","));
            if (reader.try_consume_field(")"))
                tokens.emplace_back(std::make_unique<TToken<ReflectedClassBody>>(TokenType::ReflectedClassBody, ReflectedClassBody{reader.current_line(), reader.current_char_index(), flags}, reader.current_line(),
                                                                                 reader.current_char_index()));
        }

        // Block
        else if (reader.try_consume_field("{"))
            tokens.emplace_back(std::make_unique<TToken<Block>>(TokenType::Block, Block{reader}, reader.current_line(), reader.current_char_index()));
        // Block
        else if (reader.try_consume_field("("))
            tokens.emplace_back(std::make_unique<TToken<Block>>(TokenType::Arguments, Arguments{reader}, reader.current_line(), reader.current_char_index()));

        // Word
        else if (auto word = reader.consume_next_word())
            tokens.emplace_back(std::make_unique<TToken<Word>>(TokenType::Word, *word, reader.current_line(), reader.current_char_index()));

        // Number
        else if (std::isdigit(*reader))
        {
            std::string number;
            do
            {
                number += *reader;
                ++reader;
            } while (reader && (std::isdigit(*reader) || *reader == '.'));
            tokens.emplace_back(std::make_unique<TToken<Number>>(TokenType::Number, number, reader.current_line(), reader.current_char_index()));
        }
        // Other
        else
        {
            tokens.emplace_back(std::make_unique<TToken<Symbol>>(TokenType::Symbol, *reader, reader.current_line(), reader.current_char_index()));
            ++reader;
        }
    }
}

void Block::print(size_t i) const
{
    for (const auto& t : tokens)
    {
        for (size_t x = 0; x < i; ++x)
            std::cout << "\t";
        switch (t->type)
        {
        case TokenType::Symbol:
            std::cout << std::format("S({})", t->data<Symbol>()) << '\n';
            break;
        case TokenType::Word:
            std::cout << std::format("W({})", t->data<Word>()) << '\n';
            break;
        case TokenType::Include:
            std::cout << std::format("#include({})", t->data<Include>()) << '\n';
            break;
        case TokenType::ReflectedClassBody:
            std::cout << "CLASS_BODY(";
            for (const auto& flag : t->data<ReflectedClassBody>().flags)
                std::cout << flag << "," << '\n';
            std::cout << ")\n";
            break;
        case TokenType::Block:
            std::cout << "BEGIN BLOCK{" << '\n';
            t->data<Block>().print(i + 1);
            for (size_t x = 0; x < i; ++x)
                std::cout << "\t";
            std::cout << "} BLOCK END" << '\n';
            break;
        case TokenType::Arguments:
            std::cout << "BEGIN ARGS(" << '\n';
            t->data<Arguments>().print(i + 1);
            for (size_t x = 0; x < i; ++x)
                std::cout << "\t";
            std::cout << ") END ARGS" << '\n';
            break;
        case TokenType::Number:
            std::cout << std::format("#({})", t->data<Number>()) << '\n';
        }
    }
}