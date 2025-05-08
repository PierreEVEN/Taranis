#include "depend_parser.h"

#include <iostream>

#include "llp/file_data.hpp"
#include "llp/lexer.hpp"
#include "llp/parser.hpp"

DependParser::DependParser(const std::filesystem::path &path) {
    if (auto error = parse(path)) {
        std::cerr << path << ":" << error->location.line << ":" << error->location.column << " : " << "Failed to parse depend file : " << error->message << "\n";
        exit(-1);
    }
}

std::time_t DependParser::get_last_write_time() const {
    std::time_t max = 0;
    for (const auto& dep : dependencies) {
        auto ftime = std::filesystem::last_write_time(dep);
        auto sctp = std::chrono::system_clock::to_time_t(std::chrono::file_clock::to_sys(ftime));
        if (sctp > max)
            max = sctp;
    }
    return max;
}

std::optional<Llp::ParserError> DependParser::parse(const std::filesystem::path &path) {
    if (!exists(path))
        return {};
    auto source_header = std::make_shared<FileReader>(path);
    source_header->read();
    Llp::Lexer lexer(source_header->raw_stream());

    auto skip = {Llp::ELexerToken::Whitespace, Llp::ELexerToken::Comment, Llp::ELexerToken::Endl};

    Llp::Parser main_parser(lexer.get_root(), skip);
    if (auto main_block = main_parser.consume<Llp::BlockToken>()) {
        for (Llp::Parser main_block_parser(main_block->content, skip); main_block_parser;) {
            while (auto key = main_block_parser.consume<Llp::WordToken>()) {
                main_block_parser.consume<Llp::EqualsToken>();
                if (key->word == "files") {
                    if (auto dependencies_block = main_block_parser.get<Llp::BlockToken>()) {
                        for (Llp::Parser dependencies_block_parser(dependencies_block->content, skip); dependencies_block_parser;) {
                            if (auto dep_path = dependencies_block_parser.consume<Llp::StringLiteralToken>())
                                dependencies.emplace_back(dep_path->value);
                            else
                                return Llp::ParserError{
                                    main_block_parser.current_location(),
                                    "expected string literal : \"\""
                                };
                            if (dependencies_block_parser.get_current_token_type() == Llp::ELexerToken::Null) {
                                ++dependencies_block_parser;
                                break;
                            }
                            if (!dependencies_block_parser.consume<Llp::ComaToken>())
                                return Llp::ParserError{
                                    main_block_parser.current_location(), "expected coma : ,"
                                };
                        }
                        return {};
                    }
                    return Llp::ParserError{
                        main_block_parser.current_location(),
                        "expected block : {}"
                    };
                }
                ++main_block_parser;
                main_block_parser.consume<Llp::ComaToken>();
            }
        }
    } else
        return Llp::ParserError{
            main_parser.current_location(), "expected block : {}"
        };
    return {};
}
