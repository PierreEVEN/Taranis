#include "depend_parser.h"

#include <iostream>

#include "llp/file_data.hpp"
#include "llp/lexer.hpp"
#include "llp/parser.hpp"

DependParser::DependParser(const std::filesystem::path &path) {
    if (auto error = parse(path)) {
        std::cerr << "Failed to parse depend file : " << error->message << "\n";
        exit(-1);
    }
}

std::optional<Llp::ParserError> DependParser::parse(const std::filesystem::path &path) {
    auto source_header = std::make_shared<FileReader>(path);
    source_header->read();
    Llp::Lexer lexer(source_header->raw_stream());

    auto skip = {Llp::ELexerToken::Whitespace, Llp::ELexerToken::Comment, Llp::ELexerToken::Endl};

    Llp::Parser main_parser(lexer.get_root(), skip);
    if (auto main_block = main_parser.get<Llp::Block>()) {
        for (Llp::Parser main_block_parser(*main_block, skip); main_block_parser;) {
            while (auto key = main_block_parser.consume<Llp::WordToken>()) {
                main_block_parser.consume<Llp::EqualsToken>();
                if (key->word == "files") {
                    if (auto dependencies_block = main_block_parser.get<Llp::Block>()) {
                        for (Llp::Parser dependencies_block_parser(*dependencies_block, skip);
                             dependencies_block_parser;) {
                            if (auto dep_path = dependencies_block_parser.consume<Llp::StringLiteralToken>())
                                dependencies.emplace_back(dep_path->value);
                            else
                                return Llp::ParserError{
                                    main_block_parser.current_location(),
                                    "Failed to parse depend file : expected string literal : \"\""
                                };
                            if (!dependencies_block_parser.consume<Llp::ComaToken>())
                                return Llp::ParserError{
                                    main_block_parser.current_location(), "Failed to parse depend file : coma here : ,"
                                };
                        }
                    } else
                        return Llp::ParserError{
                            main_block_parser.current_location(),
                            "Failed to parse depend file : expected block here : {}"
                        };
                } else {
                    ++main_block_parser;
                }
                main_block_parser.consume<Llp::ComaToken>();
            }
        }
    } else
        return Llp::ParserError{
            main_parser.current_location(), "Failed to parse depend file : expected block here : {}"
        };
    return {};
}
